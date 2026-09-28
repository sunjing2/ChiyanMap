#include "worldgen/TileQuery.h"

#include <algorithm>
#include <cassert>
#include <limits>

namespace ChiyanMap::WorldGen {
namespace {

[[nodiscard]] bool TryMultiply(std::int64_t value, std::int64_t multiplier, std::int64_t& output) noexcept {
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    constexpr auto max = std::numeric_limits<std::int64_t>::max();

    if (multiplier <= 0) return false;
    if (value > 0 && value > max / multiplier) return false;
    if (value < 0 && value < min / multiplier) return false;

    output = value * multiplier;
    return true;
}

[[nodiscard]] constexpr std::uint64_t MixHash(std::uint64_t value) noexcept {
    value ^= value >> 30U;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27U;
    value *= 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

[[nodiscard]] constexpr bool IsBefore(const MarkerKey& left, const MarkerKey& right) noexcept {
    if (left.layer != right.layer) return static_cast<std::uint8_t>(left.layer) < static_cast<std::uint8_t>(right.layer);
    if (left.dimension != right.dimension) {
        return static_cast<std::int8_t>(left.dimension) < static_cast<std::int8_t>(right.dimension);
    }
    if (left.chunk.x != right.chunk.x) return left.chunk.x < right.chunk.x;
    return left.chunk.z < right.chunk.z;
}

} // namespace

std::int64_t FloorDivide(std::int64_t value, std::int64_t positiveDivisor) noexcept {
    assert(positiveDivisor > 0);
    if (positiveDivisor <= 0) return 0;

    const std::int64_t quotient  = value / positiveDivisor;
    const std::int64_t remainder = value % positiveDivisor;
    return quotient - (remainder < 0 ? 1 : 0);
}

TileCoordinate TileForBlock(BlockPosition point, std::int64_t tileSizeBlocks) noexcept {
    return TileCoordinate{FloorDivide(point.x, tileSizeBlocks), FloorDivide(point.z, tileSizeBlocks)};
}

std::optional<BlockRect> TryBoundsForTile(TileCoordinate tile, std::int64_t tileSizeBlocks) noexcept {
    if (tileSizeBlocks <= 0) return std::nullopt;

    BlockRect bounds;
    if (!TryMultiply(tile.X(), tileSizeBlocks, bounds.minX) || !TryMultiply(tile.Z(), tileSizeBlocks, bounds.minZ)) {
        return std::nullopt;
    }
    if (bounds.minX > std::numeric_limits<std::int64_t>::max() - tileSizeBlocks
        || bounds.minZ > std::numeric_limits<std::int64_t>::max() - tileSizeBlocks) {
        return std::nullopt;
    }

    bounds.maxXExclusive = bounds.minX + tileSizeBlocks;
    bounds.maxZExclusive = bounds.minZ + tileSizeBlocks;
    return bounds;
}

std::optional<LayerQuery> TryMakeTileQuery(
    SeedSnapshot    seed,
    WorldGenProfile profile,
    Dimension       dimension,
    LayerId         layer,
    TileCoordinate  tile,
    std::int64_t    tileSizeBlocks
) noexcept {
    const auto bounds = TryBoundsForTile(tile, tileSizeBlocks);
    if (!bounds) return std::nullopt;
    return LayerQuery{seed, profile, dimension, layer, *bounds};
}

std::optional<TileCacheKey> TryMakeTileCacheKey(
    SeedSnapshot    seed,
    WorldGenProfile profile,
    Dimension       dimension,
    LayerId         layer,
    TileCoordinate  tile,
    std::int64_t    tileSizeBlocks
) noexcept {
    const auto seedBits = seed.TrySeedBits();
    if (!seedBits || !TryBoundsForTile(tile, tileSizeBlocks)) return std::nullopt;
    return TileCacheKey{*seedBits, profile, dimension, layer, tile, tileSizeBlocks};
}

LayerQueryResult QueryPlacementCandidatesForTile(
    SeedSnapshot    seed,
    WorldGenProfile profile,
    Dimension       dimension,
    LayerId         layer,
    TileCoordinate  tile,
    std::int64_t    tileSizeBlocks
) {
    const auto query = TryMakeTileQuery(seed, profile, dimension, layer, tile, tileSizeBlocks);
    if (query) return QueryPlacementCandidates(*query);

    const LayerDescriptor* descriptor = FindLayerDescriptor(layer);
    return {QueryStatus::CoordinateOverflow,
            descriptor == nullptr ? LayerSupportState::Unavailable : descriptor->support,
            {}};
}

std::vector<SeedMapMarker> DeduplicateMarkers(std::vector<SeedMapMarker> markers) {
    std::sort(markers.begin(), markers.end(), [](const SeedMapMarker& left, const SeedMapMarker& right) {
        return IsBefore(GetMarkerKey(left), GetMarkerKey(right));
    });
    markers.erase(
        std::unique(markers.begin(), markers.end(), [](const SeedMapMarker& left, const SeedMapMarker& right) {
            return GetMarkerKey(left) == GetMarkerKey(right);
        }),
        markers.end()
    );
    return markers;
}

std::size_t HashTileCacheKey(const TileCacheKey& key) noexcept {
    std::uint64_t hash = MixHash(key.seedBits);
    hash ^= MixHash(static_cast<std::uint64_t>(static_cast<std::uint8_t>(key.profile)) + 0x9E3779B97F4A7C15ULL);
    hash ^= MixHash(static_cast<std::uint64_t>(static_cast<std::int8_t>(key.dimension)) + 0xD1B54A32D192ED03ULL);
    hash ^= MixHash(static_cast<std::uint64_t>(static_cast<std::uint8_t>(key.layer)) + 0x94D049BB133111EBULL);
    hash ^= MixHash(static_cast<std::uint64_t>(key.tile.X()));
    hash ^= MixHash(static_cast<std::uint64_t>(key.tile.Z()));
    hash ^= MixHash(static_cast<std::uint64_t>(key.tileSizeBlocks));
    return static_cast<std::size_t>(hash);
}

} // namespace ChiyanMap::WorldGen
