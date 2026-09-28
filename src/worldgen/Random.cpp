#include "worldgen/Random.h"

#include <limits>

namespace ChiyanMap::WorldGen {

std::uint32_t Java48Random::NextBits(unsigned int bits) noexcept {
    if (bits == 0 || bits > 32) return 0;

    state_ = (state_ * Multiplier + Addend) & Mask;
    return static_cast<std::uint32_t>(state_ >> (48U - bits));
}

std::optional<std::uint32_t> Java48Random::NextInt(std::uint32_t bound) noexcept {
    // java.util.Random accepts a positive signed int bound only.
    if (bound == 0 || bound > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
        return std::nullopt;
    }

    const std::uint32_t mask = bound - 1U;
    if ((bound & mask) == 0U) {
        return static_cast<std::uint32_t>((static_cast<std::uint64_t>(bound) * NextBits(31)) >> 31U);
    }

    for (;;) {
        const std::uint32_t bits  = NextBits(31);
        const std::uint32_t value = bits % bound;

        // This is Java's `bits - value + (bound - 1) >= 0` rejection
        // condition, expressed without relying on signed overflow.
        const std::uint64_t sum = static_cast<std::uint64_t>(bits) - value + mask;
        if (sum <= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) return value;
    }
}

std::uint64_t Java48Random::NextLong() noexcept {
    return (static_cast<std::uint64_t>(NextBits(32)) << 32U) | NextBits(32);
}

BedrockMt19937::BedrockMt19937(std::uint32_t seed) noexcept {
    Seed(seed);
}

void BedrockMt19937::Seed(std::uint32_t seed) noexcept {
    state_[0] = seed;
    for (std::size_t i = 1; i < state_.size(); ++i) {
        const std::uint32_t previous = state_[i - 1];
        state_[i] = 1812433253U * (previous ^ (previous >> 30U)) + static_cast<std::uint32_t>(i);
    }
    index_ = state_.size();
}

void BedrockMt19937::Twist() noexcept {
    constexpr std::uint32_t upperMask = 0x80000000U;
    constexpr std::uint32_t lowerMask = 0x7FFFFFFFU;
    constexpr std::uint32_t matrixA   = 0x9908B0DFU;

    for (std::size_t i = 0; i < state_.size(); ++i) {
        const std::uint32_t combined = (state_[i] & upperMask) | (state_[(i + 1U) % state_.size()] & lowerMask);
        state_[i] = state_[(i + 397U) % state_.size()] ^ (combined >> 1U)
                  ^ ((combined & 1U) == 0U ? 0U : matrixA);
    }
    index_ = 0;
}

std::uint32_t BedrockMt19937::NextRaw() noexcept {
    if (index_ >= state_.size()) Twist();

    std::uint32_t value = state_[index_++];
    value ^= value >> 11U;
    value ^= (value << 7U) & 0x9D2C5680U;
    value ^= (value << 15U) & 0xEFC60000U;
    value ^= value >> 18U;
    return value;
}

std::optional<std::uint32_t> BedrockMt19937::NextModulo(std::uint32_t bound) noexcept {
    return BedrockRawModulo(NextRaw(), bound);
}

} // namespace ChiyanMap::WorldGen
