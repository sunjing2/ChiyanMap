#pragma once

#include "worldgen/StructureCatalog.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ChiyanMap::WorldGen {

inline constexpr std::int64_t kDefaultTileSizeBlocks      = 1024;
inline constexpr std::int64_t kDefaultManualSearchRadius  = 2000;
inline constexpr std::int64_t kMaximumManualSearchRadius  = 25000;

class TileCoordinate final {
public:
    constexpr TileCoordinate(std::int64_t x, std::int64_t z) noexcept : x_(x), z_(z) {}

    [[nodiscard]] constexpr std::int64_t X() const noexcept {
        return x_;
    }

    [[nodiscard]] constexpr std::int64_t Z() const noexcept {
        return z_;
    }

    [[nodiscard]] constexpr bool operator==(const TileCoordinate&) const noexcept = default;

private:
    std::int64_t x_ = 0;
    std::int64_t z_ = 0;
};

struct TileCacheKey final {
    std::uint64_t   seedBits = 0;
    WorldGenProfile profile{};
    Dimension       dimension{};
    LayerId         layer{};
    TileCoordinate  tile{0, 0};
    std::int64_t    tileSizeBlocks = kDefaultTileSizeBlocks;

    [[nodiscard]] constexpr bool operator==(const TileCacheKey&) const noexcept = default;
};

[[nodiscard]] std::int64_t FloorDivide(std::int64_t value, std::int64_t positiveDivisor) noexcept;
[[nodiscard]] TileCoordinate TileForBlock(
    BlockPosition point,
    std::int64_t  tileSizeBlocks = kDefaultTileSizeBlocks
) noexcept;
[[nodiscard]] std::optional<BlockRect> TryBoundsForTile(
    TileCoordinate tile,
    std::int64_t   tileSizeBlocks = kDefaultTileSizeBlocks
) noexcept;
[[nodiscard]] std::optional<LayerQuery> TryMakeTileQuery(
    SeedSnapshot    seed,
    WorldGenProfile profile,
    Dimension       dimension,
    LayerId         layer,
    TileCoordinate  tile,
    std::int64_t    tileSizeBlocks = kDefaultTileSizeBlocks
) noexcept;
[[nodiscard]] std::optional<TileCacheKey> TryMakeTileCacheKey(
    SeedSnapshot    seed,
    WorldGenProfile profile,
    Dimension       dimension,
    LayerId         layer,
    TileCoordinate  tile,
    std::int64_t    tileSizeBlocks = kDefaultTileSizeBlocks
) noexcept;

[[nodiscard]] LayerQueryResult QueryPlacementCandidatesForTile(
    SeedSnapshot    seed,
    WorldGenProfile profile,
    Dimension       dimension,
    LayerId         layer,
    TileCoordinate  tile,
    std::int64_t    tileSizeBlocks = kDefaultTileSizeBlocks
);

// Safe to apply to overlapping cached tiles or repeated worker completions.
// The sort order is deterministic: layer, dimension, chunk X, then chunk Z.
[[nodiscard]] std::vector<SeedMapMarker> DeduplicateMarkers(std::vector<SeedMapMarker> markers);

[[nodiscard]] std::size_t HashTileCacheKey(const TileCacheKey& key) noexcept;

} // namespace ChiyanMap::WorldGen
