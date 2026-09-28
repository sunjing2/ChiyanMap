#ifndef TERRAIN_H_
#define TERRAIN_H_

#include "biomenoise.h"

#ifdef __cplusplus
extern "C"
{
#endif

float getSpline(const Spline *sp, const float *vals);

STRUCT(TerrainNoise)
{
    SurfaceNoise sn; // octmin/octmax/octmain reused as lower/upper/interp
};

STRUCT(TerrainShaper)
{
    SplineStack ssFactor;
    Spline *spFactor;
};

/**
 * Initialize 3D terrain noise from "minecraft:terrain" key derivation.
 */
void initTerrainNoise(TerrainNoise *tn, uint64_t seed);

/**
 * Build the factor spline tree (seed-independent, fixed structure).
 */
void initTerrainShaper(TerrainShaper *ts);

/**
 * Sample offset and factor at biome scale (1:4).
 * bn must have setBiomeSeed() applied.
 */
void sampleTerrainShape(
    const BiomeNoise    *bn,
    const TerrainShaper *ts,
    int biomeX, int biomeZ,
    double *offset,
    double *factor);

/**
 * Density at block (x,y,z); caves/aquifers not included.
 * >0 means solid.
 */
double sampleTerrainDensity(
    const TerrainNoise *tn,
    double offset,
    double factor,
    int x, int y, int z);

/**
 * Cave noise (1.18+ overworld): cave entrance, cheese caves, spaghetti 2D/3D
 * tunnels, pillars, and noodle caves. Aquifers are not modelled here.
 */
STRUCT(CaveNoise)
{
    DoublePerlinNoise caveEntrance;
    DoublePerlinNoise caveLayer;
    DoublePerlinNoise caveCheese;
    DoublePerlinNoise spaghetti2d;
    DoublePerlinNoise spaghetti2dElev;
    DoublePerlinNoise spaghetti2dMod;
    DoublePerlinNoise spaghetti2dThick;
    DoublePerlinNoise spaghetti3d1;
    DoublePerlinNoise spaghetti3d2;
    DoublePerlinNoise spaghetti3dRarity;
    DoublePerlinNoise spaghetti3dThick;
    DoublePerlinNoise spaghettiRough;
    DoublePerlinNoise spaghettiRoughMod;
    DoublePerlinNoise pillar;
    DoublePerlinNoise pillarRareness;
    DoublePerlinNoise pillarThickness;
    DoublePerlinNoise noodle;
    DoublePerlinNoise noodleThickness;
    DoublePerlinNoise noodleRidgeA;
    DoublePerlinNoise noodleRidgeB;
    PerlinNoise oct[58]; // buffer for octaves
};

STRUCT(TerrainCacheEntry)
{
    int x, y, z;
    unsigned char used;
    double val;
};

STRUCT(TerrainCache)
{
    TerrainCacheEntry density[32768];
    TerrainCacheEntry noodle[4][16384];
};

/**
 * Initialize all cave-related noises from the world seed. Keys are derived
 * the same way as initTerrainNoise() (md5("minecraft:<key>") XORed onto the
 * first two Xoroshiro draws from the world seed).
 */
void initCaveNoise(CaveNoise *cn, uint64_t seed);

/**
 * Full 3D density at block (x,y,z), including terrain shape and caves before
 * the final noodle-cave block-state pass. >0 generally means solid, <=0 means
 * air/cave. Does not include aquifers, so cave floors below the fluid level
 * will render as air rather than water/lava.
 */
double sampleTerrainDensityCaves(
    const TerrainNoise *tn,
    const CaveNoise    *cn,
    double offset,
    double factor,
    int x, int y, int z);

int isSolid(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    int x, int y, int z);

int isAir(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    int x, int y, int z);

void initTerrainCache(TerrainCache *tc);

int getCaveBlockStateCached(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    TerrainCache        *tc,
    int x, int y, int z);

int isSolidCached(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    TerrainCache        *tc,
    int x, int y, int z);

#ifdef __cplusplus
}
#endif

#endif /* TERRAIN_H_ */
