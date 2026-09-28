#pragma once

#include "worldgen/WorldGenTypes.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

struct Generator;

namespace ChiyanMap::WorldGen {

// Wraps the structure APIs used by cubiomes-viewer-bedrock. It owns no
// Minecraft objects and can therefore be constructed on a seed-map worker.
class BedrockStructureFinder final {
public:
    explicit BedrockStructureFinder(std::uint64_t seedBits) noexcept
    : BedrockStructureFinder(Dimension::Overworld, seedBits) {}
    explicit BedrockStructureFinder(Dimension dimension, std::uint64_t seedBits) noexcept;
    ~BedrockStructureFinder();

    BedrockStructureFinder(const BedrockStructureFinder&)            = delete;
    BedrockStructureFinder(BedrockStructureFinder&&)                 = delete;
    BedrockStructureFinder& operator=(const BedrockStructureFinder&) = delete;
    BedrockStructureFinder& operator=(BedrockStructureFinder&&)      = delete;

    [[nodiscard]] std::optional<std::int32_t> RegionSizeChunks(LayerId layer) const noexcept;
    [[nodiscard]] std::optional<BlockPosition> FindStructurePosition(
        LayerId layer,
        std::int64_t regionX,
        std::int64_t regionZ
    ) const noexcept;
    [[nodiscard]] bool IsStructureViable(LayerId layer, BlockPosition anchor) noexcept;
    // `isSlimeChunk` is a Bedrock 26.20 cubiomes rule. It is intentionally
    // seed-independent, but remains a pure query with no client-world reads.
    [[nodiscard]] static bool IsSlimeChunk(ChunkPosition chunk) noexcept;
    [[nodiscard]] std::vector<BlockPosition> FindRareBiomeAnchors(LayerId layer, BlockRect bounds) const;
    [[nodiscard]] std::vector<BlockPosition> FindEndGatewayAnchors(BlockRect bounds) const;
    [[nodiscard]] std::optional<int> GetBiomeIdAt(BlockPosition position) const noexcept;

    [[nodiscard]] std::optional<BlockPosition> FindVillagePosition(
        std::int64_t regionX,
        std::int64_t regionZ
    ) const noexcept {
        return FindStructurePosition(LayerId::Village, regionX, regionZ);
    }
    [[nodiscard]] bool IsVillageViable(BlockPosition anchor) noexcept {
        return IsStructureViable(LayerId::Village, anchor);
    }

private:
    std::unique_ptr<Generator> generator_;
    Dimension                   dimension_ = Dimension::Overworld;
    std::uint64_t               seedBits_ = 0;
};

using Bedrock2620StructureFinder = BedrockStructureFinder;
using Bedrock2651StructureFinder = BedrockStructureFinder;

} // namespace ChiyanMap::WorldGen
