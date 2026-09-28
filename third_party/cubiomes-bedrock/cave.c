#include "cave.h"

#include <math.h>
#include <string.h>

enum {
    TS_CONTINENTALNESS,
    TS_EROSION,
    TS_RIDGES,
    TS_WEIRDNESS
};

static void xOctaveInitLegacy(
    OctaveNoise *noise,
    Xoroshiro   *xr,
    PerlinNoise *octaves,
    int omin, int len)
{
    int i;
    int end = omin + len - 1;
    double persist = 1.0 / ((1LL << len) - 1.0);
    double lacuna = pow(2.0, end);

    if (end == 0)
    {
        xPerlinInit(&octaves[0], xr);
        octaves[0].amplitude = persist;
        octaves[0].lacunarity = lacuna;
        persist *= 2.0;
        lacuna *= 0.5;
        i = 1;
    }
    else
    {
        i = 0;
    }

    for (; i < len; i++)
    {
        xPerlinInit(&octaves[i], xr);
        octaves[i].amplitude = persist;
        octaves[i].lacunarity = lacuna;
        persist *= 2.0;
        lacuna *= 0.5;
    }

    noise->octaves = octaves;
    noise->octcnt = len;
}

void initTerrainNoise(TerrainNoise *tn, uint64_t seed)
{
    Xoroshiro pxr;
    xSetSeed(&pxr, seed);
    uint64_t xlo = xNextLong(&pxr);
    uint64_t xhi = xNextLong(&pxr);

    // "minecraft:terrain"
    pxr.lo = xlo ^ 0x1ee555222ef96f14ULL;
    pxr.hi = xhi ^ 0xe2bedfdbebe43d33ULL;

    SurfaceNoise *sn = &tn->sn;
    xOctaveInitLegacy(&sn->octmin,  &pxr, sn->oct+0,  -15, 16);
    xOctaveInitLegacy(&sn->octmax,  &pxr, sn->oct+16, -15, 16);
    xOctaveInitLegacy(&sn->octmain, &pxr, sn->oct+32,  -7,  8);

    sn->xzScale  = 1.0;
    sn->yScale   = 1.0;
    sn->xzFactor = 80;
    sn->yFactor  = 160;
}

static void addSplineVal(SplineStack *ss, Spline *sp, float loc, Spline *val, float der)
{
    (void) ss;
    sp->loc[sp->len] = loc;
    sp->val[sp->len] = val;
    sp->der[sp->len] = der;
    sp->len++;
}

static Spline *createFixSpline(SplineStack *ss, float val)
{
    FixSpline *sp = &ss->fstack[ss->flen++];
    sp->len = 1;
    sp->val = val;
    return (Spline*) sp;
}

static Spline *buildErosionFactorSpline(SplineStack *ss, float f, int bl)
{
    Spline *baseW = &ss->stack[ss->len++];
    baseW->typ = TS_WEIRDNESS;
    baseW->len = 0;
    addSplineVal(ss, baseW, -0.2f, createFixSpline(ss, 6.30f), 0.0f);
    addSplineVal(ss, baseW,  0.2f, createFixSpline(ss, f),     0.0f);

    Spline *w1 = &ss->stack[ss->len++];
    w1->typ = TS_WEIRDNESS;
    w1->len = 0;
    addSplineVal(ss, w1, -0.05f, createFixSpline(ss, 6.30f), 0.0f);
    addSplineVal(ss, w1,  0.05f, createFixSpline(ss, 2.67f), 0.0f);

    Spline *w2 = &ss->stack[ss->len++];
    w2->typ = TS_WEIRDNESS;
    w2->len = 0;
    addSplineVal(ss, w2, -0.05f, createFixSpline(ss, 2.67f), 0.0f);
    addSplineVal(ss, w2,  0.05f, createFixSpline(ss, 6.30f), 0.0f);

    Spline *sp = &ss->stack[ss->len++];
    sp->typ = TS_EROSION;
    sp->len = 0;
    addSplineVal(ss, sp, -0.60f, baseW, 0.0f);
    addSplineVal(ss, sp, -0.50f, w1,    0.0f);
    addSplineVal(ss, sp, -0.35f, baseW, 0.0f);
    addSplineVal(ss, sp, -0.25f, baseW, 0.0f);
    addSplineVal(ss, sp, -0.10f, w2,    0.0f);
    addSplineVal(ss, sp,  0.03f, baseW, 0.0f);

    if (bl)
    {
        Spline *wMid = &ss->stack[ss->len++];
        wMid->typ = TS_WEIRDNESS;
        wMid->len = 0;
        addSplineVal(ss, wMid, 0.0f, createFixSpline(ss, f),      0.0f);
        addSplineVal(ss, wMid, 0.1f, createFixSpline(ss, 0.625f), 0.0f);

        Spline *ridgeHigh = &ss->stack[ss->len++];
        ridgeHigh->typ = TS_RIDGES;
        ridgeHigh->len = 0;
        addSplineVal(ss, ridgeHigh, -0.90f, createFixSpline(ss, f), 0.0f);
        addSplineVal(ss, ridgeHigh, -0.69f, wMid,                   0.0f);

        addSplineVal(ss, sp, 0.35f, createFixSpline(ss, f), 0.0f);
        addSplineVal(ss, sp, 0.45f, ridgeHigh,              0.0f);
        addSplineVal(ss, sp, 0.55f, ridgeHigh,              0.0f);
        addSplineVal(ss, sp, 0.62f, createFixSpline(ss, f), 0.0f);
    }
    else
    {
        Spline *ridgeLow = &ss->stack[ss->len++];
        ridgeLow->typ = TS_RIDGES;
        ridgeLow->len = 0;
        addSplineVal(ss, ridgeLow, -0.70f, baseW,                      0.0f);
        addSplineVal(ss, ridgeLow, -0.15f, createFixSpline(ss, 1.37f), 0.0f);

        Spline *ridgeMid = &ss->stack[ss->len++];
        ridgeMid->typ = TS_RIDGES;
        ridgeMid->len = 0;
        addSplineVal(ss, ridgeMid, 0.45f, baseW,                      0.0f);
        addSplineVal(ss, ridgeMid, 0.70f, createFixSpline(ss, 1.56f), 0.0f);

        addSplineVal(ss, sp, 0.05f, ridgeMid,               0.0f);
        addSplineVal(ss, sp, 0.40f, ridgeMid,               0.0f);
        addSplineVal(ss, sp, 0.45f, ridgeLow,               0.0f);
        addSplineVal(ss, sp, 0.55f, ridgeLow,               0.0f);
        addSplineVal(ss, sp, 0.58f, createFixSpline(ss, f), 0.0f);
    }
    return sp;
}

void initTerrainShaper(TerrainShaper *ts)
{
    memset(&ts->ssFactor, 0, sizeof(ts->ssFactor));
    SplineStack *ss = &ts->ssFactor;

    Spline *sp = &ss->stack[ss->len++];
    sp->typ = TS_CONTINENTALNESS;
    sp->len = 0;
    addSplineVal(ss, sp, -0.19f, createFixSpline(ss, 3.95f),             0.0f);
    addSplineVal(ss, sp, -0.15f, buildErosionFactorSpline(ss, 6.25f, 1), 0.0f);
    addSplineVal(ss, sp, -0.10f, buildErosionFactorSpline(ss, 5.47f, 1), 0.0f);
    addSplineVal(ss, sp,  0.03f, buildErosionFactorSpline(ss, 5.08f, 1), 0.0f);
    addSplineVal(ss, sp,  0.06f, buildErosionFactorSpline(ss, 4.69f, 0), 0.0f);

    ts->spFactor = sp;
}

void sampleTerrainShape(
    const BiomeNoise    *bn,
    const TerrainShaper *ts,
    int biomeX, int biomeZ,
    double *offset,
    double *factor)
{
    double px = biomeX + sampleDoublePerlin(&bn->climate[NP_SHIFT], biomeX, 0, biomeZ) * 4.0;
    double pz = biomeZ + sampleDoublePerlin(&bn->climate[NP_SHIFT], biomeZ, biomeX, 0) * 4.0;

    float c = (float) sampleDoublePerlin(&bn->climate[NP_CONTINENTALNESS], px, 0, pz);
    float e = (float) sampleDoublePerlin(&bn->climate[NP_EROSION],         px, 0, pz);
    float w = (float) sampleDoublePerlin(&bn->climate[NP_WEIRDNESS],       px, 0, pz);
    float r = -3.0f * (fabsf(fabsf(w) - 0.6666667f) - 0.33333334f);

    float np[4] = { c, e, r, w };

    if (offset) *offset = getSpline(bn->sp, np) - 0.50375;
    if (factor) *factor = getSpline(ts->spFactor, np);
}

double sampleTerrainDensity(const TerrainNoise *tn, double offset, double factor,
        int x, int y, int z)
{
    int l = floordiv(x, 4);
    int m = floordiv(y, 8);
    int n = floordiv(z, 4);
    double terrainD = sampleSurfaceNoise(&tn->sn, l, m, n) / 128.0;

    double g = (1.0 - (double) y / 128.0) + offset;
    double e = g * factor;
    e *= (e > 0.0) ? 4.0 : 1.0;
    double result = e + terrainD;

    if (result >  64.0) result =  64.0;
    if (result < -64.0) result = -64.0;

    int yc = floordiv(y, 8) + 8;
    result = clampedLerp(((double)(48 - yc) - 8.0) / 2.0, -0.0781250, result);
    result = clampedLerp((double) yc / 3.0,                0.1171875, result);

    if (result >  64.0) result =  64.0;
    if (result < -64.0) result = -64.0;
    return result;
}

//==============================================================================
// Cave noise (1.18+)
//==============================================================================

static double caveScaleCaves(double d)
{
    if (d < -0.75) return 0.5;
    if (d < -0.5)  return 0.75;
    if (d < 0.5)   return 1.0;
    return d < 0.75 ? 2.0 : 3.0;
}

static double caveScaleTunnels(double d)
{
    if (d < -0.5) return 0.75;
    if (d < 0.0)  return 1.0;
    return d < 0.5 ? 1.5 : 2.0;
}

// unclamped: lerp((noise+1)/2, g, h)
static double caveLerpFromNoise(
    const DoublePerlinNoise *n,
    double x, double y, double z,
    double g, double h)
{
    double i = sampleDoublePerlin(n, x, y, z);
    double t = (i + 1.0) * 0.5;
    return lerp(t, g, h);
}

static void seedKey(
    Xoroshiro *out,
    uint64_t xlo, uint64_t xhi,
    uint64_t klo, uint64_t khi)
{
    out->lo = klo ^ xlo;
    out->hi = khi ^ xhi;
}

void initCaveNoise(CaveNoise *cn, uint64_t seed)
{
    Xoroshiro pxr, kxr;
    xSetSeed(&pxr, seed);
    uint64_t xlo = xNextLong(&pxr);
    uint64_t xhi = xNextLong(&pxr);
    PerlinNoise *op = cn->oct;

    static const double AMP_ONE[]           = { 1 };
    static const double AMP_PILLAR[]        = { 1, 1 };
    static const double AMP_CAVE_ENTRANCE[] = { 0.4, 0.5, 1 };
    static const double AMP_CAVE_CHEESE[]   = { 0.5, 1, 2, 1, 2, 1, 0, 2, 0 };

    seedKey(&kxr, xlo, xhi, 0xf1008a17493d9a65ULL, 0xbd1ce09e8bc70ea0ULL);              // cave_entrance
    op += xDoublePerlinInit(&cn->caveEntrance,      &kxr, op, AMP_CAVE_ENTRANCE, -7, 3, 0);
    seedKey(&kxr, xlo, xhi, 0x4dfe67be2ef51a83ULL, 0x5a4ae8c4423e7206ULL);              // cave_layer
    op += xDoublePerlinInit(&cn->caveLayer,         &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xb159093bc7baaa50ULL, 0x53abc45424417c20ULL);              // cave_cheese
    op += xDoublePerlinInit(&cn->caveCheese,        &kxr, op, AMP_CAVE_CHEESE, -8, 9, 0);
    seedKey(&kxr, xlo, xhi, 0x34748c98fa19c477ULL, 0x2a02051eb9e9b9cdULL);              // spaghetti_2d
    op += xDoublePerlinInit(&cn->spaghetti2d,       &kxr, op, AMP_ONE, -7, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x29dd5fcf38d9edcdULL, 0x5bb1d7a839f6d4abULL);              // spaghetti_2d_elevation
    op += xDoublePerlinInit(&cn->spaghetti2dElev,   &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x95e07097e4685522ULL, 0x0232deaf95eb5fedULL);              // spaghetti_2d_modulator
    op += xDoublePerlinInit(&cn->spaghetti2dMod   , &kxr, op, AMP_ONE, -11, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x37e5cb432b85f4c8ULL, 0xe5e16b35b407aebcULL);              // spaghetti_2d_thickness
    op += xDoublePerlinInit(&cn->spaghetti2dThick,  &kxr, op, AMP_ONE, -11, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xa5053f53ce3f5840ULL, 0x834fa38a3ded87b7ULL);              // spaghetti_3d_1
    op += xDoublePerlinInit(&cn->spaghetti3d1,      &kxr, op, AMP_ONE, -7, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xb9a6367335a0ceefULL, 0xd61ccfd0fac10ac3ULL);              // spaghetti_3d_2
    op += xDoublePerlinInit(&cn->spaghetti3d2,      &kxr, op, AMP_ONE, -7, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xee4780c98d93a439ULL, 0xbe45d334fdb76d2cULL);              // spaghetti_3d_rarity
    op += xDoublePerlinInit(&cn->spaghetti3dRarity, &kxr, op, AMP_ONE, -11, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x897e1f20ad2d2b27ULL, 0xf4142f5a58d1d5abULL);              // spaghetti_3d_thickness
    op += xDoublePerlinInit(&cn->spaghetti3dThick,  &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x7aed57fd327d6591ULL, 0xd495a3593e4d67bdULL);              // spaghetti_roughness
    op += xDoublePerlinInit(&cn->spaghettiRough,    &kxr, op, AMP_ONE, -5, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xe19387d2ceaf4eaaULL, 0xcacdcd34e1bc2003ULL);              // spaghetti_roughness_modulator
    op += xDoublePerlinInit(&cn->spaghettiRoughMod, &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xd4defd1400fa5347ULL, 0xccb6785451feaa1cULL);              // pillar
    op += xDoublePerlinInit(&cn->pillar,            &kxr, op, AMP_PILLAR, -7, 2, 0);
    seedKey(&kxr, xlo, xhi, 0x8ddd6be7cd4b24d0ULL, 0x0485e8665333197aULL);              // pillar_rareness
    op += xDoublePerlinInit(&cn->pillarRareness,    &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x1a28cb2542f8308dULL, 0xc4bba10b7fc7168cULL);              // pillar_thickness
    op += xDoublePerlinInit(&cn->pillarThickness,   &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xd23ce12b0c37e44bULL, 0x66b1fdfbf6a474f3ULL);              // noodle
    op += xDoublePerlinInit(&cn->noodle,            &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x2fa49bbe949d4212ULL, 0x5f82c95251d19891ULL);              // noodle_thickness
    op += xDoublePerlinInit(&cn->noodleThickness,   &kxr, op, AMP_ONE, -8, 1, 0);
    seedKey(&kxr, xlo, xhi, 0x86172ac1315f6026ULL, 0x4a664470c7d7205fULL);              // noodle_ridge_a
    op += xDoublePerlinInit(&cn->noodleRidgeA,      &kxr, op, AMP_ONE, -7, 1, 0);
    seedKey(&kxr, xlo, xhi, 0xeb232b4b89fdde91ULL, 0x031ee565ead84e5cULL);              // noodle_ridge_b
    op += xDoublePerlinInit(&cn->noodleRidgeB,      &kxr, op, AMP_ONE, -7, 1, 0);

    // op - cn->oct must be <= 50 (sizeof(cn->oct)/sizeof(PerlinNoise))
}

static double sampleCaveEntranceNoise(const CaveNoise *cn, int x, int y, int z)
{
    double g = sampleDoublePerlin(&cn->caveEntrance, x*0.75, y*0.5, z*0.75) + 0.37;
    double h = (double)(y + 10) / 40.0;
    return g + clampedLerp(h, 0.3, 0.0);
}

static double samplePillarNoise(const CaveNoise *cn, int x, int y, int z)
{
    double f = caveLerpFromNoise(&cn->pillarRareness,  x, y, z, 0.0, 2.0);
    double l = caveLerpFromNoise(&cn->pillarThickness, x, y, z, 0.0, 1.1);
    double o = sampleDoublePerlin(&cn->pillar, x*25.0, y*0.3, z*25.0);
    l = l * l * l;
    o = l * (o * 2.0 - f);
    return o > 0.03 ? o : -1000.0;
}

static double sampleCaveLayerNoise(const CaveNoise *cn, int x, int y, int z)
{
    double d = sampleDoublePerlin(&cn->caveLayer, x, y*8, z);
    return d*d * 4.0;
}

static double sampleSpaghetti3dNoise(const CaveNoise *cn, int x, int y, int z)
{
    double d = sampleDoublePerlin(&cn->spaghetti3dRarity, x*2, y, z*2);
    double e = caveScaleTunnels(d);
    double h = caveLerpFromNoise(&cn->spaghetti3dThick, x, y, z, 0.065, 0.088);
    double l = fabs(e * sampleDoublePerlin(&cn->spaghetti3d1, x/e, y/e, z/e)) - h;
    double o = fabs(e * sampleDoublePerlin(&cn->spaghetti3d2, x/e, y/e, z/e)) - h;
    double r = l > o ? l : o;
    if (r < -1.0) r = -1.0;
    if (r >  1.0) r =  1.0;
    return r;
}

static double sampleSpaghetti2dNoise(const CaveNoise *cn, int x, int y, int z)
{
    double d = sampleDoublePerlin(&cn->spaghetti2dMod, x*2, y, z*2);
    double e = caveScaleCaves(d);
    double h = caveLerpFromNoise(&cn->spaghetti2dThick, x*2, y, z*2, 0.6, 1.3);
    double l = fabs(e * sampleDoublePerlin(&cn->spaghetti2d, x/e, y/e, z/e)) - 0.083*h;
    double q = caveLerpFromNoise(&cn->spaghetti2dElev, x, 0, z, -8.0, 8.0);
    double r = fabs(q - (double)y/8.0) - h;
    r = r*r*r;
    double m = l > r ? l : r;
    if (m < -1.0) m = -1.0;
    if (m >  1.0) m =  1.0;
    return m;
}

static double sampleSpaghettiRoughnessNoise(const CaveNoise *cn, int x, int y, int z)
{
    double d = caveLerpFromNoise(&cn->spaghettiRoughMod, x, y, z, 0.0, 0.1);
    return (0.4 - fabs(sampleDoublePerlin(&cn->spaghettiRough, x, y, z))) * d;
}

static double caveClampedLerpFromProgress(
    double progress,
    double fromProgress,
    double toProgress,
    double fromValue,
    double toValue)
{
    double part = (progress - fromProgress) / (toProgress - fromProgress);
    return clampedLerp(part, fromValue, toValue);
}

void initTerrainCache(TerrainCache *tc)
{
    memset(tc, 0, sizeof(*tc));
}

static uint32_t terrainCacheHash(int x, int y, int z)
{
    uint32_t h = (uint32_t)x * 0x9e3779b1u;
    h ^= (uint32_t)y * 0x85ebca6bu;
    h ^= (uint32_t)z * 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

static double sampleNoodleNoiseCorner(
    const DoublePerlinNoise *noise,
    double defaultVal,
    double scale,
    TerrainCache *tc,
    int typ,
    int x, int y, int z)
{
    if (y < -60 || y > 320)
        return defaultVal;
    if (tc)
    {
        TerrainCacheEntry *e = &tc->noodle[typ][terrainCacheHash(x, y, z) & 16383];
        if (e->used && e->x == x && e->y == y && e->z == z)
            return e->val;
        e->used = 1;
        e->x = x, e->y = y, e->z = z;
        e->val = sampleDoublePerlin(noise, x * scale, y * scale, z * scale);
        return e->val;
    }
    return sampleDoublePerlin(noise, x * scale, y * scale, z * scale);
}

static double sampleNoodleNoiseInterpolated(
    const DoublePerlinNoise *noise,
    double dval,
    double scale,
    TerrainCache *tc,
    int typ,
    int x, int y, int z)
{
    int x0 = floordiv(x, 4) * 4;
    int y0 = floordiv(y, 8) * 8;
    int z0 = floordiv(z, 4) * 4;
    double dx = (double)(x - x0) / 4.0;
    double dy = (double)(y - y0) / 8.0;
    double dz = (double)(z - z0) / 4.0;

    return lerp3(dx, dy, dz,
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0,   y0,   z0  ),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0+4, y0,   z0  ),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0,   y0+8, z0  ),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0+4, y0+8, z0  ),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0,   y0,   z0+4),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0+4, y0,   z0+4),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0,   y0+8, z0+4),
            sampleNoodleNoiseCorner(noise,dval,scale,tc,typ, x0+4, y0+8, z0+4));
}

static double sampleNoodleAdjustedDensityInterpolated(
    const CaveNoise *cn,
    TerrainCache    *tc,
    double density,
    int x, int y, int z)
{
    double e = density * 0.64;
    if (e < -1.0) e = -1.0;
    if (e >  1.0) e =  1.0;
    e = e / 2.0 - e * e * e / 24.0;
    if (e <= 0.0)
        return e;

    if (sampleNoodleNoiseInterpolated(&cn->noodle, -1.0, 1.0, tc, 0, x, y, z) >= 0.0)
    {
        double thick = sampleNoodleNoiseInterpolated(&cn->noodleThickness, 0.0, 1.0, tc, 1, x, y, z);
        double h = caveClampedLerpFromProgress(thick, -1.0, 1.0, 0.05, 0.1);
        double a = sampleNoodleNoiseInterpolated(&cn->noodleRidgeA, 0.0, 2.6666666666666665, tc, 2, x, y, z);
        double b = sampleNoodleNoiseInterpolated(&cn->noodleRidgeB, 0.0, 2.6666666666666665, tc, 3, x, y, z);
        double l = fabs(1.5 * a) - h;
        double m = fabs(1.5 * b) - h;
        double noodle = l > m ? l : m;
        if (noodle < e)
            e = noodle;
    }

    return e;
}

static double sampleTerrainDensityCavesCorner(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    TerrainCache        *tc,
    int x, int y, int z)
{
    if (tc)
    {
        TerrainCacheEntry *e = &tc->density[terrainCacheHash(x, y, z) & 32767];
        if (e->used && e->x == x && e->y == y && e->z == z)
            return e->val;
        e->used = 1;
        e->x = x, e->y = y, e->z = z;
        double offset, factor;
        sampleTerrainShape(bn, ts, floordiv(x, 4), floordiv(z, 4), &offset, &factor);
        e->val = sampleTerrainDensityCaves(tn, cn, offset, factor, x, y, z);
        return e->val;
    }
    double offset, factor;
    sampleTerrainShape(bn, ts, floordiv(x, 4), floordiv(z, 4), &offset, &factor);
    return sampleTerrainDensityCaves(tn, cn, offset, factor, x, y, z);
}

static double sampleTerrainDensityCavesInterpolated(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    TerrainCache        *tc,
    int x, int y, int z)
{
    int x0 = floordiv(x, 4) * 4;
    int y0 = floordiv(y, 8) * 8;
    int z0 = floordiv(z, 4) * 4;
    double dx = (double)(x - x0) / 4.0;
    double dy = (double)(y - y0) / 8.0;
    double dz = (double)(z - z0) / 4.0;

    return lerp3(dx, dy, dz,
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0,   y0,   z0  ),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0+4, y0,   z0  ),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0,   y0+8, z0  ),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0+4, y0+8, z0  ),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0,   y0,   z0+4),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0+4, y0,   z0+4),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0,   y0+8, z0+4),
            sampleTerrainDensityCavesCorner(bn,tn,ts,cn,tc, x0+4, y0+8, z0+4));
}

int isSolidCached(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    TerrainCache        *tc,
    int x, int y, int z)
{
    if (y < -60 || y > 254)
        return 0;
    double density;
    density = sampleTerrainDensityCavesInterpolated(bn, tn, ts, cn, tc, x, y, z);
    density = sampleNoodleAdjustedDensityInterpolated(cn, tc, density, x, y, z);
    return density > 0.0;
}

int isSolid(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    int x, int y, int z)
{
    return isSolidCached(bn, tn, ts, cn, NULL, x, y, z);
}

int isAir(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    int x, int y, int z)
{
    return !isSolidCached(bn, tn, ts, cn, NULL, x, y, z);
}

double sampleTerrainDensityCaves(
    const TerrainNoise *tn,
    const CaveNoise    *cn,
    double offset,
    double factor,
    int x, int y, int z)
{
    int l = floordiv(x, 4);
    int m = floordiv(y, 8);
    int n = floordiv(z, 4);
    double terrainD = sampleSurfaceNoise(&tn->sn, l, m, n) / 128.0;

    double g = (1.0 - (double) y / 128.0) + offset;
    double e = g * factor;
    e *= (e > 0.0) ? 4.0 : 1.0;
    double f = e + terrainD;

    double h, lval, mval;
    if (!(f < -64.0))
    {
        double nrm = f - 1.5625;
        int below = nrm < 0.0;
        double cave = sampleCaveEntranceNoise(cn, x, y, z);
        double rough = sampleSpaghettiRoughnessNoise(cn, x, y, z);
        double spag3 = sampleSpaghetti3dNoise(cn, x, y, z);
        double tmp = spag3 + rough;
        double r = cave < tmp ? cave : tmp;

        if (below)
        {
            h = f;
            lval = r * 5.0;
            mval = -64.0;
        }
        else
        {
            double layer = sampleCaveLayerNoise(cn, x, y, z);
            double cheese = sampleDoublePerlin(&cn->caveCheese, x, (double)y/1.5, z);
            double u = cheese + 0.27;
            if (u < -1.0) u = -1.0;
            if (u >  1.0) u =  1.0;
            double v = nrm * 1.28;
            double w = u + clampedLerp(v, 0.5, 0.0);
            h = w + layer;
            double spag2 = sampleSpaghetti2dNoise(cn, x, y, z);
            double tmp2 = spag2 + rough;
            lval = r < tmp2 ? r : tmp2;
            mval = samplePillarNoise(cn, x, y, z);
        }
    }
    else
    {
        h = f;
        lval = 64.0;
        mval = -64.0;
    }

    double result = h < lval ? h : lval;
    if (result < mval) result = mval;

    int yc = floordiv(y, 8) + 8;
    result = clampedLerp(((double)(48 - yc) - 8.0) / 2.0, -0.0781250, result);
    result = clampedLerp((double) yc / 3.0,                0.1171875, result);

    if (result >  64.0) result =  64.0;
    if (result < -64.0) result = -64.0;
    return result;
}
