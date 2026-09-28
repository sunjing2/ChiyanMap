#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ChiyanMap::WorldGen {

// Bit-exact Java java.util.Random-compatible 48-bit LCG. Structure placement
// uses Java's rejection-aware NextInt(), not std::uniform_int_distribution.
class Java48Random final {
public:
    static constexpr std::uint64_t Multiplier = 0x5DEECE66DULL;
    static constexpr std::uint64_t Addend     = 0xBULL;
    static constexpr std::uint64_t Mask       = (1ULL << 48U) - 1ULL;

    explicit constexpr Java48Random(std::uint64_t seed = 0) noexcept {
        SetSeed(seed);
    }

    constexpr void SetSeed(std::uint64_t seed) noexcept {
        state_ = (seed ^ Multiplier) & Mask;
    }

    [[nodiscard]] constexpr std::uint64_t State() const noexcept {
        return state_;
    }

    [[nodiscard]] std::uint32_t NextBits(unsigned int bits) noexcept;
    [[nodiscard]] std::optional<std::uint32_t> NextInt(std::uint32_t bound) noexcept;
    [[nodiscard]] std::uint64_t NextLong() noexcept;

private:
    std::uint64_t state_ = 0;
};

// A compact, local MT19937 implementation. Bedrock's legacy path consumes its
// raw 32-bit output with `% bound`; changing that to a distribution changes
// structure coordinates.
class BedrockMt19937 final {
public:
    explicit BedrockMt19937(std::uint32_t seed = 5489U) noexcept;

    void Seed(std::uint32_t seed) noexcept;
    [[nodiscard]] std::uint32_t NextRaw() noexcept;
    [[nodiscard]] std::optional<std::uint32_t> NextModulo(std::uint32_t bound) noexcept;

private:
    void Twist() noexcept;

    std::array<std::uint32_t, 624> state_{};
    std::size_t                     index_ = 624;
};

[[nodiscard]] constexpr std::optional<std::uint32_t> BedrockRawModulo(
    std::uint32_t raw,
    std::uint32_t bound
) noexcept {
    return bound == 0 ? std::nullopt : std::optional<std::uint32_t>{raw % bound};
}

// Region seed constants used by the legacy Bedrock grid placement path.
inline constexpr std::uint64_t kBedrockRegionXMultiplier = 341873128712ULL;
inline constexpr std::uint64_t kBedrockRegionZMultiplier = 132897987541ULL;

// All arithmetic intentionally wraps as unsigned integers before the low 32
// bits are used to seed MT19937. This preserves the behavior for negative and
// large 64-bit level seeds without signed-overflow UB.
[[nodiscard]] constexpr std::uint32_t MixBedrockRegionSeed32(
    std::uint64_t levelSeedBits,
    std::int64_t  regionX,
    std::int64_t  regionZ,
    std::uint32_t salt
) noexcept {
    const std::uint64_t mixed = levelSeedBits + static_cast<std::uint64_t>(regionX) * kBedrockRegionXMultiplier
                              + static_cast<std::uint64_t>(regionZ) * kBedrockRegionZMultiplier + salt;
    return static_cast<std::uint32_t>(mixed);
}

// Modern grid placement starts from the full level seed before Java's normal
// 48-bit seed mask is applied. This helper does not discard the high 32 bits.
[[nodiscard]] constexpr std::uint64_t MixJavaStructureSeed(
    std::uint64_t levelSeedBits,
    std::int64_t  regionX,
    std::int64_t  regionZ,
    std::uint32_t salt
) noexcept {
    return levelSeedBits + static_cast<std::uint64_t>(regionX) * kBedrockRegionXMultiplier
         + static_cast<std::uint64_t>(regionZ) * kBedrockRegionZMultiplier + salt;
}

} // namespace ChiyanMap::WorldGen
