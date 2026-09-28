#include "worldgen/StructureCatalog.h"

#include "worldgen/BedrockBiomeValidator.h"
#include <array>
#include <limits>

namespace ChiyanMap::WorldGen {
namespace {

constexpr std::array kLayers{
    LayerDescriptor{LayerId::Village, "Village", Dimension::Overworld, LayerSupportState::FixtureVerified,
                    PlacementAlgorithm::CubiomesFinder,
                    "cubiomes-viewer-bedrock 26.51 placement and biome eligibility are enabled."},
    LayerDescriptor{LayerId::PillagerOutpost, "Pillager Outpost", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::DesertPyramid, "Desert Pyramid", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "Seed prediction: upstream cannot verify final surface height."},
    LayerDescriptor{LayerId::JungleTemple, "Jungle Temple", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "Seed prediction: upstream cannot verify final surface height."},
    LayerDescriptor{LayerId::SwampHut, "Swamp Hut", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::Igloo, "Igloo", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::WoodlandMansion, "Woodland Mansion", Dimension::Overworld,
                    LayerSupportState::CubiomesReference, PlacementAlgorithm::CubiomesFinder,
                    "Seed prediction: upstream cannot verify final surface height."},
    LayerDescriptor{LayerId::OceanMonument, "Ocean Monument", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::OceanRuin, "Ocean Ruin", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::Shipwreck, "Shipwreck", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::BuriedTreasure, "Buried Treasure", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::RuinedPortal, "Ruined Portal", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::RuinedPortalNether, "Ruined Portal", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::Stronghold, "Stronghold", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock static stronghold rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::Mineshaft, "Mineshaft", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock chunk finder awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::AncientCity, "Ancient City", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::TrailRuins, "Trail Ruins", Dimension::Overworld,
                    LayerSupportState::CubiomesReference, PlacementAlgorithm::CubiomesFinder,
                    "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::TrialChambers, "Trial Chambers", Dimension::Overworld,
                    LayerSupportState::CubiomesReference, PlacementAlgorithm::CubiomesFinder,
                    "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::AbandonedCamp, "Abandoned Camp", Dimension::Overworld,
                    LayerSupportState::CubiomesReference, PlacementAlgorithm::CubiomesFinder,
                    "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::NetherFortress, "Nether Fortress", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::BastionRemnant, "Bastion Remnant", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::NetherFossil, "Nether Fossil", Dimension::Nether, LayerSupportState::Unavailable,
                    PlacementAlgorithm::None, "No verified Bedrock 26.51 placement fixture."},
    LayerDescriptor{LayerId::EndCity, "End City", Dimension::End, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder, "cubiomes-viewer-bedrock 26.51 rule awaits a Bedrock fixture."},
    LayerDescriptor{LayerId::EndGateway, "End Gateway", Dimension::End, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::CubiomesFinder,
                    "Seed prediction for naturally generated outer-end gateways; fixed dragon gateways are event-driven."},
    LayerDescriptor{LayerId::OverworldBiomes, "Overworld Biomes", Dimension::Overworld, LayerSupportState::Unavailable,
                    PlacementAlgorithm::BiomeNoise, "Biome noise model has no approved Bedrock 26.51 fixture."},
    LayerDescriptor{LayerId::NetherBiomes, "Nether Biomes", Dimension::Nether, LayerSupportState::Unavailable,
                    PlacementAlgorithm::BiomeNoise, "Biome noise model has no approved Bedrock 26.51 fixture."},
    LayerDescriptor{LayerId::EndBiomes, "End Biomes", Dimension::End, LayerSupportState::Unavailable,
                    PlacementAlgorithm::BiomeNoise, "Biome noise model has no approved Bedrock 26.51 fixture."},
    LayerDescriptor{LayerId::SlimeChunks, "Slime Chunks", Dimension::Overworld, LayerSupportState::FixtureVerified,
                    PlacementAlgorithm::ChunkPopulation,
                    "Bedrock 26.51 cubiomes isSlimeChunk rule; full 16x16 chunk cells are shown."},
    LayerDescriptor{LayerId::OverworldOres, "Overworld Ores", Dimension::Overworld, LayerSupportState::Unavailable,
                    PlacementAlgorithm::ChunkPopulation, "Ore placement requires a verified chunk-population model."},
    LayerDescriptor{LayerId::NetherOres, "Nether Ores", Dimension::Nether, LayerSupportState::Unavailable,
                    PlacementAlgorithm::ChunkPopulation, "Ore placement requires a verified chunk-population model."},
    LayerDescriptor{LayerId::OverworldUndergroundFeatures, "Underground Features", Dimension::Overworld,
                    LayerSupportState::Unavailable, PlacementAlgorithm::ChunkPopulation,
                    "Underground feature placement requires a verified terrain and population model."},
    LayerDescriptor{LayerId::DungeonSpawner, "Dungeon / Spawner", Dimension::Overworld, LayerSupportState::Unavailable,
                    PlacementAlgorithm::ChunkPopulation, "Dungeon placement requires a verified terrain and population model."},
    LayerDescriptor{LayerId::WorldSpawn, "World Spawn", Dimension::Overworld, LayerSupportState::Unavailable,
                    PlacementAlgorithm::SpawnSearch, "World-spawn search requires a verified terrain and surface model."},
    LayerDescriptor{LayerId::MushroomFields, "Mushroom Fields", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::CherryGrove, "Cherry Grove", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::Badlands, "Badlands", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::IceSpikes, "Ice Spikes", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::MangroveSwamp, "Mangrove Swamp", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::PaleGarden, "Pale Garden", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::SulfurCaves, "Sulfur Caves", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::DappledForest, "Dappled Forest", Dimension::Overworld, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 rare-biome region prediction."},
    LayerDescriptor{LayerId::SoulSandValley, "Soul Sand Valley", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 Nether-biome region prediction."},
    LayerDescriptor{LayerId::CrimsonForest, "Crimson Forest", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 Nether-biome region prediction."},
    LayerDescriptor{LayerId::WarpedForest, "Warped Forest", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 Nether-biome region prediction."},
    LayerDescriptor{LayerId::BasaltDeltas, "Basalt Deltas", Dimension::Nether, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 Nether-biome region prediction."},
    LayerDescriptor{LayerId::EndSmallIslands, "Small End Islands", Dimension::End, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 End-biome region prediction."},
    LayerDescriptor{LayerId::EndMidlands, "End Midlands", Dimension::End, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 End-biome region prediction."},
    LayerDescriptor{LayerId::EndHighlands, "End Highlands", Dimension::End, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 End-biome region prediction."},
    LayerDescriptor{LayerId::EndBarrens, "End Barrens", Dimension::End, LayerSupportState::CubiomesReference,
                    PlacementAlgorithm::BiomeNoise, "Seed-only cubiomes 26.51 End-biome region prediction."},
};

[[nodiscard]] constexpr std::int64_t FloorDivide(std::int64_t value, std::int64_t divisor) noexcept {
    const std::int64_t quotient  = value / divisor;
    const std::int64_t remainder = value % divisor;
    return quotient - (remainder < 0 ? 1 : 0);
}

} // namespace

std::span<const LayerDescriptor> LayerCatalog() noexcept {
    return kLayers;
}

const LayerDescriptor* FindLayerDescriptor(LayerId layer) noexcept {
    for (const LayerDescriptor& descriptor : kLayers) {
        if (descriptor.id == layer) return &descriptor;
    }
    return nullptr;
}

LayerQueryResult QueryPlacementCandidates(const LayerQuery& query) {
    const LayerDescriptor* descriptor = FindLayerDescriptor(query.Layer());
    if (descriptor == nullptr) return {QueryStatus::LayerUnavailable, LayerSupportState::Unavailable, {}};

    if (!query.Seed().HasSeed()) return {QueryStatus::SeedUnavailable, descriptor->support, {}};
    if (!query.Bounds().IsValid()) return {QueryStatus::InvalidBounds, descriptor->support, {}};
    if (descriptor->dimension != query.TargetDimension()) {
        return {QueryStatus::DimensionMismatch, descriptor->support, {}};
    }
    if (!IsMarkerEmissionAllowed(descriptor->support)) {
        return {QueryStatus::LayerUnavailable, descriptor->support, {}};
    }

    // The generator only consumes the copied seed value. No level, chunk, or
    // client object crosses into this pure seed-map query.
    const auto seedBits = query.Seed().TrySeedBits();
    if (!seedBits) return {QueryStatus::SeedUnavailable, descriptor->support, {}};

    LayerQueryResult result;
    result.support = descriptor->support;
    if (query.Layer() == LayerId::SlimeChunks) {
        const std::int64_t minChunkX = FloorDivide(query.Bounds().minX, 16);
        const std::int64_t minChunkZ = FloorDivide(query.Bounds().minZ, 16);
        const std::int64_t maxChunkX = FloorDivide(query.Bounds().maxXExclusive - 1, 16);
        const std::int64_t maxChunkZ = FloorDivide(query.Bounds().maxZExclusive - 1, 16);
        constexpr std::int64_t minCubiomesChunk = std::numeric_limits<int>::min();
        constexpr std::int64_t maxCubiomesChunk = std::numeric_limits<int>::max();
        if (minChunkX < minCubiomesChunk || minChunkZ < minCubiomesChunk
            || maxChunkX > maxCubiomesChunk || maxChunkZ > maxCubiomesChunk) {
            return {QueryStatus::CoordinateOverflow, descriptor->support, {}};
        }

        for (std::int64_t chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
            for (std::int64_t chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
                const ChunkPosition chunk{chunkX, chunkZ};
                if (!Bedrock2620StructureFinder::IsSlimeChunk(chunk)) continue;

                ++result.statistics.placementCandidates;
                result.markers.emplace_back(
                    query.Layer(),
                    query.TargetDimension(),
                    chunk,
                    BlockPosition{chunkX * 16 + 8, chunkZ * 16 + 8},
                    MarkerKind::ChunkRegion
                );
                ++result.statistics.emittedMarkers;
            }
        }
        return result;
    }

    Bedrock2620StructureFinder structureFinder{query.TargetDimension(), *seedBits};
    if (descriptor->algorithm == PlacementAlgorithm::BiomeNoise) {
        const auto anchors = structureFinder.FindRareBiomeAnchors(query.Layer(), query.Bounds());
        for (const BlockPosition anchor : anchors) {
            ++result.statistics.placementCandidates;
            result.markers.emplace_back(
                query.Layer(),
                query.TargetDimension(),
                ChunkPosition{FloorDivide(anchor.x, 16), FloorDivide(anchor.z, 16)},
                anchor,
                MarkerKind::BiomeRegion
            );
            ++result.statistics.emittedMarkers;
        }
        return result;
    }

    if (query.Layer() == LayerId::EndGateway) {
        const auto anchors = structureFinder.FindEndGatewayAnchors(query.Bounds());
        for (const BlockPosition anchor : anchors) {
            ++result.statistics.placementCandidates;
            result.markers.emplace_back(
                query.Layer(),
                query.TargetDimension(),
                ChunkPosition{FloorDivide(anchor.x, 16), FloorDivide(anchor.z, 16)},
                anchor
            );
            ++result.statistics.emittedMarkers;
        }
        return result;
    }

    const auto regionSizeChunks = structureFinder.RegionSizeChunks(query.Layer());
    if (!regionSizeChunks) return {QueryStatus::LayerUnavailable, descriptor->support, {}};

    // A candidate's anchor always lies inside its chunk. Therefore the regions
    // containing the first and last covered chunks are sufficient; the
    // half-open block filter below removes candidates outside a partial tile.
    const std::int64_t minChunkX = FloorDivide(query.Bounds().minX, 16);
    const std::int64_t minChunkZ = FloorDivide(query.Bounds().minZ, 16);
    const std::int64_t maxChunkX = FloorDivide(query.Bounds().maxXExclusive - 1, 16);
    const std::int64_t maxChunkZ = FloorDivide(query.Bounds().maxZExclusive - 1, 16);
    const std::int64_t minRegionX = FloorDivide(minChunkX, *regionSizeChunks);
    const std::int64_t minRegionZ = FloorDivide(minChunkZ, *regionSizeChunks);
    const std::int64_t maxRegionX = FloorDivide(maxChunkX, *regionSizeChunks);
    const std::int64_t maxRegionZ = FloorDivide(maxChunkZ, *regionSizeChunks);

    for (std::int64_t regionZ = minRegionZ; regionZ <= maxRegionZ; ++regionZ) {
        for (std::int64_t regionX = minRegionX; regionX <= maxRegionX; ++regionX) {
            const auto anchor = structureFinder.FindStructurePosition(query.Layer(), regionX, regionZ);
            if (!anchor) continue;
            if (!query.Bounds().Contains(*anchor)) {
                continue;
            }

            ++result.statistics.placementCandidates;
            if (!structureFinder.IsStructureViable(query.Layer(), *anchor)) {
                ++result.statistics.biomeRejectedCandidates;
                continue;
            }
            result.markers.emplace_back(
                query.Layer(),
                query.TargetDimension(),
                ChunkPosition{FloorDivide(anchor->x, 16), FloorDivide(anchor->z, 16)},
                *anchor
            );
            ++result.statistics.emittedMarkers;
        }
    }

    return result;
}

} // namespace ChiyanMap::WorldGen
