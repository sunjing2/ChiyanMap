#pragma once

#include "worldgen/WorldGenTypes.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ChiyanMap::WorldGen {

struct LayerDescriptor final {
    LayerId           id{};
    std::string_view  displayName;
    Dimension         dimension{};
    LayerSupportState support{};
    PlacementAlgorithm algorithm{};
    std::string_view  detail;
};

// Counts candidates that fall inside a query's half-open bounds. These make
// biome filtering observable without exposing generator internals to the UI.
struct LayerQueryStatistics final {
    std::size_t placementCandidates = 0;
    std::size_t biomeRejectedCandidates = 0;
    std::size_t emittedMarkers = 0;
};

struct LayerQueryResult final {
    QueryStatus                    status = QueryStatus::Ok;
    LayerSupportState              support = LayerSupportState::Unavailable;
    std::vector<SeedMapMarker>    markers;
    LayerQueryStatistics           statistics;
};

[[nodiscard]] std::span<const LayerDescriptor> LayerCatalog() noexcept;
[[nodiscard]] const LayerDescriptor* FindLayerDescriptor(LayerId layer) noexcept;

// Queries fixture-verified entries and cubiomes 26.51 reference entries. All
// output is a copied-seed prediction: no loaded chunk or client game object is
// read. Terrain-sensitive structures remain explicitly disclosed as such.
[[nodiscard]] LayerQueryResult QueryPlacementCandidates(const LayerQuery& query);

} // namespace ChiyanMap::WorldGen
