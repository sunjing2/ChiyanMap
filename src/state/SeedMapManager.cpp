#include "state/SeedMapManager.h"

#include "worldgen/StructureCatalog.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace SeedMapManager {
namespace {

using ChiyanMap::WorldGen::BlockPosition;
using ChiyanMap::WorldGen::BlockRect;
using ChiyanMap::WorldGen::Dimension;
using ChiyanMap::WorldGen::LayerCatalog;
using ChiyanMap::WorldGen::LayerDescriptor;
using ChiyanMap::WorldGen::LayerId;
using ChiyanMap::WorldGen::LayerQueryResult;
using ChiyanMap::WorldGen::LayerSupportState;
using ChiyanMap::WorldGen::SeedMapMarker;
using ChiyanMap::WorldGen::SeedSnapshot;
using ChiyanMap::WorldGen::TileCacheKey;
using ChiyanMap::WorldGen::TileCoordinate;
using ChiyanMap::WorldGen::WorldGenProfile;

[[nodiscard]] constexpr std::size_t LayerIndex(LayerId layer) noexcept {
    return static_cast<std::size_t>(layer);
}

[[nodiscard]] constexpr bool IsLayerIndexValid(LayerId layer) noexcept {
    return LayerIndex(layer) < kLayerSettingCount;
}

[[nodiscard]] Settings MakeDefaultSettings() noexcept {
    Settings settings;
    settings.enabledLayers.fill(false);
    return settings;
}

struct TileCacheKeyHash final {
    [[nodiscard]] std::size_t operator()(const TileCacheKey& key) const noexcept {
        return ChiyanMap::WorldGen::HashTileCacheKey(key);
    }
};

struct WorkerTask final {
    TileCacheKey  key{};
    SeedSnapshot  seed{};
    WorldGenProfile profile = WorldGenProfile::Bedrock2651;
    Dimension     dimension = Dimension::Overworld;
    LayerId       layer = LayerId::Village;
    TileCoordinate tile{0, 0};
    std::uint64_t generation = 0;
};

struct ManualSearchState final {
    bool requested = false;
    BlockRect bounds{};
    Dimension dimension = Dimension::Overworld;
    std::unordered_set<TileCacheKey, TileCacheKeyHash> requestedTiles;
    std::unordered_set<TileCacheKey, TileCacheKeyHash> completedTiles;
};

struct State final {
    State() : settings(MakeDefaultSettings()) {}

    std::mutex mutex;
    std::condition_variable workerSignal;
    bool initialized = false;
    bool stopping = false;
    std::vector<std::thread> workers;
    std::deque<WorkerTask> pendingTasks;
    std::unordered_set<TileCacheKey, TileCacheKeyHash> pendingKeys;
    std::unordered_map<TileCacheKey, LayerQueryResult, TileCacheKeyHash> tileCache;

    Settings settings;
    std::string worldId;
    SeedSnapshot seed = SeedSnapshot::Unavailable();
    std::optional<std::uint64_t> capturedSeedBits;
    SeedSnapshot autoSeed = SeedSnapshot::Unavailable();
    std::optional<std::uint64_t> autoCapturedSeedBits;
    bool isManualSeed = false;
    std::optional<Dimension> currentGameDimension;
    std::uint64_t generation = 0;
    std::size_t activeWorkers = 0;
    ManualSearchState manualSearch;
    std::optional<SeedMapMarker> selectedMarker;
};

State g_state;
std::unordered_map<std::string, std::uint64_t> g_savedManualSeeds;

[[nodiscard]] bool IsSeedQueryableLayer(LayerId layer) noexcept {
    const LayerDescriptor* descriptor = ChiyanMap::WorldGen::FindLayerDescriptor(layer);
    return descriptor != nullptr && ChiyanMap::WorldGen::IsMarkerEmissionAllowed(descriptor->support);
}

[[nodiscard]] bool AreSettingsEqual(const Settings& left, const Settings& right) noexcept {
    return left.enabled == right.enabled
        && left.searchMode == right.searchMode
        && left.profile == right.profile
        && left.targetDimension == right.targetDimension
        && left.manualCenterX == right.manualCenterX
        && left.manualCenterZ == right.manualCenterZ
        && left.manualRadius == right.manualRadius
        && left.enabledLayers == right.enabledLayers;
}

void SanitizeSettings(Settings& settings) noexcept {
    if (settings.manualRadius < 0) settings.manualRadius = 0;
    if (settings.manualRadius > ChiyanMap::WorldGen::kMaximumManualSearchRadius) {
        settings.manualRadius = ChiyanMap::WorldGen::kMaximumManualSearchRadius;
    }

    for (std::size_t index = 0; index < settings.enabledLayers.size(); ++index) {
        const auto layer = static_cast<LayerId>(index);
        if (!IsSeedQueryableLayer(layer)) settings.enabledLayers[index] = false;
    }
}

void InvalidateLocked() {
    ++g_state.generation;
    g_state.pendingTasks.clear();
    g_state.pendingKeys.clear();
    g_state.tileCache.clear();
    g_state.manualSearch = ManualSearchState{};
    g_state.selectedMarker.reset();
}

[[nodiscard]] bool IsLayerEnabledLocked(LayerId layer) noexcept {
    return IsLayerIndexValid(layer)
        && g_state.settings.enabledLayers[LayerIndex(layer)]
        && IsSeedQueryableLayer(layer);
}

[[nodiscard]] bool TryMakeSquareBounds(
    std::int64_t centerX,
    std::int64_t centerZ,
    std::int64_t radius,
    BlockRect& output
) noexcept {
    if (radius < 0 || radius > ChiyanMap::WorldGen::kMaximumManualSearchRadius) return false;
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    if (centerX < min + radius || centerX > max - radius || centerZ < min + radius || centerZ > max - radius) {
        return false;
    }

    output.minX = centerX - radius;
    output.minZ = centerZ - radius;
    if (centerX > max - radius - 1 || centerZ > max - radius - 1) return false;
    output.maxXExclusive = centerX + radius + 1;
    output.maxZExclusive = centerZ + radius + 1;
    return output.IsValid();
}

struct ScheduledTask final {
    WorkerTask task{};
    std::uint64_t distance = 0;
};

[[nodiscard]] std::uint64_t TileDistance(
    const TileCoordinate& tile,
    std::int64_t centerX,
    std::int64_t centerZ
) noexcept {
    const auto distanceX = tile.X() >= centerX
        ? static_cast<std::uint64_t>(tile.X() - centerX)
        : static_cast<std::uint64_t>(centerX - tile.X());
    const auto distanceZ = tile.Z() >= centerZ
        ? static_cast<std::uint64_t>(tile.Z() - centerZ)
        : static_cast<std::uint64_t>(centerZ - tile.Z());
    return distanceX > std::numeric_limits<std::uint64_t>::max() - distanceZ
        ? std::numeric_limits<std::uint64_t>::max()
        : distanceX + distanceZ;
}

void ScheduleBoundsLocked(BlockRect bounds, Dimension dimension, ManualSearchState* manualSearch) {
    if (!g_state.initialized || !g_state.settings.enabled || !g_state.seed.HasSeed() || !bounds.IsValid()) return;

    const TileCoordinate minTile = ChiyanMap::WorldGen::TileForBlock(
        BlockPosition{bounds.minX, bounds.minZ}
    );
    const TileCoordinate maxTile = ChiyanMap::WorldGen::TileForBlock(
        BlockPosition{bounds.maxXExclusive - 1, bounds.maxZExclusive - 1}
    );
    const std::int64_t centerTileX = minTile.X() + (maxTile.X() - minTile.X()) / 2;
    const std::int64_t centerTileZ = minTile.Z() + (maxTile.Z() - minTile.Z()) / 2;

    std::vector<ScheduledTask> candidates;
    for (const LayerDescriptor& descriptor : LayerCatalog()) {
        if (descriptor.dimension != dimension || !IsLayerEnabledLocked(descriptor.id)) continue;

        for (std::int64_t tileZ = minTile.Z();; ++tileZ) {
            for (std::int64_t tileX = minTile.X();; ++tileX) {
                const TileCoordinate tile{tileX, tileZ};
                const auto key = ChiyanMap::WorldGen::TryMakeTileCacheKey(
                    g_state.seed,
                    g_state.settings.profile,
                    dimension,
                    descriptor.id,
                    tile
                );
                if (!key) continue;

                if (manualSearch != nullptr) {
                    manualSearch->requestedTiles.insert(*key);
                    if (g_state.tileCache.contains(*key)) manualSearch->completedTiles.insert(*key);
                }

                if (!g_state.tileCache.contains(*key) && !g_state.pendingKeys.contains(*key)) {
                    candidates.push_back({
                        WorkerTask{*key, g_state.seed, g_state.settings.profile, dimension, descriptor.id, tile,
                                   g_state.generation},
                        TileDistance(tile, centerTileX, centerTileZ)
                    });
                }

                if (tileX == maxTile.X()) break;
            }
            if (tileZ == maxTile.Z()) break;
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const ScheduledTask& left, const ScheduledTask& right) {
        if (left.distance != right.distance) return left.distance < right.distance;
        if (left.task.layer != right.task.layer) {
            return static_cast<std::uint8_t>(left.task.layer) < static_cast<std::uint8_t>(right.task.layer);
        }
        if (left.task.tile.X() != right.task.tile.X()) return left.task.tile.X() < right.task.tile.X();
        return left.task.tile.Z() < right.task.tile.Z();
    });

    for (ScheduledTask& candidate : candidates) {
        if (g_state.pendingKeys.insert(candidate.task.key).second) {
            g_state.pendingTasks.push_back(std::move(candidate.task));
        }
    }
    if (!candidates.empty()) g_state.workerSignal.notify_all();
}

void WorkerLoop() {
    for (;;) {
        WorkerTask task;
        {
            std::unique_lock lock(g_state.mutex);
            g_state.workerSignal.wait(lock, [] { return g_state.stopping || !g_state.pendingTasks.empty(); });
            if (g_state.stopping && g_state.pendingTasks.empty()) return;

            task = std::move(g_state.pendingTasks.front());
            g_state.pendingTasks.pop_front();
            ++g_state.activeWorkers;
        }

        LayerQueryResult result;
        try {
            result = ChiyanMap::WorldGen::QueryPlacementCandidatesForTile(
                task.seed,
                task.profile,
                task.dimension,
                task.layer,
                task.tile
            );
        } catch (...) {
            // A failed pure query is not allowed to become a speculative map marker.
            result.markers.clear();
            result.statistics = {};
        }

        {
            std::lock_guard lock(g_state.mutex);
            --g_state.activeWorkers;
            if (!g_state.stopping && task.generation == g_state.generation) {
                g_state.pendingKeys.erase(task.key);
                g_state.tileCache.emplace(task.key, std::move(result));
                if (g_state.manualSearch.requestedTiles.contains(task.key)) {
                    g_state.manualSearch.completedTiles.insert(task.key);
                }
            }
        }
    }
}

} // namespace

Settings DefaultSettings() noexcept {
    return MakeDefaultSettings();
}

void Init(std::size_t workerCount) {
    workerCount = std::clamp<std::size_t>(workerCount, 1, 2);
    std::lock_guard lock(g_state.mutex);
    if (g_state.initialized) return;

    g_state.stopping = false;
    g_state.initialized = true;
    g_state.workers.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index) {
        g_state.workers.emplace_back(WorkerLoop);
    }
}

void Shutdown() {
    std::vector<std::thread> workers;
    {
        std::lock_guard lock(g_state.mutex);
        if (!g_state.initialized) return;
        g_state.stopping = true;
        ++g_state.generation;
        g_state.pendingTasks.clear();
        g_state.pendingKeys.clear();
        workers = std::move(g_state.workers);
    }
    g_state.workerSignal.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable()) worker.join();
    }

    std::lock_guard lock(g_state.mutex);
    g_state.tileCache.clear();
    g_state.manualSearch = ManualSearchState{};
    g_state.selectedMarker.reset();
    g_state.activeWorkers = 0;
    g_state.initialized = false;
    g_state.stopping = false;
}

std::optional<Dimension> DimensionFromGameDimensionId(int dimensionId) noexcept {
    switch (dimensionId) {
    case 0: return Dimension::Overworld;
    case 1: return Dimension::Nether;
    case 2: return Dimension::End;
    default: return std::nullopt;
    }
}

void NotifyWorldChanged(
    std::string worldId,
    SeedSnapshot seed,
    std::optional<std::uint64_t> capturedSeedBits,
    int gameDimensionId
) {
    const auto dimension = DimensionFromGameDimensionId(gameDimensionId);
    std::lock_guard lock(g_state.mutex);

    g_state.autoSeed = seed;
    g_state.autoCapturedSeedBits = capturedSeedBits;

    SeedSnapshot effectiveSeed = seed;
    std::optional<std::uint64_t> effectiveBits = capturedSeedBits;
    bool manual = false;

    auto it = g_savedManualSeeds.find(worldId);
    if (it != g_savedManualSeeds.end()) {
        effectiveSeed = SeedSnapshot::FromSeedBits(it->second);
        effectiveBits = it->second;
        manual = true;
    }

    if (g_state.worldId == worldId && g_state.seed == effectiveSeed && g_state.capturedSeedBits == effectiveBits
        && g_state.currentGameDimension == dimension && g_state.isManualSeed == manual) {
        return;
    }

    g_state.worldId = std::move(worldId);
    g_state.seed = effectiveSeed;
    g_state.capturedSeedBits = effectiveBits;
    g_state.currentGameDimension = dimension;
    g_state.isManualSeed = manual;
    InvalidateLocked();
}

void ClearWorldContext() {
    std::lock_guard lock(g_state.mutex);
    if (g_state.worldId.empty() && !g_state.seed.HasSeed() && !g_state.currentGameDimension) return;

    g_state.worldId.clear();
    g_state.seed = SeedSnapshot::Unavailable();
    g_state.autoSeed = SeedSnapshot::Unavailable();
    g_state.capturedSeedBits.reset();
    g_state.autoCapturedSeedBits.reset();
    g_state.isManualSeed = false;
    g_state.currentGameDimension.reset();
    InvalidateLocked();
}

void SetManualSeed(std::uint64_t seedBits) {
    std::lock_guard lock(g_state.mutex);
    g_state.seed = SeedSnapshot::FromSeedBits(seedBits);
    g_state.capturedSeedBits = seedBits;
    g_state.isManualSeed = true;
    if (!g_state.worldId.empty()) {
        g_savedManualSeeds[g_state.worldId] = seedBits;
    }
    InvalidateLocked();
}

void ResetToAutoSeed() {
    std::lock_guard lock(g_state.mutex);
    if (!g_state.worldId.empty()) {
        g_savedManualSeeds.erase(g_state.worldId);
    }
    g_state.isManualSeed = false;
    g_state.seed = g_state.autoSeed;
    g_state.capturedSeedBits = g_state.autoCapturedSeedBits;
    InvalidateLocked();
}

std::optional<std::uint64_t> GetSavedManualSeed(const std::string& worldId) {
    std::lock_guard lock(g_state.mutex);
    auto it = g_savedManualSeeds.find(worldId);
    if (it != g_savedManualSeeds.end()) return it->second;
    return std::nullopt;
}

void SaveManualSeed(const std::string& worldId, std::uint64_t seedBits) {
    std::lock_guard lock(g_state.mutex);
    g_savedManualSeeds[worldId] = seedBits;
}

void ClearManualSeed(const std::string& worldId) {
    std::lock_guard lock(g_state.mutex);
    g_savedManualSeeds.erase(worldId);
}

std::unordered_map<std::string, std::uint64_t> GetSavedManualSeeds() {
    std::lock_guard lock(g_state.mutex);
    return g_savedManualSeeds;
}

void SetSavedManualSeeds(const std::unordered_map<std::string, std::uint64_t>& seeds) {
    std::lock_guard lock(g_state.mutex);
    g_savedManualSeeds = seeds;
}

Settings GetSettings() {
    std::lock_guard lock(g_state.mutex);
    return g_state.settings;
}

void SetSettings(Settings settings) {
    SanitizeSettings(settings);
    std::lock_guard lock(g_state.mutex);
    if (AreSettingsEqual(g_state.settings, settings)) return;

    g_state.settings = settings;
    InvalidateLocked();
}

bool SetLayerEnabled(LayerId layer, bool enabled) {
    if (!IsLayerIndexValid(layer) || (enabled && !IsSeedQueryableLayer(layer))) return false;

    std::lock_guard lock(g_state.mutex);
    if (g_state.settings.enabledLayers[LayerIndex(layer)] == enabled) return true;

    g_state.settings.enabledLayers[LayerIndex(layer)] = enabled;
    InvalidateLocked();
    return true;
}

bool ClearAllLayers() {
    std::lock_guard lock(g_state.mutex);
    bool changed = false;
    for (const LayerDescriptor& descriptor : LayerCatalog()) {
        if (!IsSeedQueryableLayer(descriptor.id)) continue;
        bool& enabled = g_state.settings.enabledLayers[LayerIndex(descriptor.id)];
        if (!enabled) continue;
        enabled = false;
        changed = true;
    }
    if (changed) InvalidateLocked();
    return changed;
}

bool IsLayerEnabled(LayerId layer) {
    std::lock_guard lock(g_state.mutex);
    return IsLayerEnabledLocked(layer);
}

Status GetStatus() {
    std::lock_guard lock(g_state.mutex);
    ChiyanMap::WorldGen::LayerQueryStatistics statistics;
    for (const auto& [key, result] : g_state.tileCache) {
        static_cast<void>(key);
        statistics.placementCandidates += result.statistics.placementCandidates;
        statistics.biomeRejectedCandidates += result.statistics.biomeRejectedCandidates;
        statistics.emittedMarkers += result.statistics.emittedMarkers;
    }
    return {
        g_state.initialized,
        g_state.seed.HasSeed(),
        g_state.capturedSeedBits,
        g_state.currentGameDimension,
        g_state.pendingTasks.size(),
        g_state.activeWorkers,
        g_state.tileCache.size(),
        statistics,
        g_state.isManualSeed,
    };
}

void RequestVisibleMap(BlockRect bounds) {
    std::lock_guard lock(g_state.mutex);
    ScheduleBoundsLocked(bounds, g_state.settings.targetDimension, nullptr);
}

bool StartManualSearch(std::int64_t centerX, std::int64_t centerZ, std::int64_t radius) {
    radius = std::clamp<std::int64_t>(radius, 0, ChiyanMap::WorldGen::kMaximumManualSearchRadius);
    BlockRect bounds;
    if (!TryMakeSquareBounds(centerX, centerZ, radius, bounds)) return false;

    std::lock_guard lock(g_state.mutex);
    g_state.settings.manualCenterX = centerX;
    g_state.settings.manualCenterZ = centerZ;
    g_state.settings.manualRadius = radius;
    g_state.settings.searchMode = SearchMode::Manual;
    g_state.manualSearch = ManualSearchState{};
    g_state.manualSearch.requested = true;
    g_state.manualSearch.bounds = bounds;
    g_state.manualSearch.dimension = g_state.settings.targetDimension;
    ScheduleBoundsLocked(bounds, g_state.manualSearch.dimension, &g_state.manualSearch);
    return true;
}

ManualSearchProgress GetManualSearchProgress() {
    std::lock_guard lock(g_state.mutex);
    return {
        g_state.manualSearch.requested,
        g_state.seed.HasSeed(),
        g_state.manualSearch.bounds,
        g_state.manualSearch.dimension,
        g_state.manualSearch.completedTiles.size(),
        g_state.manualSearch.requestedTiles.size(),
    };
}

std::vector<SeedMapMarker> GetMarkersInBounds(BlockRect bounds, Dimension dimension) {
    if (!bounds.IsValid()) return {};

    std::vector<SeedMapMarker> result;
    {
        std::lock_guard lock(g_state.mutex);
        if (!g_state.seed.HasSeed() || !g_state.settings.enabled || dimension != g_state.settings.targetDimension) {
            return result;
        }

        for (const auto& [key, tileResult] : g_state.tileCache) {
            if (key.seedBits != *g_state.seed.TrySeedBits()
                || key.profile != g_state.settings.profile
                || key.dimension != dimension
                || !IsLayerEnabledLocked(key.layer)) {
                continue;
            }
            for (const SeedMapMarker& marker : tileResult.markers) {
                if (bounds.Contains(marker.Block())) result.push_back(marker);
            }
        }
    }
    return ChiyanMap::WorldGen::DeduplicateMarkers(std::move(result));
}

std::vector<SeedMapMarker> GetManualMarkers() {
    ManualSearchProgress progress = GetManualSearchProgress();
    if (!progress.requested) return {};
    return GetMarkersInBounds(progress.bounds, progress.dimension);
}

void SelectMarker(const SeedMapMarker& marker) {
    std::lock_guard lock(g_state.mutex);
    g_state.selectedMarker = marker;
}

void ClearSelectedMarker() {
    std::lock_guard lock(g_state.mutex);
    g_state.selectedMarker.reset();
}

std::optional<SeedMapMarker> GetSelectedMarker() {
    std::lock_guard lock(g_state.mutex);
    return g_state.selectedMarker;
}

} // namespace SeedMapManager
