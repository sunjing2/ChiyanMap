#ifndef mt_H_
#define mt_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t mt_next();
void mt_seed(uint32_t seed_value);

typedef struct MTRngState
{
    uint32_t MT[624];
    size_t index;
    size_t index_fast;
    uint64_t ws;
    int a;
    int b;
} MTRngState;

void mt_get_state(MTRngState *out);
void mt_set_state(const MTRngState *in);

#ifdef _MSC_VER
extern __declspec(thread) uint64_t mt_ws;
extern __declspec(thread) int mt_a;
extern __declspec(thread) int mt_b;
#else
extern __thread uint64_t mt_ws;
extern __thread int mt_a;
extern __thread int mt_b;
#endif

static inline void setSeed(uint32_t value)
{
    mt_seed(value);
}

static inline int next()
{
    return (int)(mt_next() >> 1);
}

static inline int nextInt(const int n)
{
    return (int)(mt_next() % n);
}

static inline int nextIntRange(const int a, const int b)
{
    return (a < b) ? (a + nextInt(b - a)) : a;
}

static inline int nextBoolean()
{
    return (mt_next() & 0x8000000) != 0;
}

static inline double nextDouble()
{
    return mt_next() * (1.0 / 4294967296.0);
}

static inline float nextFloat()
{
    return (float)nextDouble();
}

static inline void skipNextN(int n)
{
    for (int i = 0; i < n; i++) mt_next();
}

//==============================================================================
// ChunkRandom
//==============================================================================

#define REGION_A 341873128712L
#define REGION_B 132897987541L

static inline int32_t setTerrainSeed(int chunkX, int chunkZ)
{
    uint32_t seed = (chunkX * REGION_A + chunkZ * REGION_B);
    setSeed(seed);
    return seed;
}

static inline uint32_t setPopulationSeed(uint64_t worldSeed, int chunkX, int chunkZ)
{
    if (mt_ws != worldSeed)
    {
        setSeed(worldSeed);
        mt_a = next();
        mt_b = next();
        mt_ws = worldSeed;
    }
    uint32_t seed = (chunkX * (mt_a | 1u) + chunkZ * (mt_b | 1u)) ^ worldSeed;
    setSeed(seed);
    return seed;
}

static inline uint32_t setDecorationSeed(uint64_t worldSeed, int chunkX, int chunkZ, int salt)
{
    if (mt_ws != worldSeed)
    {
        setSeed(worldSeed);
        mt_a = next();
        mt_b = next();
        mt_ws = worldSeed;
    }
    uint32_t seed = (chunkX * (mt_a | 1u) + chunkZ * (mt_b | 1u)) ^ worldSeed;
    seed = ((seed >> 2) + (seed << 6) + salt - 1640531527u) ^ seed;
    setSeed(seed);
    return seed;
}

static inline uint32_t setCarverSeed(uint64_t worldSeed, int chunkX, int chunkZ)
{
    if (mt_ws != worldSeed)
    {
        setSeed(worldSeed);
        mt_a = next();
        mt_b = next();
        mt_ws = worldSeed;
    }
    uint32_t seed = (chunkX * mt_a) ^ (chunkZ * mt_b) ^ worldSeed;
    setSeed(seed);
    return seed;
}

static inline uint32_t setRegionSeed(uint64_t worldSeed, int regX, int regZ, int salt)
{
    uint32_t seed = (regX * REGION_A) + (regZ * REGION_B) + worldSeed + salt;
    setSeed(seed);
    return seed;
}

static inline uint32_t setFortressSeed(uint64_t worldSeed, int chunkX, int chunkZ)
{
    uint32_t seed = ((chunkX >> 4) ^ ((chunkZ >> 4) << 4)) ^ worldSeed;
    setSeed(seed);
    return seed;
}

static inline uint32_t seedSlimeChunk(int chunkX, int chunkZ)
{
    uint32_t seed = (chunkX * 522133279U) ^ chunkZ;
    setSeed(seed);
    return seed;
}

#ifdef __cplusplus
}
#endif

#endif
