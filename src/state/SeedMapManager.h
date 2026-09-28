#pragma once

#include "worldgen/TileQuery.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// This bridge deliberately owns no Level, BlockSource, ClientInstance, or
// renderer object. The game thread captures a SeedSnapshot and this manager
// only sends copied query values to the pure world-generation workers.
namespace SeedMapManager {

constexpr std::size_t kLayerSettingCount =
    static_cast<std::size_t>(ChiyanMap::WorldGen::LayerId::DappledForest) + 1U;

enum class SearchMode : std::uint8_t {
    VisibleMap,
    Manual,
};

struct Settings final {
    bool                                  enabled = true;
    SearchMode                            searchMode = SearchMode::VisibleMap;
    ChiyanMap::WorldGen::WorldGenProfile profile = ChiyanMap::WorldGen::WorldGenProfile::Bedrock2651;
    ChiyanMap::WorldGen::Dimension       targetDimension = ChiyanMap::WorldGen::Dimension::Overworld;
    std::int64_t                          manualCenterX = 0;
    std::int64_t                          manualCenterZ = 0;
    std::int64_t                          manualRadius = ChiyanMap::WorldGen::kDefaultManualSearchRadius;
    std::array<bool, kLayerSettingCount>  enabledLayers{};
};

struct Status final {
    bool                                            initialized = false;
    bool                                            seedAvailable = false;
    std::optional<std::uint64_t>                    capturedSeedBits;
    std::optional<ChiyanMap::WorldGen::Dimension>  currentGameDimension;
    std::size_t                                     queuedTiles = 0;
    std::size_t                                     activeWorkers = 0;
    std::size_t                                     cachedTiles = 0;
    ChiyanMap::WorldGen::LayerQueryStatistics       cachedQueryStatistics{};
    bool                                            isManual = false;
};

struct ManualSearchProgress final {
    bool                                 requested = false;
    bool                                 seedAvailable = false;
    ChiyanMap::WorldGen::BlockRect       bounds{};
    ChiyanMap::WorldGen::Dimension       dimension = ChiyanMap::WorldGen::Dimension::Overworld;
    std::size_t                          completedTiles = 0;
    std::size_t                          totalTiles = 0;
};

[[nodiscard]] Settings DefaultSettings() noexcept;

void Init(std::size_t workerCount = 2);
void Shutdown();

// Game-thread entry points. A valid zero seed is represented by
// SeedSnapshot::FromLevelSeed(0); an unreadable seed is SeedSnapshot::Unavailable().
void NotifyWorldChanged(
    std::string                           worldId,
    ChiyanMap::WorldGen::SeedSnapshot     seed,
    std::optional<std::uint64_t>          capturedSeedBits,
    int                                   gameDimensionId
);
void ClearWorldContext();

void SetManualSeed(std::uint64_t seedBits);
void ResetToAutoSeed();
[[nodiscard]] std::optional<std::uint64_t> GetSavedManualSeed(const std::string& worldId);
void SaveManualSeed(const std::string& worldId, std::uint64_t seedBits);
void ClearManualSeed(const std::string& worldId);
[[nodiscard]] std::unordered_map<std::string, std::uint64_t> GetSavedManualSeeds();
void SetSavedManualSeeds(const std::unordered_map<std::string, std::uint64_t>& seeds);

[[nodiscard]] std::optional<ChiyanMap::WorldGen::Dimension>
DimensionFromGameDimensionId(int dimensionId) noexcept;

[[nodiscard]] Settings GetSettings();
void SetSettings(Settings settings);
bool SetLayerEnabled(ChiyanMap::WorldGen::LayerId layer, bool enabled);
// Disables every queryable layer in one invalidation step. Returns true only
// when at least one enabled layer changed state.
bool ClearAllLayers();
[[nodiscard]] bool IsLayerEnabled(ChiyanMap::WorldGen::LayerId layer);
[[nodiscard]] Status GetStatus();

// Queues enabled seed-only layers for the current target dimension. The
// half-open bounds are copied into worker tasks; no game object crosses this
// boundary.
void RequestVisibleMap(ChiyanMap::WorldGen::BlockRect bounds);

// Starts a bounded manual search. Radius is clamped to the worldgen cap. The
// resulting markers are available through GetManualMarkers().
bool StartManualSearch(std::int64_t centerX, std::int64_t centerZ, std::int64_t radius);
[[nodiscard]] ManualSearchProgress GetManualSearchProgress();

[[nodiscard]] std::vector<ChiyanMap::WorldGen::SeedMapMarker> GetMarkersInBounds(
    ChiyanMap::WorldGen::BlockRect bounds,
    ChiyanMap::WorldGen::Dimension dimension
);
[[nodiscard]] std::vector<ChiyanMap::WorldGen::SeedMapMarker> GetManualMarkers();

void SelectMarker(const ChiyanMap::WorldGen::SeedMapMarker& marker);
void ClearSelectedMarker();
[[nodiscard]] std::optional<ChiyanMap::WorldGen::SeedMapMarker> GetSelectedMarker();

} // namespace SeedMapManager
