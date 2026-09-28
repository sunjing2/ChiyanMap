#include "worldgen/BedrockBiomeValidator.h"

#include "finders.h"
#include "biomes.h"

#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace ChiyanMap::WorldGen {
namespace {

constexpr int kCubiomesViewerBedrockVersion = MC_26_51;

[[nodiscard]] constexpr bool IsIntCoordinate(std::int64_t value) noexcept {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}

[[nodiscard]] constexpr std::optional<int> ToCubiomesDimension(Dimension dimension) noexcept {
    switch (dimension) {
    case Dimension::Overworld: return DIM_OVERWORLD;
    case Dimension::Nether: return DIM_NETHER;
    case Dimension::End: return DIM_END;
    }
    return std::nullopt;
}

struct StructureSpec final {
    int       structure = 0;
    Dimension dimension = Dimension::Overworld;
};

[[nodiscard]] constexpr std::optional<StructureSpec> ToCubiomesStructure(LayerId layer) noexcept {
    switch (layer) {
    case LayerId::Village: return StructureSpec{Village, Dimension::Overworld};
    case LayerId::PillagerOutpost: return StructureSpec{Outpost, Dimension::Overworld};
    case LayerId::DesertPyramid: return StructureSpec{Desert_Pyramid, Dimension::Overworld};
    case LayerId::JungleTemple: return StructureSpec{Jungle_Temple, Dimension::Overworld};
    case LayerId::SwampHut: return StructureSpec{Swamp_Hut, Dimension::Overworld};
    case LayerId::Igloo: return StructureSpec{Igloo, Dimension::Overworld};
    case LayerId::WoodlandMansion: return StructureSpec{Mansion, Dimension::Overworld};
    case LayerId::OceanMonument: return StructureSpec{Monument, Dimension::Overworld};
    case LayerId::OceanRuin: return StructureSpec{Ocean_Ruin, Dimension::Overworld};
    case LayerId::Shipwreck: return StructureSpec{Shipwreck, Dimension::Overworld};
    case LayerId::BuriedTreasure: return StructureSpec{Treasure, Dimension::Overworld};
    case LayerId::RuinedPortal: return StructureSpec{Ruined_Portal, Dimension::Overworld};
    case LayerId::RuinedPortalNether: return StructureSpec{Ruined_Portal_N, Dimension::Nether};
    case LayerId::Stronghold: return StructureSpec{Stronghold, Dimension::Overworld};
    case LayerId::Mineshaft: return StructureSpec{Mineshaft, Dimension::Overworld};
    case LayerId::AncientCity: return StructureSpec{Ancient_City, Dimension::Overworld};
    case LayerId::TrailRuins: return StructureSpec{Trail_Ruins, Dimension::Overworld};
    case LayerId::TrialChambers: return StructureSpec{Trial_Chambers, Dimension::Overworld};
    case LayerId::AbandonedCamp: return StructureSpec{Abandoned_Camp, Dimension::Overworld};
    case LayerId::NetherFortress: return StructureSpec{Fortress, Dimension::Nether};
    case LayerId::BastionRemnant: return StructureSpec{Bastion, Dimension::Nether};
    case LayerId::EndCity: return StructureSpec{End_City, Dimension::End};
    default: return std::nullopt;
    }
}

struct BiomeSpec final {
    int       id = none;
    Dimension dimension = Dimension::Overworld;
    int       scale = 4;
    int       sampleY = 319 >> 2;
};

[[nodiscard]] constexpr BiomeSpec BiomeSamplingFor(Dimension dimension) noexcept {
    switch (dimension) {
    case Dimension::Overworld: return BiomeSpec{none, Dimension::Overworld, 4, 319 >> 2};
    case Dimension::Nether: return BiomeSpec{none, Dimension::Nether, 4, 33 >> 2};
    case Dimension::End: return BiomeSpec{none, Dimension::End, 16, 0};
    }
    return {};
}

[[nodiscard]] constexpr std::optional<BiomeSpec> ToRareBiomeSpec(LayerId layer) noexcept {
    switch (layer) {
    case LayerId::MushroomFields: return BiomeSpec{mushroom_fields, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::CherryGrove: return BiomeSpec{cherry_grove, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::Badlands: return BiomeSpec{badlands, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::IceSpikes: return BiomeSpec{ice_spikes, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::MangroveSwamp: return BiomeSpec{mangrove_swamp, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::PaleGarden: return BiomeSpec{pale_garden, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::SulfurCaves: return BiomeSpec{sulfur_caves, Dimension::Overworld, 4, 0};
    case LayerId::DappledForest: return BiomeSpec{dappled_forest, Dimension::Overworld, 4, 319 >> 2};
    case LayerId::SoulSandValley: return BiomeSpec{soul_sand_valley, Dimension::Nether, 4, 33 >> 2};
    case LayerId::CrimsonForest: return BiomeSpec{crimson_forest, Dimension::Nether, 4, 33 >> 2};
    case LayerId::WarpedForest: return BiomeSpec{warped_forest, Dimension::Nether, 4, 33 >> 2};
    case LayerId::BasaltDeltas: return BiomeSpec{basalt_deltas, Dimension::Nether, 4, 33 >> 2};
    case LayerId::EndSmallIslands: return BiomeSpec{small_end_islands, Dimension::End, 16, 0};
    case LayerId::EndMidlands: return BiomeSpec{end_midlands, Dimension::End, 16, 0};
    case LayerId::EndHighlands: return BiomeSpec{end_highlands, Dimension::End, 16, 0};
    case LayerId::EndBarrens: return BiomeSpec{end_barrens, Dimension::End, 16, 0};
    default: return std::nullopt;
    }
}

[[nodiscard]] constexpr std::int64_t FloorDivide(std::int64_t value, std::int64_t divisor) noexcept {
    const std::int64_t quotient = value / divisor;
    return quotient - (value % divisor < 0 ? 1 : 0);
}

inline constexpr std::size_t kMaximumBiomeSamplesPerQuery = 256U * 256U;

} // namespace

BedrockStructureFinder::BedrockStructureFinder(Dimension dimension, std::uint64_t seedBits) noexcept
: generator_(std::make_unique<Generator>()), dimension_(dimension), seedBits_(seedBits) {
    setupGenerator(generator_.get(), kCubiomesViewerBedrockVersion, 0);
    if (const auto cubiomesDimension = ToCubiomesDimension(dimension)) {
        applySeed(generator_.get(), *cubiomesDimension, seedBits);
    }
}

BedrockStructureFinder::~BedrockStructureFinder() = default;

std::optional<std::int32_t> BedrockStructureFinder::RegionSizeChunks(LayerId layer) const noexcept {
    const auto structure = ToCubiomesStructure(layer);
    if (!structure || structure->dimension != dimension_) return std::nullopt;

    StructureConfig config{};
    if (!getStructureConfig(structure->structure, kCubiomesViewerBedrockVersion, &config) || config.regionSize <= 0) {
        return std::nullopt;
    }
    return config.regionSize;
}

std::optional<BlockPosition> BedrockStructureFinder::FindStructurePosition(
    LayerId layer,
    std::int64_t regionX,
    std::int64_t regionZ
) const noexcept {
    const auto structure = ToCubiomesStructure(layer);
    if (!structure || structure->dimension != dimension_ || !IsIntCoordinate(regionX) || !IsIntCoordinate(regionZ)) {
        return std::nullopt;
    }

    Pos position{};
    if (!getStructurePos(structure->structure, kCubiomesViewerBedrockVersion, seedBits_,
                         static_cast<int>(regionX), static_cast<int>(regionZ), &position)) {
        return std::nullopt;
    }
    return BlockPosition{position.x, position.z};
}

bool BedrockStructureFinder::IsStructureViable(LayerId layer, BlockPosition anchor) noexcept {
    const auto structure = ToCubiomesStructure(layer);
    if (!structure || structure->dimension != dimension_ || !IsIntCoordinate(anchor.x) || !IsIntCoordinate(anchor.z)) {
        return false;
    }
    return isViableStructurePos(structure->structure, generator_.get(), static_cast<int>(anchor.x),
                                static_cast<int>(anchor.z), 0) != 0;
}

bool BedrockStructureFinder::IsSlimeChunk(ChunkPosition chunk) noexcept {
    if (!IsIntCoordinate(chunk.x) || !IsIntCoordinate(chunk.z)) return false;
    return isSlimeChunk(static_cast<int>(chunk.x), static_cast<int>(chunk.z)) != 0;
}

std::vector<BlockPosition> BedrockStructureFinder::FindRareBiomeAnchors(
    LayerId layer,
    BlockRect bounds
) const {
    const auto targetBiome = ToRareBiomeSpec(layer);
    if (!targetBiome || targetBiome->dimension != dimension_ || !bounds.IsValid()) return {};

    const std::int64_t minSampleX = FloorDivide(bounds.minX, targetBiome->scale);
    const std::int64_t minSampleZ = FloorDivide(bounds.minZ, targetBiome->scale);
    const std::int64_t maxSampleXExclusive = FloorDivide(bounds.maxXExclusive - 1, targetBiome->scale) + 1;
    const std::int64_t maxSampleZExclusive = FloorDivide(bounds.maxZExclusive - 1, targetBiome->scale) + 1;
    const std::int64_t width64 = maxSampleXExclusive - minSampleX;
    const std::int64_t height64 = maxSampleZExclusive - minSampleZ;
    if (minSampleX < std::numeric_limits<int>::min() || minSampleZ < std::numeric_limits<int>::min()
        || maxSampleXExclusive > std::numeric_limits<int>::max() || maxSampleZExclusive > std::numeric_limits<int>::max()
        || width64 <= 0 || height64 <= 0) {
        return {};
    }

    const std::size_t width = static_cast<std::size_t>(width64);
    const std::size_t height = static_cast<std::size_t>(height64);
    if (width > kMaximumBiomeSamplesPerQuery / height) return {};
    const std::size_t sampleCount = width * height;

    const Range range{
        targetBiome->scale,
        static_cast<int>(minSampleX),
        static_cast<int>(minSampleZ),
        static_cast<int>(width),
        static_cast<int>(height),
        targetBiome->sampleY,
        1,
    };
    int* rawBiomes = allocCache(generator_.get(), range);
    if (rawBiomes == nullptr) return {};
    std::unique_ptr<int, decltype(&std::free)> biomes(rawBiomes, &std::free);
    if (genBiomes(generator_.get(), biomes.get(), range) != 0) return {};

    std::vector<std::uint8_t> visited(sampleCount, 0);
    std::vector<std::size_t> queue;
    std::vector<BlockPosition> anchors;
    queue.reserve(sampleCount / 8U);

    for (std::size_t start = 0; start < sampleCount; ++start) {
        if (visited[start] || biomes.get()[start] != targetBiome->id) continue;

        visited[start] = 1;
        queue.clear();
        queue.push_back(start);
        for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
            const std::size_t index = queue[cursor];
            const std::size_t x = index % width;
            const std::size_t z = index / width;

            const auto visit = [&](std::size_t neighbor) {
                if (!visited[neighbor] && biomes.get()[neighbor] == targetBiome->id) {
                    visited[neighbor] = 1;
                    queue.push_back(neighbor);
                }
            };
            if (x > 0) visit(index - 1);
            if (x + 1 < width) visit(index + 1);
            if (z > 0) visit(index - width);
            if (z + 1 < height) visit(index + width);
        }

        // `start` is a known matching cell from the exact range produced by
        // cubiomes. Keeping that cell avoids changing scale-16 End semantics
        // by deriving a new location from a component centroid.
        const std::size_t sampleX = start % width;
        const std::size_t sampleZ = start / width;
        const BlockPosition anchor{
            (minSampleX + static_cast<std::int64_t>(sampleX)) * targetBiome->scale + targetBiome->scale / 2,
            (minSampleZ + static_cast<std::int64_t>(sampleZ)) * targetBiome->scale + targetBiome->scale / 2,
        };
        if (bounds.Contains(anchor)) anchors.push_back(anchor);
    }
    return anchors;
}

std::vector<BlockPosition> BedrockStructureFinder::FindEndGatewayAnchors(BlockRect bounds) const {
    if (dimension_ != Dimension::End || !bounds.IsValid()) return {};

    const std::int64_t minChunkX = FloorDivide(bounds.minX, 16);
    const std::int64_t minChunkZ = FloorDivide(bounds.minZ, 16);
    const std::int64_t maxChunkX = FloorDivide(bounds.maxXExclusive - 1, 16);
    const std::int64_t maxChunkZ = FloorDivide(bounds.maxZExclusive - 1, 16);
    if (!IsIntCoordinate(minChunkX) || !IsIntCoordinate(minChunkZ)
        || !IsIntCoordinate(maxChunkX) || !IsIntCoordinate(maxChunkZ)) {
        return {};
    }

    SurfaceNoise surfaceNoise{};
    initSurfaceNoise(&surfaceNoise, DIM_END, seedBits_);
    std::vector<BlockPosition> anchors;
    for (std::int64_t chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
        for (std::int64_t chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
            // Upstream squares these signed ints before testing the outer-End
            // radius. Do not pass coordinate pairs whose squared radius does
            // not fit its signed-int implementation.
            const std::int64_t squaredRadius = chunkX * chunkX + chunkZ * chunkZ;
            if (squaredRadius > std::numeric_limits<int>::max()) continue;
            Pos position{};
            if (!getEndGatewayPos(seedBits_, generator_->en, surfaceNoise, static_cast<int>(chunkX),
                                  static_cast<int>(chunkZ), &position)) {
                continue;
            }
            const BlockPosition anchor{position.x, position.z};
            if (bounds.Contains(anchor)) anchors.push_back(anchor);
        }
    }
    return anchors;
}

std::optional<int> BedrockStructureFinder::GetBiomeIdAt(BlockPosition position) const noexcept {
    if (!IsIntCoordinate(position.x) || !IsIntCoordinate(position.z)) return std::nullopt;
    const BiomeSpec sampling = BiomeSamplingFor(dimension_);
    const int biome = getBiomeAt(generator_.get(), sampling.scale,
                                  static_cast<int>(FloorDivide(position.x, sampling.scale)),
                                  sampling.sampleY,
                                  static_cast<int>(FloorDivide(position.z, sampling.scale)));
    return biome == none ? std::nullopt : std::optional<int>{biome};
}

} // namespace ChiyanMap::WorldGen
