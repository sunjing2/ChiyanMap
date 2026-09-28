#include "finders.h"
#include "biomes.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>


#define PI 3.14159265358979323846


//==============================================================================
// Finding Structure Positions
//==============================================================================


int getStructureConfig(int structureType, int mc, StructureConfig *sconf)
{
    static const StructureConfig
    s_feature               = { 14357617, 32, 24, Feature,          DIM_OVERWORLD, 0},
    s_desert_pyramid        = { 14357617, 32, 24, Desert_Pyramid,   DIM_OVERWORLD, 0},
    s_igloo                 = { 14357617, 32, 24, Igloo,            DIM_OVERWORLD, 0},
    s_jungle_temple         = { 14357617, 32, 24, Jungle_Pyramid,   DIM_OVERWORLD, 0},
    s_swamp_hut             = { 14357617, 32, 24, Swamp_Hut,        DIM_OVERWORLD, 0},
    s_outpost               = {165745296, 80, 56, Outpost,          DIM_OVERWORLD, 0},
    s_village_110           = { 10387312,  1,  1, Village,          DIM_OVERWORLD, 50},// potentially generated chunk-by-chunk
    s_village_117           = { 10387312, 27, 17, Village,          DIM_OVERWORLD, 0.2 },// abandoned
    s_village_1730          = { 10387312, 27, 17, Village,          DIM_OVERWORLD, 0.02},// abandoned
    s_village               = { 10387312, 34, 26, Village,          DIM_OVERWORLD, 0.02},// abandoned
    s_stronghold            = { 97858791,200,150, Stronghold,       DIM_OVERWORLD, 0.25},
    s_ocean_ruin_117        = { 14357621, 12,  5, Ocean_Ruin,       DIM_OVERWORLD, 0},
    s_ocean_ruin            = { 14357621, 20, 12, Ocean_Ruin,       DIM_OVERWORLD, 0},
    s_shipwreck_117         = {165745295, 10,  5, Shipwreck,        DIM_OVERWORLD, 0},
    s_shipwreck             = {165745295, 24, 20, Shipwreck,        DIM_OVERWORLD, 0},
    s_monument              = { 10387313, 32, 27, Monument,         DIM_OVERWORLD, 0},
    s_mansion               = { 10387319, 80, 60, Mansion,          DIM_OVERWORLD, 0},
    s_ruined_portal         = { 40552231, 40, 25, Ruined_Portal,    DIM_OVERWORLD, 0},
    s_ancient_city          = { 20083232, 24, 16, Ancient_City,     DIM_OVERWORLD, 0},
    s_trail_ruins           = { 83469867, 34, 26, Trail_Ruins,      DIM_OVERWORLD, 0},
    s_trial_chambers        = { 94251327, 34, 22, Trial_Chambers,   DIM_OVERWORLD, 0},
    s_abandoned_camp        = { 91231127, 34, 26, Abandoned_Camp,   DIM_OVERWORLD, 0},
    s_treasure              = { 16842397,  4,  2, Treasure,         DIM_OVERWORLD, 0},
    s_mineshaft             = {        0,  1,  1, Mineshaft,        DIM_OVERWORLD, 0},
    // nether structures
    s_ruined_portal_n       = { 40552231, 25, 15, Ruined_Portal_N,  DIM_NETHER, 0},
    s_fortress_114          = {        0, 16,  8, Fortress,         DIM_NETHER, 0},
    s_fortress              = { 30084232, 30, 26, Fortress,         DIM_NETHER, 0},
    s_bastion               = { 30084232, 30, 26, Bastion,          DIM_NETHER, 0},
    // end structures
    s_end_city              = { 10387313, 20,  9, End_City,         DIM_END, 0},
    s_end_gateway           = {        0,  1,  1, End_Gateway,      DIM_END, 700},
    // decorators
    s_desert_well           = {-1160484816,  1,  1, Desert_Well,    DIM_OVERWORLD, 500},
    s_geode_117             = { 1974035328,  1,  1, Geode,          DIM_OVERWORLD, 53},
    s_geode                 = { 1974035328,  1,  1, Geode,          DIM_OVERWORLD, 24},
    s_dungeon               = {          0,  1,  1, Dungeon,        DIM_OVERWORLD, 0},
    s_end_island            = {          0,  1,  1, End_Island,     DIM_END,       14},
    s_ravine                = {          0,  1,  1, Ravine,         DIM_OVERWORLD, 0},
    s_lavalake              = {          0,  1,  1, Lava_Lake,      DIM_OVERWORLD, 8}
    ;

    switch (structureType)
    {
    case Feature:
        *sconf = s_feature;
        return mc <= MC_UNDEF;
    case Desert_Pyramid:
        *sconf = s_desert_pyramid;
        return mc >= MC_UNDEF;// 0.13.0
    case Jungle_Pyramid:
        *sconf = s_jungle_temple;
        return mc >= MC_UNDEF;// 0.15.0
    case Swamp_Hut:
        *sconf = s_swamp_hut;
        return mc >= MC_UNDEF;// 0.14.0
    case Igloo:
        *sconf = s_igloo;
        return mc >= MC_1_0;
    case Village:
        if (mc >= MC_1_18)
            *sconf = s_village;
        else if (mc >= MC_1_17_30)
            *sconf = s_village_1730;
        else if (mc >= MC_1_11)
            *sconf = s_village_117;
        else
            *sconf = s_village_110;
        return mc >= MC_UNDEF;// 0.9.0
    case Stronghold:
        *sconf = s_stronghold;
        return mc >= MC_UNDEF;// 0.9.0
    case Ocean_Ruin:
        *sconf = mc < MC_1_18 ? s_ocean_ruin_117 : s_ocean_ruin;
        return mc >= MC_1_4;
    case Shipwreck:
        *sconf = mc < MC_1_18 ? s_shipwreck_117 : s_shipwreck;
        return mc >= MC_1_4;
    case Ruined_Portal:
        *sconf = s_ruined_portal;
        return mc >= MC_1_16;
    case Ruined_Portal_N:
        *sconf = s_ruined_portal_n;
        return mc >= MC_1_16;
    case Monument:
        *sconf = s_monument;
        return mc >= MC_UNDEF;// 0.16.0
    case End_City:
        *sconf = s_end_city;
        return mc >= MC_1_0; 
    case Mansion:
        *sconf = s_mansion;
        return mc >= MC_1_1;
    case Outpost:
        *sconf = s_outpost;
        return mc >= MC_1_11;
    case Ancient_City:
        *sconf = s_ancient_city;
        return mc >= MC_1_19;
    case Treasure:
        *sconf = s_treasure;
        return mc >= MC_UNDEF;// 0.9.0
    case Mineshaft:
        *sconf = s_mineshaft;
        return mc >= MC_UNDEF;// 0.9.0
    case Ravine:
        *sconf = s_ravine;
        return mc >= MC_1_2;
    case Fortress:
        *sconf = mc <= MC_1_14 ? s_fortress_114 : s_fortress;
        return mc >= MC_1_0;
    case Bastion:
        *sconf = s_bastion;
        return mc >= MC_1_16;
    case End_Gateway:
        *sconf = s_end_gateway;
        return mc >= MC_1_0; 
    case End_Island:
        *sconf = s_end_island;
        return mc >= MC_1_13; // we only support decorator features for 1.13+
    case Desert_Well:
        *sconf = s_desert_well;
        // wells were introduced in 1.2, but we only support decorator features
        // for 1.13+
        return mc >= MC_1_13;
    case Lava_Lake:
        *sconf = s_lavalake;
        return mc >= MC_1_0;
    case Geode:
        *sconf = mc < MC_1_18 ? s_geode_117 : s_geode;
        return mc >= MC_1_17;
    case Dungeon:
        *sconf = s_dungeon;
        // only 1.18+ dungeons are supported
        // the cave terrain uses 1.18+
        return mc >= MC_1_18;
    case Trail_Ruins:
        *sconf = s_trail_ruins;
        return mc >= MC_1_20;
    case Trial_Chambers:
        *sconf = s_trial_chambers;
        return mc >= MC_1_21;
    case Abandoned_Camp:
        *sconf = s_abandoned_camp;
        return mc >= MC_26_40; // 26.40.27
    default:
        memset(sconf, 0, sizeof(StructureConfig));
        return 0;
    }
}



int getStructurePos(int structureType, int mc, uint64_t seed, int regX, int regZ, Pos *pos)
{
    StructureConfig sconf;
#if STRUCT_CONFIG_OVERRIDE
    if (!getStructureConfig_override(structureType, mc, &sconf))
#else
    if (!getStructureConfig(structureType, mc, &sconf))
#endif
    {
        return 0;
    }

    switch (structureType)
    {
    case Feature:
    case Desert_Pyramid:
    case Jungle_Pyramid:
    case Swamp_Hut:
    case Igloo:
    case Ruined_Portal:
    case Ruined_Portal_N:
    case Trail_Ruins:
    case Trial_Chambers:
        *pos = getFeaturePos(sconf, seed, regX, regZ);
        return 1;

    case Ocean_Ruin:
	case Shipwreck:
		*pos = (mc < MC_1_18 ? getLargeStructurePos : getFeaturePos)(sconf, seed, regX, regZ);
		return 1;

    case Village:
        if (mc < MC_1_11) {
            if (!isVillageChunk(sconf, seed, regX, regZ))
                return 0;
            Pos vp;
            vp.x = (regX << 4) + 8;
            vp.z = (regZ << 4) + 8;
            *pos = vp;
        } else {
            *pos = getLargeStructurePos(sconf, seed, regX, regZ);
        }
		return 1;

    case Monument:
    case Mansion:
    case Outpost:
    case Ancient_City:
    case Treasure:
    case Abandoned_Camp:
        *pos = getLargeStructurePos(sconf, seed, regX, regZ);
        // bug? for some reason v1.4.2 treasures spawn at coords -2 from where they should be
        if (structureType == Treasure && mc <= MC_1_4)
            pos->x -= 2, pos->z -= 2;
        return 1;

    case Stronghold:
        return getStaticStronghold(sconf, seed, regX, regZ, pos);

    case End_City:
        *pos = getLargeStructurePos(sconf, seed, regX, regZ);
        return (pos->x*(int64_t)pos->x + pos->z*(int64_t)pos->z) >= 1008*1008LL;

    case Mineshaft:
        return getMineshafts(mc, seed, regX, regZ, regX, regZ, pos, 1);

    case Ravine:
        return getRavines(mc, seed, regX, regZ, regX, regZ, pos, 1);

    case Lava_Lake:
        {
            if (mc < MC_1_18) return 0;// If a desert is present, the water lake will not be processed, causing the random numbers to mismatch.
            return getLavaLakes(mc, seed, regX, regZ, regX, regZ, pos, 1, 0);
        }

    case Fortress:
        if (mc >= MC_1_16) {
            *pos = getFeaturePos(sconf, seed, regX, regZ);
            return nextInt(6) < 2;
        }
        else
        {
            int cx = regX << 4;
            int cz = regZ << 4;
            setFortressSeed(seed, cx, cz);
            skipNextN(1);
            if (nextInt(3) != 0) return 0;
            pos->x = (((cx & ~15) + nextInt(sconf.chunkRange) + 4) << 4)+11;
            pos->z = (((cz & ~15) + nextInt(sconf.chunkRange) + 4) << 4)+11;
            return 1;
        }

    case Bastion:
        {
            *pos = getFeaturePos(sconf, seed, regX, regZ);
            return nextInt(6) >= 2;
        }

    case End_Gateway:
    {
        return 0;// dont use getStructurePos
    }

    case Desert_Well:
    {
        pos->x = regX << 4;
        pos->z = regZ << 4;
        setDecorationSeed(seed, regX, regZ, sconf.salt);
        if (nextInt(sconf.rarity) != 0) return 0;
        pos->x += nextInt(16);
        pos->z += nextInt(16);
        return 1;
    }

    case End_Island:
    {
        pos->x = regX << 4;
        pos->z = regZ << 4;
        if ((pos->x*(int64_t)pos->x + pos->z*(int64_t)pos->z) < 1008*1008LL)
            return 0;
        setPopulationSeed(seed, regX, regZ);
        if (mc >= MC_1_18)
            skipNextN(1);
        if (nextInt(sconf.rarity) != 0) return 0;
        pos->x += nextInt(16) + 8;
        nextInt(16);// y
        pos->z += nextInt(16) + 8;
        return 1;
    }

    case Geode:
    {
        pos->x = regX << 4;
        pos->z = regZ << 4;
        setDecorationSeed(seed, regX, regZ, sconf.salt);
        if (nextInt(sconf.rarity) != 0) return 0;
        pos->x += 4;
        pos->z += 4;
        return 1;
    }

    case Dungeon:
        // multiple dungeons can generate per chunk; use getDungeons() instead
        return 0;

    default:
        fprintf(stderr,
                "ERR getStructurePos: unsupported structure type %d\n", structureType);
        exit(-1);
    }
    return 0;
}


int getMineshafts(int mc, uint64_t seed, int cx0, int cz0, int cx1, int cz1,
        Pos *out, int nout)
{
    int i, j;
    int n = 0;

    for (i = cx0; i <= cx1; i++)
    {
        for (j = cz0; j <= cz1; j++)
        {
            if (mc >= MC_1_11)
                setCarverSeed(seed, i, j);
            else
                setSeed(seed ^ j ^ i);
            skipNextN(1);
            if (nextFloat() < 0.004)
            {
                int a = abs(i);
                int b = abs(j);
                if (nextInt(80) < (a > b ? a : b))
                {
                    if (out && n < nout)
                    {
                        out[n].x = i * 16+8;
                        out[n].z = j * 16+8;
                    }
                    n++;
                }
            }
        }
    }

    return n;
}

int getRavines(int mc, uint64_t seed, int cx0, int cz0, int cx1, int cz1,
                  Pos *out, int nout)
{
    int i, j;
    int n = 0;
    int chance = mc >= MC_1_21_60 ? 100 : 150;

    for (i = cx0; i <= cx1; i++)
    {
        for (j = cz0; j <= cz1; j++)
        {
            setPopulationSeed(seed, i, j);
            if (nextInt(chance) == 0)
            {
                if (out && n < nout)
                {
                    out[n].x = i * 16+nextInt(16);
                    skipNextN(3);
                    out[n].z = j * 16+nextInt(16);
                }
                n++;
            }
        }
    }

    return n;
}

int getGeodes(int mc, uint64_t seed, int cx0, int cz0, int cx1, int cz1,
                  Pos *out, int nout)
{
    StructureConfig sconf;
    getStructureConfig(Geode, mc, &sconf);

    int i, j;
    int n = 0;
    int chance = sconf.rarity;

    for (i = cx0; i <= cx1; i++)
    {
        for (j = cz0; j <= cz1; j++)
        {
            setDecorationSeed(seed, i, j, sconf.salt);
            if (nextInt(chance) == 0)
            {
                if (out && n < nout)
                {
                    out[n].x = i * 16+4;
                    out[n].z = j * 16+4;
                }
                n++;
            }
        }
    }

    return n;
}

int getLavaLakes(int mc, uint64_t seed, int cx0, int cz0, int cx1, int cz1,
                  Pos *out, int nout, int isDesert)
{
    int i, j;
    int x, y, z, n = 0;;

    for (i = cx0; i <= cx1; i++)
    {
        for (j = cz0; j <= cz1; j++)
        {
            setPopulationSeed(seed, i, j);
            if (mc < MC_1_18 && !isDesert) {
                //water lakes
                if (nextInt(4) == 0)
                    continue;
            }
            if (nextInt(8) == 0)
            {
                x = nextInt(9);
                int k = nextIntRange(8, 128);
                y = nextInt(k);
                z = nextInt(9);
                if (y >= 60)
                {
                    if (y >= 64)
                    {
                        if (nextInt(10) == 0)
                        {
                            if (out && n < nout)
                            {
                                out[n].x = i * 16 + x;
                                out[n].z = j * 16 + z;
                            }
                            n++;
                        }
                    }
                    else
                    {
                        if (out && n < nout)
                        {
                            out[n].x = i * 16 + x;
                            out[n].z = j * 16 + z;
                        }
                        n++;
                    }
                }
            }
        }
    }

    return n;
}

int getEndIslands(EndIsland islands[2], int mc, uint64_t seed, int chunkX, int chunkZ)
{
    StructureConfig sconf;
    if (!getStructureConfig(End_Island, mc, &sconf))
        return 0;

    int x = chunkX * 16;
    int z = chunkZ * 16;
    float r;

    setPopulationSeed(seed, chunkX, chunkZ);
    if (mc >= MC_1_18) {
        skipNextN(1);
    }
    if (nextInt(sconf.rarity) != 0)
        return 0;
    islands[0].x = nextInt(16) + x + 8;
    islands[0].y = nextInt(16) + 55;
    islands[0].z = nextInt(16) + z + 8;
    islands[0].r = nextInt(3) + 4;
    for (r = islands[0].r; r > 0.5; r -= nextInt(2) + 0.5);
    int second = nextInt(3) == 0;
    if (!second)
        return 1;
    islands[1].x = islands[0].x;
    islands[1].y = islands[0].y;
    islands[1].z = islands[0].z;
    islands[1].r = nextInt(3) + 4;
    return 2;
}

static void applyEndIslandHeight(float *y, const EndIsland *island,
    int x, int z, int w, int h, int scale)
{
    int r = island->r;
    int r2 = (r + 1) * (r + 1);
    int x0 = floordiv(island->x - r, scale);
    int z0 = floordiv(island->z - r, scale);
    int x1 = floordiv(island->x + r, scale);
    int z1 = floordiv(island->z + r, scale);
    int ds = 0;
    int i, j;
    for (j = z0; j <= z1; j++)
    {
        if (j < z || j >= z+h)
            continue;
        int dz = j * scale - island->z + ds;
        for (i = x0; i <= x1; i++)
        {
            if (i < x || i >= x+w)
                continue;
            int dx = i * scale - island->x + ds;
            if (dx*dx + dz*dz > r2)
                continue;
            int idx = (j - z) * w + (i - x);
            if (y[idx] < island->y)
                y[idx] = island->y;
        }
    }
}

int mapEndIslandHeight(float *y, const EndNoise *en, uint64_t seed,
    int x, int z, int w, int h, int scale)
{
    int rmax = (6 + scale - 1) / scale;
    int cx = floordiv(x - rmax, 16 / scale);
    int cz = floordiv(z - rmax, 16 / scale);
    int cw = floordiv(x + w + rmax, 16 / scale) - cx + 1;
    int ch = floordiv(z + h + rmax, 16 / scale) - cz + 1;
    int ci, cj;

    int *ids = (int*) malloc(sizeof(int) * cw * ch);
    mapEndBiome(en, ids, cx, cz, cw, ch);

    for (cj = 0; cj < ch; cj++)
    {
        for (ci = 0; ci < cw; ci++)
        {
            if (ids[cj*cw + ci] != small_end_islands)
                continue;
            EndIsland islands[2];
            int n = getEndIslands(islands, en->mc, seed, cx+ci, cz+cj);
            while (n --> 0)
                applyEndIslandHeight(y, islands+n, x, z, w, h, scale);
        }
    }

    free(ids);
    return 0;
}

float getEndHeightNoise(const EndNoise *en, int x, int z, int range);

int isEndChunkEmpty(const EndNoise *en, const SurfaceNoise *sn, uint64_t seed,
    int chunkX, int chunkZ)
{
    int i, j, k;
    int x = chunkX * 2;
    int z = chunkZ * 2;
    double depth[3][3];
    float y[256];

    // check if small end islands intersect this chunk
    for (j = -1; j <= +1; j++)
    {
        for (i = -1; i <= +1; i++)
        {
            EndIsland is[2];
            int n = getEndIslands(is, en->mc, seed, chunkX+i, chunkZ+j);
            while (n --> 0)
            {
                if (is[n].x + is[n].r <= chunkX*16) continue;
                if (is[n].z + is[n].r <= chunkZ*16) continue;
                if (is[n].x - is[n].r > chunkX*16 + 15) continue;
                if (is[n].z - is[n].r > chunkZ*16 + 15) continue;
                int id;
                mapEndBiome(en, &id, is[n].x >> 4, is[n].z >> 4, 1, 1);
                if (id == small_end_islands)
                    return 0;
            }
        }
    }

    // clamped (32 + 46 - y) / 64.0
    static const double upper_drop[] = {
           1.0,    1.0,    1.0,    1.0,    1.0,    1.0,    1.0,    1.0, // 0-7
           1.0,    1.0,    1.0,    1.0,    1.0,    1.0,    1.0, 63./64, // 8-15
        62./64, 61./64, 60./64, 59./64, 58./64, 57./64, 56./64, 55./64, // 16-23
        54./64, 53./64, 52./64, 51./64, 50./64, 49./64, 48./64, 47./64, // 24-31
        46./64 // 32
    };
    // clamped (y - 1) / 7.0
    static const double lower_drop[] = {
          0.0,  0.0, 1./7, 2./7, 3./7, 4./7, 5./7, 6./7, // 0-7
          1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, // 8-15
          1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, // 16-23
          1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, // 24-31
          1.0, // 32
    };
    // inverse of clamping func: ( 30 * (1-l) / l + 3000 * (1-u) ) / u
    static const double inverse_drop[] = {
        1e9, 1e9, 180.0, 75.0, 40.0, 22.5, 12.0, 5.0, // 0-7
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // 8-14
        1000./21, 3000./31, 9000./61, 200.0, // 15-18
    };
    const double eps = 0.001;

    // get the inner depth values and see if they imply blocks in the chunk
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            depth[i][j] = getEndHeightNoise(en, x+i, z+j, 0) - 8.0f;
            for (k = 8; k <= 14; k++)
            {
                double u = upper_drop[k];
                double l = lower_drop[k];
                double noise = depth[i][j];
                double pivot = inverse_drop[k] - noise;
                noise += sampleSurfaceNoiseBetween(sn, x+i, k, z+j, pivot-eps, pivot+eps);
                noise = lerp(u, -3000, noise);
                noise = lerp(l, -30, noise);
                if (noise > 0)
                    return 0;
            }
        }
    }

    // fill in the depth values at the boundaries to neighbouring chunks
    for (i = 0; i < 3; i++)
        depth[i][2] = getEndHeightNoise(en, x+i, z+2, 0) - 8.0f;
    for (j = 0; j < 2; j++)
        depth[2][j] = getEndHeightNoise(en, x+2, z+j, 0) - 8.0f;

    // see if none of the noise values can generate blocks
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            for (k = 2; k < 18; k++)
            {
                double u = upper_drop[k];
                double l = lower_drop[k];
                double noise = depth[i][j];
                double pivot = inverse_drop[k] - noise;
                noise += sampleSurfaceNoiseBetween(sn, x+i, k, z+j, pivot-eps, pivot+eps);
                noise = lerp(u, -3000, noise);
                noise = lerp(l, -30, noise);
                if (noise > 0)
                    goto L_check_full;
            }
        }
    }
    return 1;

L_check_full:
    mapEndSurfaceHeight(y, en, sn, chunkX*16, chunkZ*16, 16, 16, 1, 0);
    for (k = 0; k < 256; k++)
        if (y[k] != 0) return 0;
    return 1;
}

//==============================================================================
// Checking Biomes & Biome Helper Functions
//==============================================================================


static int id_matches(int id, uint64_t b, uint64_t m)
{
    return id < 128 ? !!(b & (1ULL << id)) : !!(m & (1ULL << (id-128)));
}

Pos locateBiome(
    const Generator *g, int x, int y, int z, int radius,
    uint64_t validB, uint64_t validM, uint64_t *rng, int *passes)
{
    Pos out = {x, z};
    int i, j, found;
    found = 0;

    if (g->mc >= MC_1_18)
    {
        x >>= 2;
        z >>= 2;
        radius >>= 2;
        uint64_t dat = 0;

        for (j = -radius; j <= radius; j++)
        {
            for (i = -radius; i <= radius; i++)
            {
                // emulate order-dependent biome generation MC-241546
                //int id = getBiomeAt(g, 4, x+i, y, z+j);
                int id = sampleBiomeNoise(&g->bn, NULL, x+i, y, z+j, &dat, 0);
                if (!id_matches(id, validB, validM))
                    continue;

                if (found == 0 || JnextInt(rng, found+1) == 0)
                {
                    out.x = (x+i) * 4;
                    out.z = (z+j) * 4;
                }
                found++;
            }
        }
    }
    else
    {
        int x1 = (x-radius) >> 2;
        int z1 = (z-radius) >> 2;
        int x2 = (x+radius) >> 2;
        int z2 = (z+radius) >> 2;
        int width  = x2 - x1 + 1;
        int height = z2 - z1 + 1;

        Range r = {4, x1, z1, width, height, y, 1};
        int *ids = allocCache(g, r);
        genBiomes(g, ids, r);

        if (g->mc >= MC_1_13)
        {
            for (i = 0, j = 2; i < width*height; i++)
            {
                if (!id_matches(ids[i], validB, validM))
                    continue;
                if (found == 0 || JnextInt(rng, j++) == 0)
                {
                    out.x = (x1 + i%width) * 4;
                    out.z = (z1 + i/width) * 4;
                    found = 1;
                }
            }
            found = j - 2;
        }
        else
        {
            for (i = 0; i < width*height; i++)
            {
                if (!id_matches(ids[i], validB, validM))
                    continue;
                if (found == 0 || JnextInt(rng, found + 1) == 0)
                {
                    out.x = (x1 + i%width) * 4;
                    out.z = (z1 + i/width) * 4;
                    ++found;
                }
            }
        }

        free(ids);
    }


    if (passes != NULL)
    {
        *passes = found;
    }

    return out;
}


int areBiomesViable(
    const Generator *g, int x, int y, int z, int rad,
    uint64_t validB, uint64_t validM, int approx)
{
    int x1 = (x - rad) >> 2, x2 = (x + rad) >> 2, sx = x2 - x1 + 1;
    int z1 = (z - rad) >> 2, z2 = (z + rad) >> 2, sz = z2 - z1 + 1;
    int i, j, id, viable = 1;
    int *ids = NULL;

    // In 1.18+ the area is also checked in y, forming a cube volume.
    // However, this function is only used for monuments, which need ocean or
    // river, where we can get away with just checking the lowest y for caves.
    y = (y - rad) >> 2;

    // check corners
    Pos corners[4] = { {x1,z1}, {x2,z2}, {x1,z2}, {x2,z1} };
    for (i = 0; i < 4; i++)
    {
        id = getBiomeAt(g, 4, corners[i].x, y, corners[i].z);
        if (id < 0 || !id_matches(id, validB, validM))
            goto L_no;
    }
    if (approx >= 1) goto L_yes;

    if (g->mc >= MC_1_18)
    {
        for (i = 0; i < sx; i++)
        {
            uint64_t dat = 0;
            for (j = 0; j < sz; j++)
            {
                if (g->mc >= MC_1_18)
                    id = sampleBiomeNoise(&g->bn, NULL, x1+i, y, z1+j, &dat, 0);
                else
                    id = getBiomeAt(g, 4, x1+i, y, z1+j);
                if (id < 0 || !id_matches(id, validB, validM))
                    goto L_no;
            }
        }
    }
    else
    {
        Range r = {4, x1, z1, sx, sz, y, 1};
        ids = allocCache(g, r);
        if (genBiomes(g, ids, r))
            goto L_no;
        for (i = 0; i < sx*sz; i++)
        {
            if (id < 0 || !id_matches(ids[i], validB, validM))
                goto L_no;
        }
    }

    if (0) L_yes: viable = 1;
    if (0) L_no:  viable = 0;
    if (ids)
        free(ids);
    return viable;
}


//==============================================================================
// Finding Strongholds and Spawn
//==============================================================================


int nextVillageStronghold(StrongholdIter *sh, const Generator *g)
{
    Pos p;
    StructureConfig sc;
    Generator *gm = (Generator *) g;
    getStructureConfig(Village, g->mc, &sc);

    if (sh->index == 0)
    {
        setSeed(g->seed);
        float angle = 2.0 * PI * nextFloat();
        int dist = 40 + nextInt(16);

        sh->pos.x = 0;
        sh->pos.z = 0;
        sh->angle = angle;
        sh->dist = dist;
        sh->mc = g->mc;
    }

    int placed = 0;
    while (!placed)
    {
        int cx = (int)floor(cos(sh->angle) * sh->dist);
        int cz = (int)floor(sin(sh->angle) * sh->dist);

        if (g->mc >= MC_1_11)
        {
            int minChunkX = cx - 8;
            int maxChunkX = cx + 7;
            int minChunkZ = cz - 8;
            int maxChunkZ = cz + 7;
            Pos r0 = chunkToRegion(minChunkX, minChunkZ, sc.regionSize);
            Pos r1 = chunkToRegion(maxChunkX, maxChunkZ, sc.regionSize);
            Pos candPos[4];
            int candChunkX[4], candChunkZ[4];
            int candCount = 0;

            /* Each 1.11+ village region has at most one candidate chunk, so
             * scanning unique regions avoids recomputing the same position for
             * every chunk in the 16x16 search window.
             */
            for (int rx = r0.x; rx <= r1.x; rx++)
            {
                for (int rz = r0.z; rz <= r1.z; rz++)
                {
                    p = getLargeStructurePos(sc, g->seed, rx, rz);
                    int x = p.x >> 4;
                    int z = p.z >> 4;

                    if (x < minChunkX || x > maxChunkX ||
                        z < minChunkZ || z > maxChunkZ)
                        continue;
                    candPos[candCount] = p;
                    candChunkX[candCount] = x;
                    candChunkZ[candCount] = z;
                    candCount++;
                }
            }

            for (int i = 0; i < candCount; i++)
            {
                int best = i;
                for (int j = i + 1; j < candCount; j++)
                {
                    if (candChunkX[j] < candChunkX[best] ||
                        (candChunkX[j] == candChunkX[best] && candChunkZ[j] < candChunkZ[best]))
                    {
                        best = j;
                    }
                }
                if (best != i)
                {
                    int tmpx = candChunkX[i], tmpz = candChunkZ[i];
                    Pos tmpp = candPos[i];
                    candChunkX[i] = candChunkX[best];
                    candChunkZ[i] = candChunkZ[best];
                    candPos[i] = candPos[best];
                    candChunkX[best] = tmpx;
                    candChunkZ[best] = tmpz;
                    candPos[best] = tmpp;
                }
            }

            for (int i = 0; i < candCount; i++)
            {
                p = candPos[i];
                if (!isViableStructurePos(Village, gm, p.x, p.z, 0))
                    continue;
                sh->pos.x = p.x - 4;
                sh->pos.z = p.z - 4;
                sh->index++;
                placed = 1;
                break;
            }
        }
        else
        {
            for (int x = cx - 8; x < cx + 8 && !placed; x++)
            {
                for (int z = cz - 8; z < cz + 8 && !placed; z++)
                {
                    if (!isVillageChunk(sc, g->seed, x, z))
                        continue;
                    p.x = x*16 + 8;
                    p.z = z*16 + 8;
                    if (!isViableStructurePos(Village, gm, p.x, p.z, 0))
                        continue;
                    sh->pos.x = p.x - 4;
                    sh->pos.z = p.z - 4;
                    sh->index++;
                    placed = 1;
                }
            }
        }
        if (placed)
        {
            sh->angle += 0.6f * PI;
            sh->dist += 8;
        }
        else
        {
            sh->angle += 0.25f * PI;
            sh->dist += 4;
        }
    }

    return 3 - (sh->index-1);
}

int getStaticStronghold(StructureConfig sconf, uint64_t seed,
    int regionX, int regionZ, Pos *pos)
{
    int a = sconf.regionSize * regionX + 100;
    int b = sconf.regionSize * regionZ + 100;
    setSeed(784295783249ULL * a + 827828252345ULL * b + seed + sconf.salt);
    int x = nextIntRange(-50, 50) + a;
    int z = nextIntRange(-50, 50) + b;
    if (nextFloat() >= sconf.rarity)
       return 0;
    pos->x = x*16+4;
    pos->z = z*16+4;
    return 1;
}


static
uint64_t calcFitness(const Generator *g, int x, int z)
{
    int64_t np[6];
    uint32_t flags = SAMPLE_NO_DEPTH | SAMPLE_NO_BIOME;
    sampleBiomeNoise(&g->bn, np, x>>2, 0, z>>2, NULL, flags);
    const int64_t spawn_np[][2] = {
        {-10000,10000},{-10000,10000},{-1100,10000},{-10000,10000},{0,0},
        {-10000,-1600},{1600,10000} // [6]: weirdness for the second noise point
    };
    uint64_t ds = 0, ds1 = 0, ds2 = 0;
    uint64_t a, b, q, i;
    for (i = 0; i < 5; i++)
    {
        a = +np[i] - (uint64_t)spawn_np[i][1];
        b = -np[i] + (uint64_t)spawn_np[i][0];
        q = (int64_t)a > 0 ? a : (int64_t)b > 0 ? b : 0;
        ds += q * q;
    }
    a = +np[5] - (uint64_t)spawn_np[5][1];
    b = -np[5] + (uint64_t)spawn_np[5][0];
    q = (int64_t)a > 0 ? a : (int64_t)b > 0 ? b : 0;
    ds1 = ds + q*q;
    a = +np[5] - (uint64_t)spawn_np[6][1];
    b = -np[5] + (uint64_t)spawn_np[6][0];
    q = (int64_t)a > 0 ? a : (int64_t)b > 0 ? b : 0;
    ds2 = ds + q*q;
    ds = ds1 <= ds2 ? ds1 : ds2;
    // apply dependence on distance from origin
    a = (int64_t)x*x;
    b = (int64_t)z*z;
    if (g->mc < MC_1_21_60)
    {
        double s = (double)(a + b) / (2500 * 2500);
        q = (uint64_t)(s*s * 1e8) + ds;
    }
    else
    {
        q = ds * (2048LL * 2048LL) + a + b;
    }
    return q;
}

static
void findFittest(const Generator *g, Pos *pos, uint64_t *fitness, double maxrad, double step)
{
    double rad, ang;
    Pos p = *pos;
    for (rad = step; rad <= maxrad; rad += step)
    {
        for (ang = 0; ang <= PI*2; ang += step/rad)
        {
            int x = p.x + (int)(sin(ang) * rad);
            int z = p.z + (int)(cos(ang) * rad);
            uint64_t fit = calcFitness(g, x, z);
            // Then update pos and fitness if combined total is lower/better
            if (fit < *fitness)
            {
                pos->x = x;
                pos->z = z;
                *fitness = fit;
            }
        }
    }
}

static
Pos findFittestPos(const Generator *g)
{
    Pos spawn = {0, 0};
    uint64_t fitness = calcFitness(g, 0, 0);
    findFittest(g, &spawn, &fitness, 2048.0, 512.0);
    findFittest(g, &spawn, &fitness, 512.0, 32.0);
    // center of chunk
    spawn.x = (spawn.x & ~15) + 8;
    spawn.z = (spawn.z & ~15) + 8;
    return spawn;
}

// valid spawn biomes up to 1.17
int isSpawnBiome(int biome_id) {
    switch (biome_id) {
        case forest:
        case plains:
        case taiga:
        case taiga_hills:
        case wooded_hills:
        case jungle_hills:
        case jungle:
            return 1;
        default:
            return 0;
    }
}

static int surroundingBiomes(int *biomeIds, int grid, int x, int z)
{
    int idx = z * grid + x;
    return isSpawnBiome(biomeIds[idx       ]) &&
           isSpawnBiome(biomeIds[idx - grid]) &&
           isSpawnBiome(biomeIds[idx + grid]) &&
           isSpawnBiome(biomeIds[idx -    1]) &&
           isSpawnBiome(biomeIds[idx +    1]);
}

Pos estimateSpawn(const Generator *g, uint64_t *rng)
{
    Pos spawn = {0, 0};

    if (g->mc >= MC_1_18)
    {
        spawn = findFittestPos(g);
    }

    return spawn;
}

Pos getSpawn(const Generator *g)
{
    uint64_t rng;
    Pos spawn = estimateSpawn(g, &rng);
    int i, j, k, u, v, cx0, cz0;
    uint32_t ii, jj;

    if (g->mc <= MC_B1_7)
        return spawn;

    if (g->mc < MC_1_18)
    {
        const int grid = 10;
        int step = g->mc < MC_1_12 ? 0 : 10;
        int sx   = g->mc < MC_1_12 ? 0 : 1;

        Range r = {4, 0, 0, grid, grid, 0, 1};
        int *biomeIds = allocCache(g, r);

        if (!biomeIds) {
            free(biomeIds);
            return spawn;
        }

        while (1) {
            r.x = step;
            r.z = 0;
            int err = genBiomes(g, biomeIds, r);
            if (err != 0) {
                free(biomeIds);
                break;
            }

            for (int z = 1; z < grid-1; z++) {
                for (int x = sx; x < grid-1; x++) {
                    if (surroundingBiomes(biomeIds, grid, x, z)) {
                        spawn.x = 4 * (x + step);
                        spawn.z = 4 * z;
                        free(biomeIds);
                        return spawn;
                    }
                }
            }

            step += grid;
        }
        free(biomeIds);
    }

    SurfaceNoise sn;
    initSurfaceNoise(&sn, DIM_OVERWORLD, g->seed);

    if (g->mc >= MC_1_18)
    {
        j = k = u = 0;
        v = -1;
        for (i = 0; i < 121; i++)
        {
            if (j >= -5 && j <= 5 && k >= -5 && k <= 5)
            {
                // find server spawn point in chunk
                cx0 = (spawn.x & ~15) + j * 16;
                cz0 = (spawn.z & ~15) + k * 16;
                for (ii = 0; ii < 4; ii++)
                {
                    for (jj = 0; jj < 4; jj++)
                    {
                        float y;
                        int id;
                        int x = cx0 + ii * 4;
                        int z = cz0 + jj * 4;
                        mapApproxHeight(&y, &id, g, &sn, x >> 2, z >> 2, 1, 1);
                        if (y > 63 || id == frozen_ocean ||
                            id == deep_frozen_ocean || id == frozen_river)
                        {
                            spawn.x = x;
                            spawn.z = z;
                            return spawn;
                        }
                    }
                }
            }
            if (j == k || (j < 0 && j == -k) || (j > 0 && j == 1 - k))
            {
                int tmp = u;
                u = -v;
                v = tmp;
            }
            j += u;
            k += v;
        }
        // chunk center
        spawn.x = (spawn.x & ~15) + 8;
        spawn.z = (spawn.z & ~15) + 8;
    }

    return spawn;
}



//==============================================================================
// Validating Structure Positions
//==============================================================================


int isViableFeatureBiome(int mc, int structureType, int biomeID)
{
    switch (structureType)
    {
    case Desert_Pyramid:
        return biomeID == desert || biomeID == desert_hills;

    case Jungle_Pyramid:
        return biomeID == jungle || biomeID == jungle_hills;

    case Swamp_Hut:
        return biomeID == swamp || biomeID == swamp_hills;

    case Igloo:
        return biomeID == snowy_plains || biomeID == snowy_taiga || biomeID == snowy_slopes;

    case Ocean_Ruin:
        if (mc <= MC_1_2) return 0;
        return isOceanic(biomeID);

    case Shipwreck:
        if (mc <= MC_1_2) return 0;
        if (biomeID == deep_warm_ocean) return 0;
        return isOceanic(biomeID) || biomeID == beach || biomeID == snowy_beach || biomeID == mushroom_field_shore;

    case Ruined_Portal:
    case Ruined_Portal_N:
        return mc >= MC_1_16;

    case Ancient_City:
        if (mc <= MC_1_18) return 0;
        return biomeID == deep_dark;

    case Trail_Ruins:
        if (mc <= MC_1_19) return 0;
        else {
            switch (biomeID) {
            case taiga:
            case snowy_taiga:
            case old_growth_pine_taiga:
            case old_growth_spruce_taiga:
            case old_growth_birch_forest:
            case jungle:
                return 1;
            default:
                return 0;
            }
        }

    case Trial_Chambers:
        if (mc <= MC_1_20) return 0;
        return biomeID != deep_dark && isOverworld(mc, biomeID);

    case Abandoned_Camp:
        if (mc <= MC_26_30) return 0;
        else {
            switch (biomeID) {
            case savanna:
            case flower_forest:
            case birch_forest:
            case forest:
            case snowy_taiga:
            case bamboo_jungle:
            case sparse_jungle:
            case cherry_grove:
            case meadow:
            case old_growth_birch_forest:
            case old_growth_spruce_taiga:
            case old_growth_pine_taiga:
            case swamp:
            case taiga:
            case windswept_forest:
            case dappled_forest:
            case wooded_badlands:
            case pale_garden:
                return 1;
            default:
                return 0;
            }
        }

    case Treasure:
        if (mc <= MC_1_2) return 0;
        return biomeID == beach || biomeID == snowy_beach || biomeID == stony_shore || biomeID == mushroom_field_shore;

    case Mineshaft:
    case Ravine:
    case Stronghold:
        return isOverworld(mc, biomeID);

    case Lava_Lake:
        return isOverworld(mc, biomeID) && !isOceanic(biomeID) && getCategory(mc, biomeID) != river;

    case Desert_Well:
        return biomeID == desert;

    case Monument:
        return isDeepOcean(biomeID);

    case Outpost:
        if (mc <= MC_1_11) return 0;
        if (mc >= MC_1_18) {
            switch (biomeID) {
            case desert:
            case plains:
            case savanna:
            case snowy_plains:
            case taiga:
            case snowy_taiga:
            case meadow:
            case sunflower_plains:
            case frozen_peaks:
            case jagged_peaks:
            case stony_peaks:
            case snowy_slopes:
            case grove:
            case cherry_grove:
                return 1;
            default:
                return 0;
            }
        }
        return biomeID == desert || biomeID == plains || biomeID == savanna || biomeID == taiga;

    case Village:
        if (biomeID == plains || biomeID == desert || biomeID == savanna || biomeID == snowy_plains)
            return 1;
        if (mc >= MC_1_11 && (biomeID == taiga || biomeID == snowy_taiga || biomeID == sunflower_plains))
            return 1;
        if (mc >= MC_1_18 && biomeID == meadow)
            return 1;
        return 0;

    case Mansion:
        if (mc <= MC_1_0) return 0;
        if (mc >= MC_1_21_60 && biomeID == pale_garden) return 1;
        return (biomeID == dark_forest || biomeID == dark_forest_hills /*||
                biomeID == lush_caves  || biomeID == dripstone_caves*/);

    case Fortress:
        return (biomeID == nether_wastes || biomeID == soul_sand_valley ||
                biomeID == warped_forest || biomeID == crimson_forest ||
                biomeID == basalt_deltas);

    case Bastion:
        if (mc <= MC_1_14) return 0;
        return (biomeID == nether_wastes || biomeID == soul_sand_valley ||
                biomeID == warped_forest || biomeID == crimson_forest);

    case End_City:
        return biomeID == end_midlands || biomeID == end_highlands;

    case End_Gateway:
        return 1;// gateways are always valid, but check height instead

    default:
        fprintf(stderr,
                "isViableFeatureBiome: not implemented for structure type %d.\n",
                structureType);
        exit(1);
    }
    return 0;
}


static int mapViableBiome(const Layer * l, int * out, int x, int z, int w, int h)
{
    int err = mapBiome(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    int styp = ((const int*) l->data)[0];
    int i, j;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int biomeID = out[i + w*j];
            switch (styp)
            {
            case Desert_Pyramid:
            case Desert_Well:
                if (biomeID == desert || isMesa(biomeID))
                    return 0;
                break;
            case Jungle_Pyramid:
                if (biomeID == jungle)
                    return 0;
                break;
            case Swamp_Hut:
                if (biomeID == swamp)
                    return 0;
                break;
            case Igloo:
                if (biomeID == snowy_plains || biomeID == snowy_taiga)
                    return 0;
                break;
            case Treasure:
                if (isOceanic(biomeID))
                    return 0;
                break;
            case Ocean_Ruin:
            case Shipwreck:
            case Monument:
                if (isOceanic(biomeID))
                    return 0;
                break;
            case Mansion:
                if (biomeID == dark_forest || (l->mc > MC_1_21_50 && biomeID == pale_garden))
                    return 0;
                break;
            default:
                return 0;
            }
        }
    }

    return 1; // required biomes not found: set err status to stop generator
}

static int mapViableShore(const Layer * l, int * out, int x, int z, int w, int h)
{
    int err = mapShore(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    int styp = ((const int*) l->data)[0];
    int mc   = ((const int*) l->data)[1];
    int i, j;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int biomeID = out[i + w*j];
            switch (styp)
            {
            case Desert_Pyramid:
            case Jungle_Pyramid:
            case Swamp_Hut:
            case Igloo:
            case Ocean_Ruin:
            case Shipwreck:
            case Village:
            case Monument:
            case Mansion:
            case Treasure:
            case Desert_Well:
                if (isViableFeatureBiome(mc, styp, biomeID))
                    return 0;
                break;

            default:
                return 0;
            }
        }
    }

    return 1;
}


static const uint64_t g_monument_biomes2 =
    (1ULL << deep_frozen_ocean) |
    (1ULL << deep_cold_ocean) |
    (1ULL << deep_ocean) |
    (1ULL << deep_lukewarm_ocean) |
    (1ULL << deep_warm_ocean);

static const uint64_t g_monument_biomes1 =
    (1ULL << ocean) |
    (1ULL << deep_ocean) |
    (1ULL << river) |
    (1ULL << frozen_river) |
    (1ULL << frozen_ocean) |
    (1ULL << deep_frozen_ocean) |
    (1ULL << cold_ocean) |
    (1ULL << deep_cold_ocean) |
    (1ULL << lukewarm_ocean) |
    (1ULL << deep_lukewarm_ocean) |
    (1ULL << warm_ocean) |
    (1ULL << deep_warm_ocean);

static const uint64_t g_village_biomes =
    (1ULL << plains) |
    (1ULL << savanna) |
    (1ULL << desert) |
    (1ULL << taiga) |
    (1ULL << snowy_plains) |
    (1ULL << snowy_taiga);

static const uint64_t g_treasure_biomes =
    (1ULL << beach) |
    (1ULL << snowy_beach) |
    (1ULL << stony_shore) |
    (1ULL << mushroom_field_shore);

static int monumentNearby(Generator *g, int chunkX, int chunkZ)
{
    int x0 = chunkX-5, x1 = chunkX+4;
    int z0 = chunkZ-5, z1 = chunkZ+4;
    StructureConfig sconf;
    if (!getStructureConfig(Monument, g->mc, &sconf))
        return 0;
    Pos rmin = chunkToRegion(x0, z0, sconf.regionSize);
    Pos rmax = chunkToRegion(x1, z1, sconf.regionSize);
    for (int rz = rmin.z; rz <= rmax.z; rz++)
    {
        for (int rx = rmin.x; rx <= rmax.x; rx++)
        {
            Pos p;
            if (!getStructurePos(Monument, g->mc, g->seed, rx, rz, &p))
                continue;
            int mcx = p.x >> 4, mcz = p.z >> 4;
            if (mcx < x0 || mcx > x1 || mcz < z0 || mcz > z1)
                continue;
            if (isViableStructurePos(Monument, g, p.x, p.z, 0))
                return 1;
        }
    }
    return 0;
}

int isViableStructurePos(int structureType, Generator *g, int x, int z, uint32_t flags)
{
    int approx = 0; // enables approximation levels
    int viable = 0;

    int64_t chunkX = x >> 4;
    int64_t chunkZ = z >> 4;

    int sampleX, sampleZ, sampleY;
    int id;


    if (g->dim == DIM_NETHER)
    {
        if (structureType == Fortress && g->mc < MC_1_16)
            return 1;
        if (g->mc <= MC_1_14)
            return 0;
        if (structureType == Ruined_Portal_N)
            return 1;
        if (structureType == Fortress)
        {   // in 1.18+ fortresses generate everywhere, where bastions don't
            StructureConfig sc;
            if (!getStructureConfig(Fortress, g->mc, &sc))
                return 0;
            Pos rp = {
                floordiv(x, sc.regionSize << 4),
                floordiv(z, sc.regionSize << 4)
            };
            if (!getStructurePos(Bastion, g->mc, g->seed, rp.x, rp.z, &rp))
                return 1;
            return !isViableStructurePos(Bastion, g, x, z, flags);
        }
        sampleY = 0;
        if (g->mc >= MC_1_16 && structureType == Bastion)
        {
            StructureVariant sv;
            getVariant(&sv, Bastion, g->mc, g->seed, x, z, -1);
            sampleX = (chunkX*32 + 2*sv.x + sv.sx-1) / 2 >> 2;
            sampleZ = (chunkZ*32 + 2*sv.z + sv.sz-1) / 2 >> 2;
            if (g->mc >= MC_1_19)
                sampleY = 33 >> 2; // nether biomes don't actually vary in Y
        }
        else
        {
            sampleX = (chunkX * 4) + 2;
            sampleZ = (chunkZ * 4) + 2;
        }
        id = getBiomeAt(g, 4, sampleX, sampleY, sampleZ);
        return isViableFeatureBiome(g->mc, structureType, id);
    }
    else if (g->dim == DIM_END)
    {
        switch (structureType)
        {
        case End_City:
            if (g->mc <= MC_1_0) return 0;
            break;
        case End_Gateway:
            if (g->mc <= MC_1_0) return 0;
            break;
        default:
            return 0;
        }
        // End biomes vary only on a per-chunk scale (1:16)
        // voronoi pre-1.15 shouldn't matter for End Cities as the check will
        // be near the chunk center
        id = getBiomeAt(g, 16, chunkX, 0, chunkZ);
        return isViableFeatureBiome(g->mc, structureType, id) ? id : 0;
    }

    // Overworld

    Layer lbiome, lshore, *entry = 0;
    int data[2] = { structureType, g->mc };

    if (g->mc < MC_1_18)
    {
        lbiome = g->ls.layers[L_BIOME_256];
        lshore = g->ls.layers[L_SHORE_16];
        entry = g->entry;

        g->ls.layers[L_BIOME_256].data = (void*) data;
        g->ls.layers[L_BIOME_256].getMap = mapViableBiome;
        g->ls.layers[L_SHORE_16].data = (void*) data;
        g->ls.layers[L_SHORE_16].getMap = mapViableShore;
    }

    switch (structureType)
    {
    case Trail_Ruins:
        if (g->mc <= MC_1_19) goto L_not_viable;
        goto L_feature;
    case Ocean_Ruin:
    {
        if (g->mc <= MC_1_2) goto L_not_viable;
        if (monumentNearby(g, chunkX, chunkZ)) goto L_not_viable;
        goto L_feature;
    }
    case Shipwreck:
    {
        if (g->mc <= MC_1_2) goto L_not_viable;
        int id = getBiomeAt(g, 4, x >> 2, 63 >> 2, z >> 2);
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        int radius = (id == beach || id == snowy_beach || id == mushroom_field_shore) ? 10 : 20;
        uint64_t b = (1ULL << id);
        if (!areBiomesViable(g, x, 63, z, radius, b, 0, 0) ||
             monumentNearby(g, chunkX, chunkZ)
            ) goto L_not_viable;
        viable = id;
        goto L_viable;
    }

    case Treasure:
    {
        if (g->mc < MC_1_18)
            g->entry = &g->ls.layers[L_RIVER_MIX_4];
        if (!areBiomesViable(g, x, 319, z, 3, g_treasure_biomes, 0, 0))
            goto L_not_viable;
        goto L_viable;
    }
    case Igloo:
        if (g->mc <= MC_1_8) goto L_not_viable;
        goto L_feature;
    case Desert_Pyramid:
    case Jungle_Pyramid:
    case Swamp_Hut:
L_feature:
        if (g->mc <= MC_1_14)
        {
            g->entry = &g->ls.layers[L_VORONOI_1];
            sampleX = chunkX * 16 + 9;
            sampleZ = chunkZ * 16 + 9;
        }
        else
        {
            if (g->mc < MC_1_18)
                g->entry = &g->ls.layers[L_RIVER_MIX_4];
            sampleX = chunkX * 4 + 2;
            sampleZ = chunkZ * 4 + 2;
        }
        id = getBiomeAt(g, 0, sampleX, 319>>2, sampleZ);
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        goto L_viable;

    case Desert_Well:
        if (g->mc <= MC_1_14)
        {
            g->entry = &g->ls.layers[L_VORONOI_1];
            sampleX = x;
            sampleZ = z;
        }
        else
        {
            if (g->mc < MC_1_18)
                g->entry = &g->ls.layers[L_RIVER_MIX_4];
            sampleX = x >> 2;
            sampleZ = z >> 2;
        }
        id = getBiomeAt(g, 0, sampleX, 319>>2, sampleZ);
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        goto L_viable;

    case Village:
    {
        uint64_t m = (1ULL << (meadow - 128) | 1ULL << (sunflower_plains - 128));
        if (!areBiomesViable(g, x, 319, z, 2, g_village_biomes, m, 0))
            goto L_not_viable;
        viable = getBiomeAt(g, 0, x >> 2, 319>>2, z >> 2);
        goto L_viable;
    }

    case Outpost:
    {
        if (g->mc <= MC_1_13)
            goto L_not_viable;
        if (g->mc >= MC_1_18)
        {
            sampleX = (chunkX * 16 + 8) >> 2;
            sampleZ = (chunkZ * 16 + 8) >> 2;
        }
        else if (g->mc >= MC_1_16)
        {
            g->entry = &g->ls.layers[L_RIVER_MIX_4];
            sampleX = chunkX * 4 + 2;
            sampleZ = chunkZ * 4 + 2;
        }
        else
        {
            g->entry = &g->ls.layers[L_VORONOI_1];
            sampleX = chunkX * 16 + 9;
            sampleZ = chunkZ * 16 + 9;
        }
        id = getBiomeAt(g, 0, sampleX, 319>>2, sampleZ);
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        goto L_viable;
    }

    case Monument:
        if (g->mc < MC_1_0)
            goto L_not_viable;
        else if (g->mc == MC_1_8)
        {   // In 1.8 monuments require only a single deep ocean block.
            id = getBiomeAt(g, 1, chunkX * 16 + 8, 0, chunkZ * 16 + 8);
            if (id < 0 || !isDeepOcean(id))
                goto L_not_viable;
        }
        else if (g->mc < MC_1_18)
        {   // Monuments require two viability checks with the ocean layer
            // branch => worth checking for potential deep ocean beforehand.
            g->entry = &g->ls.layers[L_SHORE_16];
            id = getBiomeAt(g, 0, chunkX, 0, chunkZ);
            if (id < 0 || !isDeepOcean(id))
                goto L_not_viable;
        }
        sampleX = chunkX * 16 + 8;
        sampleZ = chunkZ * 16 + 8;
        if (g->mc >= MC_1_9 && g->mc < MC_1_18)
        {   // check for deep ocean center
            if (!areBiomesViable(g, sampleX, 63, sampleZ, 16, g_monument_biomes2, 0, approx))
                goto L_not_viable;
        }
        else if (g->mc >= MC_1_18)
        {   // check is done at y level of ocean floor - approx. with y = 36
            id = getBiomeAt(g, 4, sampleX>>2, 36>>2, sampleZ>>2);
            if (!isDeepOcean(id))
                goto L_not_viable;
        }
        if (areBiomesViable(g, sampleX, 63, sampleZ, 29, g_monument_biomes1, 0, approx))
            goto L_viable;
        goto L_not_viable;

    case Mansion:
        if (g->mc < MC_1_0)
            goto L_not_viable;
        else
        {
            sampleX = chunkX * 16 + 8;
            sampleZ = chunkZ * 16 + 8;
            uint64_t b = (1ULL << dark_forest);
            uint64_t m = (1ULL << (dark_forest_hills - 128)/* |
                          1ULL << (lush_caves - 128) |
                          1ULL << (dripstone_caves - 128)*/);
            if (g->mc < MC_1_18)
            {
                if (!areBiomesViable(g, sampleX, 64, sampleZ, 32, b, m, approx))
                    goto L_not_viable;
            }
            else
            {
                if (g->mc >= MC_1_21_60)
                    m |= (1ULL << (pale_garden - 128));
                SurfaceNoise sn;
                initSurfaceNoise(&sn, DIM_OVERWORLD, g->seed);
                float height;
                int biome_id;
                mapApproxHeight(&height, &biome_id, g, &sn, x >> 2, z >> 2, 1, 1);
                if (!areBiomesViable(g, sampleX, (int)height>>2, sampleZ, 32, b, m, approx))
                    goto L_not_viable;
            }
        }
        goto L_viable;

    case Ruined_Portal:
    case Ruined_Portal_N:
        if (g->mc <= MC_1_14)
            goto L_not_viable;
        goto L_viable;

    case Geode:
        if (g->mc <= MC_1_16)
            goto L_not_viable;
        goto L_viable;

    case Dungeon:
        if (g->mc < MC_1_18)
            goto L_not_viable;
        goto L_viable;

    case Ancient_City:
        if (g->mc <= MC_1_18) goto L_not_viable;
        id = getBiomeAt(g, 0, x>>2, -27>>2, z>>2);
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        goto L_viable;

    case Trial_Chambers:
        if (g->mc <= MC_1_20) goto L_not_viable;
L_jigsaw:
        {
            StructureVariant sv;
            getVariant(&sv, structureType, g->mc, g->seed, x, z, -1);
            sampleX = (chunkX*32 + 2*sv.x + sv.sx - 1) / 2 >> 2;
            sampleZ = (chunkZ*32 + 2*sv.z + sv.sz - 1) / 2 >> 2;
            sampleY = sv.y >> 2;
            id = getBiomeAt(g, 4, sampleX, sampleY, sampleZ);
        }
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        goto L_viable;
    
    case Abandoned_Camp:
        if (g->mc <= MC_26_30) goto L_not_viable;
        id = getBiomeAt(g, 0, x >> 2, 319>>2, z >> 2);
        if (id < 0 || !isViableFeatureBiome(g->mc, structureType, id))
            goto L_not_viable;
        viable = id;
        goto L_viable;


    case Mineshaft:
    case Stronghold:
        goto L_viable;

    case Ravine:
        if (g->mc < MC_1_18 || g->mc >= MC_1_21_60)
            goto L_viable;
        id = getBiomeAt(g, 4, x>>2, 319>>2, z>>2);
        if (id < 0 || isOceanic(id))
            goto L_not_viable;
        goto L_viable;

    case Lava_Lake:
        {
            id = getBiomeAt(g, 4, x>>2, 319>>2, z>>2);
            if (id < 0 || isOceanic(id))
               goto L_not_viable;

            if (g->mc >= MC_1_0)
            {
               SurfaceNoise sn;
               initSurfaceNoise(&sn, DIM_OVERWORLD, g->seed);
               setPopulationSeed(g->seed, chunkX, chunkZ);
               if (g->mc < MC_1_18 && (id != desert || id != desert_hills))
               {
                   //water lakes
                   if (nextInt(4) == 0)
                       goto L_not_viable;
               }
               skipNextN(2);
               int i = nextIntRange(8, 128);
               int lava_y = nextInt(i);
               float height;
               int biome_id;
               mapApproxHeight(&height, &biome_id, g, &sn, x >> 2, z >> 2, 1, 1);
               if (lava_y >= (int)height)
                   goto L_not_viable;
            }

            return 1;
        }

    default:
        fprintf(stderr,
                "isViableStructurePos: bad structure type %d or dimension %d\n",
                structureType, g->dim);
        goto L_not_viable;
    }

L_viable:
    if (!viable)
        viable = 1;
L_not_viable:
    if (g->mc < MC_1_18)
    {
        g->ls.layers[L_BIOME_256] = lbiome;
        g->ls.layers[L_SHORE_16] = lshore;
        g->entry = entry;
    }
    return viable;
}


int isViableStructureTerrain(int structType, Generator *g, int x, int z)
{
    int sx, sz;
    if (g->mc < MC_1_18)
        return 1;
    if (structType == Desert_Pyramid || structType == Jungle_Temple)
    {
        sx = (structType == Desert_Pyramid ? 21 : 12);
        sz = (structType == Desert_Pyramid ? 21 : 15);
    }
    // else if (structType == Mansion)
    // {
    //     int cx = x >> 4, cz = z >> 4;
    //     Pos region = chunkToRegion(cx, cz, 80);
    //     setRegionSeed(g->seed, region.x, region.z, 10387319);
    //     skipNextN(4);
    //     int rot = nextInt(4);
    //     sx = 5;
    //     sz = 5;
    //     if (rot == 0) { sx = -5; }
    //     if (rot == 1) { sx = -5; sz = -5; }
    //     if (rot == 2) { sz = -5; }
    //     x = cx * 16 + 7;
    //     z = cz * 16 + 7;
    // }
    else
    {
        return 1;
    }

    // approx surface height using depth parameter (0.5 ~ sea level)
    double corners[][2] = {
        {(x+ 0)/4.0, (z+ 0)/4.0},
        {(x+sx)/4.0, (z+sz)/4.0},
        {(x+ 0)/4.0, (z+sz)/4.0},
        {(x+sx)/4.0, (z+ 0)/4.0},
    };
    int nptype = g->bn.nptype;
    int i, ret = 1;
    g->bn.nptype = NP_DEPTH;
    for (i = 0; i < 4; i++)
    {
        double depth = sampleClimatePara(&g->bn, 0, corners[i][0], corners[i][1]);
        if (depth < 0.48)
        {
            ret = 0;
            break;
        }
    }
    g->bn.nptype = nptype;
    return ret;
}


/* Given bordering noise columns and a fractional position between those,
 * determine the surface block height (i.e. where the interpolated noise > 0).
 * Note that the noise columns should be of size: ncolxz[ colheight+1 ]
 */
int getSurfaceHeight(
        const double ncol00[], const double ncol01[],
        const double ncol10[], const double ncol11[],
        int colymin, int colymax, int blockspercell, double dx, double dz);

void sampleNoiseColumnEnd(double column[], const SurfaceNoise *sn,
        const EndNoise *en, int x, int z, int colymin, int colymax);

int isViableEndCityTerrain(const Generator *g, const SurfaceNoise *sn,
        int blockX, int blockZ)
{
    const EndNoise *en = &g->en;
    int chunkX = blockX >> 4;
    int chunkZ = blockZ >> 4;
    blockX = chunkX * 16 + 7;
    blockZ = chunkZ * 16 + 7;

    int x0 = blockX, z0 = blockZ;
    int x1, z1, x2, z2, x3, z3;

    setSeed(chunkX + chunkZ * 10387313ULL);

    switch (nextInt(4))
    {
    case 0: // (++) 0
        x1 = blockX;     z1 = blockZ + 5;
        x2 = blockX + 5; z2 = blockZ;
        x3 = blockX + 5; z3 = blockZ + 5;
        break;
    case 1: // (-+) 90
        x1 = blockX;     z1 = blockZ + 5;
        x2 = blockX - 5; z2 = blockZ;
        x3 = blockX - 5; z3 = blockZ + 5;
        break;
    case 2: // (--) 180
        x1 = blockX;     z1 = blockZ - 5;
        x2 = blockX - 5; z2 = blockZ;
        x3 = blockX - 5; z3 = blockZ - 5;
        break;
    case 3: // (+-) 270
        x1 = blockX;     z1 = blockZ - 5;
        x2 = blockX + 5; z2 = blockZ;
        x3 = blockX + 5; z3 = blockZ - 5;
        break;
    default:
        return 0; // error
    }
    float h[4];
    mapEndSurfaceHeight(&h[0], en, sn, x0, z0, 1, 1, 1, 0);
    mapEndSurfaceHeight(&h[1], en, sn, x1, z1, 1, 1, 1, 0);
    mapEndSurfaceHeight(&h[2], en, sn, x2, z2, 1, 1, 1, 0);
    mapEndSurfaceHeight(&h[3], en, sn, x3, z3, 1, 1, 1, 0);
    // printf("%f %d %d\n", h[0], x0, z0);
    // printf("%f %d %d\n", h[1], x1, z1);
    // printf("%f %d %d\n", h[2], x2, z2);
    // printf("%f %d %d\n", h[3], x3, z3);
    if (h[1] < h[0]) h[0] = h[1];
    if (h[2] < h[0]) h[0] = h[2];
    if (h[3] < h[0]) h[0] = h[3];
    return (int)h[0]+1 >= 60 ? (int)h[0] : 0;
}


//==============================================================================
// Finding Properties of Structures
//==============================================================================


STRUCT(PieceEnv)
{
    Piece *list;
    int *n;
    uint64_t *rng;
    int *ship;
    int y;
    int typlast;
    int nmax;
    int ntyp[PIECE_COUNT];
};

typedef int (piecefunc_t)(PieceEnv *env, Piece *current, int depth);

static piecefunc_t genTower;
static piecefunc_t genBridge;
static piecefunc_t genHouseTower;
static piecefunc_t genFatTower;

static void moveBelowSeaLevel(Piece *list, int count, int seaLevel, int minWorldHeight, int offset);
static void moveInsideHeights(Piece *list, int count, int minY, int maxY);
static void offsetPiecesVertically(Piece *list, int count, int dy);


int getVariant(StructureVariant *r, int structType, int mc, uint64_t seed,
        int x, int z, int biomeID)
{
    int t;
    char sx, sy, sz;
    StructureConfig sc;
    getStructureConfig(structType, mc, &sc);

    memset(r, 0, sizeof(*r));
    r->start = -1;
    r->biome = -1;
    r->y = 320;
    int cx = x >> 4;
    int cz = z >> 4;
    Pos rpos = chunkToRegion(cx, cz, sc.regionSize);

    switch (structType)
    {
    case Village:
        if (mc < MC_1_11)
        {
            int regX = cx < 0 ? cx - 40+1 : cx;
            int regZ = cz < 0 ? cz - 40+1 : cz;
            setRegionSeed(seed, regX, regZ, 10387312);
            r->abandoned = nextInt(sc.rarity) == 0;
            return 0;
        }
        if (!isViableFeatureBiome(mc, Village, biomeID))
            return 0;
        r->biome = biomeID;
        setRegionSeed(seed, rpos.x, rpos.z, sc.salt);
        skipNextN(4);
        r->rotation = nextInt(4);
        r->abandoned = nextFloat() < sc.rarity;
        switch (biomeID)
        {
        case meadow:
        case sunflower_plains:
            r->biome = plains;
            // fallthrough
        case plains:
            t = nextInt(1+1+1+1);
            if      (t < 1) { r->start = 0; sx =  9; sy = 4; sz =  9; } // plains_fountain_01
            else if (t < 2) { r->start = 1; sx = 10; sy = 7; sz = 10; } // plains_meeting_point_1
            else if (t < 3) { r->start = 2; sx =  8; sy = 5; sz = 15; } // plains_meeting_point_2
            else if (t < 4) { r->start = 3; sx = 11; sy = 9; sz = 11; } // plains_meeting_point_3
            else  UNREACHABLE();
            break;
        case desert:
            t = nextInt(3+3+1);
            if      (t < 3) { r->start = 1; sx = 17; sy = 6; sz =  9; } // desert_meeting_point_1
            else if (t < 6) { r->start = 2; sx = 12; sy = 6; sz = 12; } // desert_meeting_point_2
            else if (t < 7) { r->start = 3; sx = 15; sy = 6; sz = 15; } // desert_meeting_point_3
            else  UNREACHABLE();
            break;
            break;
        case savanna:
            t = nextInt(2+1+3+3);
            if      (t < 2) { r->start = 1; sx = 14; sy = 5; sz = 12; } // savanna_meeting_point_1
            else if (t < 3) { r->start = 2; sx = 11; sy = 6; sz = 11; } // savanna_meeting_point_2
            else if (t < 6) { r->start = 3; sx =  9; sy = 6; sz = 11; } // savanna_meeting_point_3
            else if (t < 9) { r->start = 4; sx =  9; sy = 6; sz =  9; } // savanna_meeting_point_4
            else  UNREACHABLE();
            break;
        case snowy_taiga:
            r->biome = taiga;
            // fallthrough
        case taiga:
            t = nextInt(2+2);
            if      (t < 2) { r->start = 1; sx = 22; sy = 3; sz = 18; } // taiga_meeting_point_1
            else if (t < 4) { r->start = 2; sx =  9; sy = 7; sz =  9; } // taiga_meeting_point_2
            else  UNREACHABLE();
            break;
        case snowy_plains:
            t = nextInt(2+1+3);
            if      (t < 2) { r->start = 1; sx = 12; sy = 8; sz =  8; } // snowy_meeting_point_1
            else if (t < 3) { r->start = 2; sx = 11; sy = 5; sz =  9; } // snowy_meeting_point_2
            else if (t < 6) { r->start = 3; sx =  7; sy = 7; sz =  7; } // snowy_meeting_point_3
            else  UNREACHABLE();
            break;
        default:
            sx = sy = sz = 0;
            return 0;
        }
        goto L_rotate_village_bastion;

    case Outpost:
        setRegionSeed(seed, rpos.x, rpos.z, sc.salt);
        skipNextN(4);
        r->rotation = nextInt(4);
        sx = sz = 15;
        sy = 21;
        goto L_rotate_village_bastion;

    case Bastion:
        setRegionSeed(seed, rpos.x, rpos.z, sc.salt);
        skipNextN(3);
        r->rotation = nextInt(4);
        r->start = 3 - nextInt(4); // ooposite 0->3, 1->2, 2->1, 3->0
        switch (r->start)
        {
        case 0: sx = 46; sy = 24; sz = 46; break; // units/air_base
        case 1: sx = 30; sy = 24; sz = 48; break; // hoglin_stable/air_base
        case 2: sx = 38; sy = 48; sz = 38; break; // treasure/big_air_full
        case 3: sx = 16; sy = 32; sz = 32; break; // bridge/starting_pieces/entrance_base
        }
    L_rotate_village_bastion:
        r->sy = sy;
        if (mc >= MC_1_18)
        {
            switch (r->rotation)
            { // 0:0, 1:cw90, 2:cw180, 3:cw270=ccw90
            case 0: r->x = 0;    r->z = 0;    r->sx = sx; r->sz = sz; break;
            case 1: r->x = 1-sz; r->z = 0;    r->sx = sz; r->sz = sx; break;
            case 2: r->x = 1-sx; r->z = 1-sz; r->sx = sx; r->sz = sz; break;
            case 3: r->x = 0;    r->z = 1-sx; r->sx = sz; r->sz = sx; break;
            }
        }
        else
        {
            switch (r->rotation)
            { // 0:0, 1:cw90, 2:cw180, 3:cw270=ccw90
            case 0: r->x = 0;        r->z = 0;        r->sx = sx; r->sz = sz; break;
            case 1: r->x = (x<0)-sz; r->z = 0;        r->sx = sz; r->sz = sx; break;
            case 2: r->x = (x<0)-sx; r->z = (z<0)-sz; r->sx = sx; r->sz = sz; break;
            case 3: r->x = 0;        r->z = (z<0)-sx; r->sx = sz; r->sz = sx; break;
            }
        }
        return 1;

    case Ancient_City:
        setRegionSeed(seed, rpos.x, rpos.z, sc.salt);
        r->rotation = nextInt(4);
        r->start = 1 + nextInt(3); // city_center_1..3
        sx = 18; sy = 31; sz = 41;
        switch (r->rotation)
        { // 0:0, 1:cw90, 2:cw180, 3:cw270=ccw90
        case 0: x = -(x>0);    z = -(z>0);    r->sx = sx; r->sz = sz; break;
        case 1: x = +(x<0)-sz; z = -(z>0);    r->sx = sz; r->sz = sx; break;
        case 2: x = +(x<0)-sx; z = +(z<0)-sz; r->sx = sx; r->sz = sz; break;
        case 3: x = -(x>0);    z = +(z<0)-sx; r->sx = sz; r->sz = sx; break;
        }
        // note the city_anchor (13, *, 20) is part of the city_center
        sx = 13; sz = 20; // city_anchor
        switch (r->rotation)
        { // 0:0, 1:cw90, 2:cw180, 3:cw270=ccw90
        case 0: r->x = x-sx; r->z = z-sz; break; // 0:0
        case 1: r->x = x+sz; r->z = z-sx; break; // 1:cw90
        case 2: r->x = x+sx; r->z = z+sz; break; // 2:cw180
        case 3: r->x = x-sz; r->z = z+sx; break; // 3:cw270=ccw90
        }
        r->y = -27;
        r->sy = sy;
        return 1;

    case Ruined_Portal:
    case Ruined_Portal_N:
        // Ruined portals are split into 7 types that generate independenly
        // from one another, each in a certain set of biomes. Together they
        // cover each biome once (save for the deep_dark) and have no terrain
        // restrictions, so a ruined portal *should* always generate in each
        // region. However, in locations with underground biomes, a ruined
        // portal can fail to generate, or possibly have two ruined portals
        // above one another, because the biome check is done after selecting
        // the portal type and generation height, and can therefore vertically
        // move into unsupported biomes. Testing for this case requires the
        // surface height and is therefore not supported.
        {
            setPopulationSeed(seed, cx, cz);
            int cat = getCategory(mc, biomeID);
            switch (cat)
            {
            case desert:
            case jungle:
            case swamp:
            case ocean:
            case nether_wastes:
                r->biome = cat;
                break;
            }
            if (r->biome == -1)
            {
                switch (biomeID)
                {
                case mangrove_swamp:
                    r->biome = swamp;
                    break;
                case mountains:                     // windswept_hills
                case mountain_edge:
                case wooded_mountains:              // windswept_forest
                case gravelly_mountains:            // windswept_gravelly_hills
                case modified_gravelly_mountains:
                case savanna_plateau:
                case shattered_savanna:             // windswept_savanna
                case shattered_savanna_plateau:
                case badlands:
                case eroded_badlands:
                case wooded_badlands_plateau:       // wooded_badlands
                case modified_badlands_plateau:
                case modified_wooded_badlands_plateau:
                case snowy_taiga_mountains:
                case taiga_mountains:
                case stony_shore:
                case meadow:
                case frozen_peaks:
                case jagged_peaks:
                case stony_peaks:
                case snowy_slopes:
                    r->biome = mountains;
                    break;
                }
            }
            if (r->biome == -1)
                r->biome = plains;
        }
        if (mc < MC_1_18)
            // this value was obtained by brute force
            skipNextN(40);
        r->underground = nextFloat() < 0.5;
        r->airpocket = r->underground;
        r->rotation = nextInt(4);
        r->mirror = 0.5 < nextFloat();
        r->giant = nextFloat() < 0.05;
        if (r->giant)
        {   // ruined_portal/giant_portal_1..3
            r->start = 1 + nextInt(3);
        }
        else
        {   // ruined_portal/portal_1..10
            r->start = 1 + nextInt(10);
        }
        return 1;

    case Monument:
        r->x = r->z = -29;
        r->sx = r->sz = 58;
        return 1;
    
    case Ocean_Ruin:
        float largeProb = 0, clusterProb = 0;
        switch (biomeID)
        {
        // warm
        case warm_ocean:
        case lukewarm_ocean:
        case deep_warm_ocean:
        case deep_lukewarm_ocean:
            largeProb = 0.3f;
            clusterProb = 0.5f;
            break;
        // cold
        case ocean:
        case frozen_ocean:
        case cold_ocean:
            largeProb = 0.3f;
            clusterProb = 0.25f;
            break;
        // deep cold
        case deep_ocean:
        case deep_cold_ocean:
        case deep_frozen_ocean:
            largeProb = 0.5f;
            clusterProb = 0.4f;
            break;
        default:
            return 0;
        }
        StructureConfig msconf;
        getStructureConfig(Monument, mc, &msconf);
        Pos region = chunkToRegion(x >> 4, z >> 4, msconf.regionSize);
        setRegionSeed(seed, region.x, region.z, msconf.salt);
        skipNextN(4);  // offset random
        r->rotation = nextInt(4);
        int isLarge = nextFloat() <= largeProb;
        skipNextN(1); // rotation
        int hasCluster = nextFloat() <= clusterProb;
        if (hasCluster)
        {
            hasCluster = 1;            
            skipNextN(16);
            r->size = 4 + nextInt(5); // 4-8
        }
        r->large = isLarge;
        r->cluster = hasCluster;
        r->biome = biomeID;
        return 1;

    case Igloo:
        setPopulationSeed(seed, cx, cz);
        r->rotation = nextInt(4);
        r->basement = nextFloat() >= 0.5;
        r->size = nextInt(8) + 4;
        sx = 7; sy = 5; sz = 8;
        r->sy = sy;
        switch (r->rotation)
        { // orientation: 0:north, 1:east, 2:south, 3:west
        case 0: r->rotation = 0; r->mirror = 0; r->sx = sx; r->sz = sz; break;
        case 1: r->rotation = 1; r->mirror = 0; r->sx = sz; r->sz = sx; break;
        case 2: r->rotation = 0; r->mirror = 1; r->sx = sx; r->sz = sz; break;
        case 3: r->rotation = 1; r->mirror = 1; r->sx = sz; r->sz = sx; break;
        }
        return 1;

    case Desert_Pyramid:
        sx = 21; sy = 15; sz = 21;
        goto L_rotate_temple;
    case Jungle_Temple:
        sx = 12; sy = 10; sz = 15;
        goto L_rotate_temple;
    case Swamp_Hut:
        sx = 7; sy = 7; sz = 9;
    L_rotate_temple:
        r->sy = sy;
        r->rotation = 0;
        r->mirror = 0;
        r->sx = sx;
        r->sz = sz;
        return 1;

    case Geode:
        setDecorationSeed(seed, cx, cz, sc.salt);
        if (nextInt(sc.rarity) != 0) // rarity chance
            return 0;
        r->x = 4; // chunk offset X
        r->z = 4; // chunk offset Z
        r->x -= x & 15; // make offset relative to x and z
        r->z -= z & 15;
        if (mc >= MC_1_18) {
            r->y = nextIntRange(-58, 30); // Y-level
        } else {
            r->y = nextIntRange(6, 47); // Y-level
        }
        r->size = nextInt(2) + 3; // distribution points
        skipNextN(2);
        r->cracked = nextFloat() < 0.95;
        r->x += 5; r->y += 5; r->z += 5;
        return 1;

    case Trial_Chambers:
        uint64_t rng = chunkGenerateRnd(seed, cx, cz);
        r->y = JnextInt(&rng, 1+20) + -40; // Y-level
        r->rotation = JnextInt(&rng, 4);
        r->start = JnextInt(&rng, 2); // corridor/end_[12]
        r->sx = 19; r->sy = 20; r->sz = 19;
        //r->y += -1; // groundLevelData
        switch (r->rotation)
        { // 0:0, 1:cw90, 2:cw180, 3:cw270=ccw90
        case 0: break;
        case 1: r->x = 1-r->sz; r->z = 0;       break;
        case 2: r->x = 1-r->sx; r->z = 1-r->sz; break;
        case 3: r->x = 0;       r->z = 1-r->sx; break;
        }
        return 1;

    case Ravine:
        setPopulationSeed(seed, cx, cz);
        if (nextInt(mc >= MC_1_21_60 ? 100 : 150) != 0) // rarity chance
            return 0;
        nextInt(16);// x
        if (mc >= MC_1_21_60)
        {
            r->y = nextIntRange(10, 68);
            skipNextN(1);
        }
        else
        {
            int i = nextIntRange(8, 48);
            r->y = nextInt(i) + 20;
        }
        skipNextN(1);
        nextInt(16);// z
        r->yaw   = nextFloat() * 2.0f * PI;
        r->pitch = (nextFloat() - 0.5f) / 4.0f;
        r->thick = (nextFloat() + nextFloat()) * 3.0f;
        r->giant = nextFloat() < 0.05f;
        r->underwater = isOceanic(biomeID);
        return 1;

    case Lava_Lake:
        setPopulationSeed(seed, cx, cz);
        if (mc < MC_1_18 && (biomeID != desert || biomeID != desert_hills))
        {
            //water lakes
            if (nextInt(4) == 0)
                return 0;
        }
        if (nextInt(sc.rarity) != 0) // rarity chance
            return 0;
        skipNextN(1);
        int i = nextIntRange(8, 128);
        r->y = nextInt(i);
        r->biome = biomeID;
        return 1;

    case Abandoned_Camp:
    {
        static const uint8_t repTent[10][3] = {
            {8,8,8}, {8,8,8}, {8,8,8}, {6,8,8}, {8,8,8},
            {8,8,8}, {8,8,8}, {8,8,8}, {6,8,8}, {8,8,8},
        };

        uint64_t rng = chunkGenerateRnd(seed, cx, cz);
        int rotIdx  = JnextInt(&rng, 4);
        int tentIdx = JnextInt(&rng, 10);

        sx = repTent[tentIdx][0];
        sy = repTent[tentIdx][1];
        sz = repTent[tentIdx][2];
        int ex, ez;
        switch (rotIdx)
        {
        case 0: ex =  (sx-1); ez =  (sz-1); r->sx = sx; r->sz = sz; break;
        case 1: ex = -(sz-1); ez =  (sx-1); r->sx = sz; r->sz = sx; break;
        case 2: ex = -(sx-1); ez = -(sz-1); r->sx = sx; r->sz = sz; break;
        default:ex =  (sz-1); ez = -(sx-1); r->sx = sz; r->sz = sx; break;
        }
        r->x = ex / 2;
        r->z = ez / 2;
        r->sy = sy;
        r->rotation = (uint8_t) rotIdx;
        r->start = (uint8_t) tentIdx; // tent piece index (0-9): see getCampTentName()
        r->biome = biomeID;

        int arr[48];
        for (int i = 0; i < 48; i++)
            arr[i] = i;
        // fisher-yates shuffle
        for (int n = 48; n > 1; n--)
        {
            int k = JnextInt(&rng, n);
            int tmp = arr[k];
            arr[k] = arr[n-1];
            arr[n-1] = tmp;
        }
        int campIdx = arr[0]; // 0-47: see getCampsiteName()
        r->size = (uint8_t) campIdx;
        // secret chest
        r->secret = (campIdx == 46) ||                            // campsite_default_special_8
                    (campIdx == 1 && biomeID == sparse_jungle) || // campsite_sparse_jungle_2
                    (campIdx == 0 && biomeID == birch_forest ) || // campsite_birch_forest_1
                    (tentIdx == 4 && biomeID == cherry_grove );   // tent_cherry_grove_4
        return 1;
    }

    default:
        return 0;
    }
}


static
void moveBelowSeaLevel(Piece *list, int count, int seaLevel, int minWorldHeight, int offset)
{
    int boxMinY = 100;
    int boxMaxY = -100;
    int i;

    for (i = 0; i < count; i++)
    {
        if (list[i].bb0.y < boxMinY) boxMinY = list[i].bb0.y;
        if (list[i].bb1.y > boxMaxY) boxMaxY = list[i].bb1.y;
    }
    int h = boxMaxY - boxMinY + 1;
    int y = h + minWorldHeight + 1;
    if (y < seaLevel - offset) {
        y += nextInt((seaLevel - offset) - y);
    }
    int dy = y - boxMaxY;
    offsetPiecesVertically(list, count, dy);
}

static
void moveInsideHeights(Piece *list, int count, int minY, int maxY)
{
    int boxMinY = 100;
    int boxMaxY = -100;
    int i;

    for (i = 0; i < count; i++)
    {
        if (list[i].bb0.y < boxMinY) boxMinY = list[i].bb0.y;
        if (list[i].bb1.y > boxMaxY) boxMaxY = list[i].bb1.y;
    }

    int h = boxMaxY - boxMinY + 1;
    int s = (maxY - minY + 1) - h;
    int offset = (s > 1) ? nextInt(s) : 0;
    int dy = minY - boxMinY + offset;
    offsetPiecesVertically(list, count, dy);
}

static
void offsetPiecesVertically(Piece *list, int count, int dy)
{
    int i;
    for (i = 0; i < count; i++)
    {
        list[i].bb0.y += dy;
        list[i].bb1.y += dy;
    }
}


static
Piece *addEndCityPiece(PieceEnv *env, Piece *prev, int rot, int px, int py, int pz, int typ)
{
    static const struct { int sx, sy, sz; const char *name; } info[] = {
        {  9,  3,  9, "base_floor"},
        { 11,  1, 11, "base_roof"},
        {  4,  5,  1, "bridge_end"},
        {  4,  6,  7, "bridge_gentle_stairs"},
        {  4,  5,  3, "bridge_piece"},
        {  4,  6,  3, "bridge_steep_stairs"},
        { 12,  3, 12, "fat_tower_base"},
        { 12,  7, 12, "fat_tower_middle"},
        { 16,  5, 16, "fat_tower_top"},
        { 11,  7, 11, "second_floor_1"},
        { 11,  7, 11, "second_floor_2"},
        { 13,  1, 13, "second_roof"},
        { 12, 23, 28, "ship"},
        { 13,  7, 13, "third_floor_1"},
        { 13,  7, 13, "third_floor_2"},
        { 15,  1, 15, "third_roof"},
        {  6,  6,  6, "tower_base"},
        {  6,  3,  6, "tower_floor"}, // unused
        {  6,  3,  6, "tower_piece"},
        {  8,  4,  8, "tower_top"},
    };

    Piece *p = env->list + *env->n;
    (*env->n)++;
    p->name = info[typ].name;
    p->rot = rot;
    p->depth = 0;
    p->type = typ;
    p->next = NULL;

    int sizeX = info[typ].sx;
    int sizeY = info[typ].sy;
    int sizeZ = info[typ].sz;

    if (prev == NULL) {
        p->pos.x = px;
        p->pos.y = py;
        p->pos.z = pz;

        p->bb0 = p->bb1 = p->pos;
        p->bb1.y += sizeY;
        switch (rot)
        {
        case 0: p->bb0.x += sizeX; p->bb0.z += sizeZ; break;
        case 1: p->bb1.x -= sizeZ; p->bb0.z += sizeX; break;
        case 2: p->bb1.x -= sizeX; p->bb1.z -= sizeZ; break;
        case 3: p->bb0.x += sizeZ; p->bb1.z -= sizeX; break;
        default: UNREACHABLE();
        }
    }
    else
    {
        int dx = 0, dy = py, dz = 0;
        switch (prev->rot)
        {
        case 0: dx += px; dz += pz; break;
        case 1: dx -= pz; dz += px; break;
        case 2: dx -= px; dz -= pz; break;
        case 3: dx += pz; dz -= px; break;
        default: UNREACHABLE();
        }
        p->pos.x = prev->pos.x + dx;
        p->pos.y = prev->pos.y + dy;
        p->pos.z = prev->pos.z + dz;

        p->bb0 = p->bb1 = p->pos;
        p->bb1.y += sizeY;
        switch (rot)
        {
        case 0: p->bb1.x += sizeX; p->bb1.z += sizeZ; break;
        case 1: p->bb0.x -= sizeZ; p->bb1.z += sizeX; break;
        case 2: p->bb0.x -= sizeX; p->bb0.z -= sizeZ; break;
        case 3: p->bb1.x += sizeZ; p->bb0.z -= sizeX; break;
        default: UNREACHABLE();
        }
    }
    return p;
}

static
int genPiecesRecusively(piecefunc_t gen, PieceEnv *env, Piece *current, int depth)
{
    if (depth > 8)
        return 0;
    int i, j, n_local = 0;
    PieceEnv env_local = *env;
    env_local.list = env->list + *env->n;
    env_local.n = &n_local;
    if (!gen(&env_local, current, depth))
        return 0;
    int gendepth = nextInt(32);
    for (i = 0; i < n_local; i++)
    {
        Piece *p = env_local.list + i;
        p->depth = gendepth;
        for (j = 0; j < *env->n; j++)
        {   // check for piece with bounding box collision
            Piece *q = env->list + j;
            if (q->bb1.x >= p->bb0.x && q->bb0.x <= p->bb1.x &&
                q->bb1.z >= p->bb0.z && q->bb0.z <= p->bb1.z &&
                q->bb1.y >= p->bb0.y && q->bb0.y <= p->bb1.y)
            {
                if (current->depth != q->depth)
                    return 0;
                break;
            }
        }
    }
    (*env->n) += n_local;
    return 1;
}

static
int genTower(PieceEnv *env, Piece *current, int depth)
{
    int rot = current->rot;
    int x = 3 + nextInt(2);
    int z = 3 + nextInt(2);
    Piece *base = current;
    base = addEndCityPiece(env, base, rot, x, -3, z, TOWER_BASE);
    base = addEndCityPiece(env, base, rot, 0, 7, 0, TOWER_PIECE);
    Piece *floor = (nextInt(3) == 0 ? base : NULL);
    int floorcnt = 1 + nextInt(3);
    int i;
    for (i = 0; i < floorcnt; i++)
    {
        base = addEndCityPiece(env, base, rot, 0, 4, 0, TOWER_PIECE);
        if (i < floorcnt - 1 && nextBoolean())
            floor = base;
    }
    if (floor)
    {
        static const int binfo[][4] = {
            {0, 1, -1, 0}, // 0
            {1, 6, -1, 1}, // 90
            {3, 0, -1, 5}, // 270
            {2, 5, -1, 6}, // 180
        };
        for (i = 0; i < 4; i++)
        {
            if (!nextBoolean())
                continue;
            int brot = (rot + binfo[i][0]) & 3;
            Piece *bridge = addEndCityPiece(env, base, brot,
                binfo[i][1], binfo[i][2], binfo[i][3], BRIDGE_END);
            genPiecesRecusively(genBridge, env, bridge, depth+1);
        }
    }
    else if (depth != 7)
    {
        return genPiecesRecusively(genFatTower, env, base, depth+1);
    }

    addEndCityPiece(env, base, rot, -1, 4, -1, TOWER_TOP);
    return 1;
}

static
int genBridge(PieceEnv *env, Piece *current, int depth)
{
    int rot = current->rot;
    int i, y, floorcnt = 1 + nextInt(4);
    Piece *base = current;
    base = addEndCityPiece(env, base, rot, 0, 0, -4, BRIDGE_PIECE);
    base->depth = -1;
    for (i = y = 0; i < floorcnt; i++)
    {
        if (nextBoolean())
        {
            base = addEndCityPiece(env, base, rot, 0, y, -4, BRIDGE_PIECE);
            y = 0;
            continue;
        }
        if (nextBoolean())
            base = addEndCityPiece(env, base, rot, 0, y, -4, BRIDGE_STEEP_STAIRS);
        else
            base = addEndCityPiece(env, base, rot, 0, y, -8, BRIDGE_GENTLE_STAIRS);
        y = 4;
    }
    if (!*env->ship && nextInt(10 - depth) == 0)
    {
        int x = -8 + nextInt(8);
        int z = -70 + nextInt(10);
        base = addEndCityPiece(env, base, rot, x, y, z, END_SHIP);
        *env->ship = 1;
    }
    else
    {
        if (!genPiecesRecusively(genHouseTower, env, base, depth+1))
            return 0;
    }
    base = addEndCityPiece(env, base, rot, 4, y, 0, BRIDGE_END);
    base->depth = -1;
    return 1;
}

static
int genHouseTower(PieceEnv *env, Piece *current, int depth)
{
    if (depth > 8) return 0;
    int rot = current->rot;
    Piece *base = current;
    base = addEndCityPiece(env, base, rot, -3, env->y, -11, BASE_FLOOR);
    int size = nextInt(3);
    if (size == 0)
    {
        addEndCityPiece(env, base, rot, -1, 4, -1, BASE_ROOF);
        return 1;
    }
    base = addEndCityPiece(env, base, rot, -1, 0, -1, SECOND_FLOOR_2);
    if (size == 1)
    {
        base = addEndCityPiece(env, base, rot, -1, 8, -1, SECOND_ROOF);
    }
    else
    {
        base = addEndCityPiece(env, base, rot, -1, 4, -1, THIRD_FLOOR_2);
        base = addEndCityPiece(env, base, rot, -1, 8, -1, THIRD_ROOF);
    }
    genPiecesRecusively(genTower, env, base, depth+1);
    return 1;
}

static
int genFatTower(PieceEnv *env, Piece *current, int depth)
{
    int rot = current->rot;
    int i, j;
    Piece *base = current;
    base = addEndCityPiece(env, base, rot, -3, 4, -3, FAT_TOWER_BASE);
    base = addEndCityPiece(env, base, rot, 0, 4, 0, FAT_TOWER_MIDDLE);
    static const int binfo[][4] = {
        {0,  4, -1,  0}, // 0
        {1, 12, -1,  4}, // 90
        {3,  0, -1,  8}, // 270
        {2,  8, -1, 12}, // 180
    };
    for (j = 0; j < 2 && nextInt(3) != 0; j++)
    {
        base = addEndCityPiece(env, base, rot, 0, 8, 0, FAT_TOWER_MIDDLE);
        for (i = 0; i < 4; i++)
        {
            if (!nextBoolean())
                continue;
            int brot = (rot + binfo[i][0]) & 3;
            Piece *bridge = addEndCityPiece(env, base, brot,
                binfo[i][1], binfo[i][2], binfo[i][3], BRIDGE_END);
            genPiecesRecusively(genBridge, env, bridge, depth+1);
        }
    }
    addEndCityPiece(env, base, rot, -2, 8, -2, FAT_TOWER_TOP);
    return 1;
}

int getEndCityPieces(Piece *list, uint64_t seed, int chunkX, int chunkZ)
{
    StructureConfig sc;
    getStructureConfig(End_City, MC_1_13, &sc);
    setSeed(chunkX + chunkZ * sc.salt);
    int rot = nextInt(4);
    Pos region = chunkToRegion(chunkX, chunkZ, sc.regionSize);
    setRegionSeed(seed, region.x, region.z, sc.salt);
    skipNextN(4);
    int ship = 0, n = 0;
    PieceEnv env;
    memset(&env, 0, sizeof(env));
    env.list = list;
    env.n = &n;
    env.ship = &ship;
    env.y = 1;
    Piece *base = NULL;
    int x = chunkX * 16 + 8, z = chunkZ * 16 + 8;
    base = addEndCityPiece(&env, base, rot, x, 0, z, BASE_FLOOR);
    base = addEndCityPiece(&env, base, rot, -1, 0, -1, SECOND_FLOOR_1);
    base = addEndCityPiece(&env, base, rot, -1, 4, -1, THIRD_FLOOR_1);
    base = addEndCityPiece(&env, base, rot, -1, 8, -1, THIRD_ROOF);
    genPiecesRecusively(genTower, &env, base, 1);
    return n;
}


//==============================================================================
// Stronghold Generator
//==============================================================================

typedef struct
{
    Piece *list;
    int count;
    int nmax;
    uint8_t *data;
    Piece *pending;
    Piece *pending_tail;
    int pending_count;
    int ntyp[SH_WEIGHT_TYPES];
    int typlast;
    int typcur;
    uint64_t seed;
    uint8_t *eyes;
    int gotportal;
} StrongholdEnv;

static
const struct
{
    const char *name;
    int weight;
    int max;
    int rngCount;
}
stronghold_info[] = {
    {"Start",            0, 0,  194},
    {"Corridor",        40, 0,  134},
    {"PrisonHall",       5, 5,  318},
    {"LeftTurn",        20, 0,   98},
    {"RightTurn",       20, 0,   98},
    {"RoomCrossing",    10, 6,  442},
    {"Stairs",           5, 5,  278},
    {"SpiralStaircase",  5, 5,  194},
    {"FiveWayCrossing",  5, 4,  595},
    {"ChestCorridor",    5, 4,  130},
    {"Library",         10, 2, 1156},
    {"PortalRoom",      10, 1,  760},
    {"SmallCorridor",    0, 0,    0},
};

static
const int stronghold_weights[SH_WEIGHT_TYPES] = {
    SH_CORRIDOR,
    SH_PRISON_HALL,
    SH_LEFT_TURN,
    SH_RIGHT_TURN,
    SH_ROOM_CROSSING,
    SH_STAIRS,
    SH_SPIRAL_STAIRCASE,
    SH_FIVE_WAY_CROSSING,
    SH_CHEST_CORRIDOR,
    SH_LIBRARY,
    SH_PORTAL_ROOM,
};

static void setPortalFramePos(StrongholdPortalFrame *frames, const Piece *portal);
static void setPortalFrameEyes(StrongholdEnv *env, const Piece *portal);

static
void strongholdRotatedBox(Pos3 *bb0, Pos3 *bb1,
        int x, int y, int z, int offx, int offy, int offz,
        int sizex, int sizey, int sizez, int rot)
{
    int minX, minY, minZ, maxX, maxY, maxZ;
    minY = y + offy;
    maxY = y + sizey - 1 + offy;
    switch (rot)
    {
    case 3: // west
        minX = x - sizez + 1 + offz;
        minZ = z + offx;
        maxX = x + offz;
        maxZ = z + sizex - 1 + offx;
        break;
    case 1: // east
        minX = x + offz;
        minZ = z + offx;
        maxX = x + sizez - 1 + offz;
        maxZ = z + sizex - 1 + offx;
        break;
    case 2: // south
        minX = x + offx;
        minZ = z + offz;
        maxX = x + sizex - 1 + offx;
        maxZ = z + sizez - 1 + offz;
        break;
    case 0: // north
        minX = x + offx;
        minZ = z - sizez + 1 + offz;
        maxX = x + sizex - 1 + offx;
        maxZ = z + offz;
        break;
    default:
        UNREACHABLE();
    }
    *bb0 = (Pos3){minX, minY, minZ};
    *bb1 = (Pos3){maxX, maxY, maxZ};
}

static
int boxesIntersect(Pos3 a0, Pos3 a1, Pos3 b0, Pos3 b1)
{
    return a1.x >= b0.x && a0.x <= b1.x &&
           a1.y >= b0.y && a0.y <= b1.y &&
           a1.z >= b0.z && a0.z <= b1.z;
}

static
Piece *getNextIntersectingPiece(
        const Piece *list, int count, Pos3 bb0, Pos3 bb1)
{
    int i;
    for (i = 0; i < count; i++)
    {
        const Piece *p = list + i;
        if (boxesIntersect(p->bb0, p->bb1, bb0, bb1))
            return (Piece *) p;
    }
    return NULL;
}

static
int applyXTransform(const Piece *p, int x, int z)
{
    switch (p->rot)
    {
    case 0: return p->bb0.x + x;
    case 2: return p->bb0.x + x;
    case 3: return p->bb1.x - z;
    case 1: return p->bb0.x + z;
    default:
        UNREACHABLE();
    }
}

static
int applyYTransform(const Piece *p, int y)
{
    return p->bb0.y + y;
}

static
int applyZTransform(const Piece *p, int x, int z)
{
    switch (p->rot)
    {
    case 0: return p->bb1.z - z;
    case 2: return p->bb0.z + z;
    case 3: return p->bb0.z + x;
    case 1: return p->bb0.z + x;
    default:
        UNREACHABLE();
    }
}

static
Pos3 transformPos(const Piece *p, int x, int y, int z)
{
    Pos3 pos;
    pos.x = applyXTransform(p, x, z);
    pos.y = applyYTransform(p, y);
    pos.z = applyZTransform(p, x, z);
    return pos;
}

static
int strongholdIsHighEnough(Pos3 bb0)
{
    return bb0.y > 10;
}

static
int canStrongholdTypeSpawn(int typ, int spawned, int pieceId)
{
    int max = stronghold_info[typ].max;
    if (max > 0 && spawned >= max)
        return 0;
    if (typ == SH_LIBRARY)
        return pieceId > 4;
    if (typ == SH_PORTAL_ROOM)
        return pieceId > 5;
    return 1;
}

static
int canAddStrongholdPieces(const StrongholdEnv *env, int *totalWeight)
{
    int i, flag = 0, total = 0;
    for (i = 0; i < SH_WEIGHT_TYPES; i++)
    {
        int typ = stronghold_weights[i];
        int max = stronghold_info[typ].max;
        if (max > 0 && env->ntyp[i] >= max)
            continue;
        if (stronghold_info[typ].max > 0 && env->ntyp[i] < stronghold_info[typ].max)
            flag = 1;
        total += stronghold_info[typ].weight;
    }
    *totalWeight = total;
    return flag;
}

static
Piece *addStrongholdPiece(StrongholdEnv *env, int typ,
        int x, int y, int z, int facing, int pieceId)
{
    Piece piece;
    Pos3 bb0, bb1;
    Piece *intersect;
    uint8_t data = 0;

    if (env->count >= env->nmax)
        return NULL;

    memset(&piece, 0, sizeof(piece));
    piece.name = stronghold_info[typ].name;
    piece.pos = (Pos3){x, y, z};
    piece.rot = facing;
    piece.depth = pieceId;
    piece.type = typ;
    piece.next = NULL;

    switch (typ)
    {
    case SH_START:
        piece.bb0 = (Pos3){x, 64, z};
        piece.bb1 = (Pos3){x + 5 - 1, 64 + 11 - 1, z + 5 - 1};
        break;
    case SH_CORRIDOR:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -1, 0, 5, 5, 7, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        if (nextInt(2) == 0) data |= 0x01;
        if (nextInt(2) == 0) data |= 0x02;
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_PRISON_HALL:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -1, 0, 9, 5, 11, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_LEFT_TURN:
    case SH_RIGHT_TURN:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -1, 0, 5, 5, 5, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_ROOM_CROSSING:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -4, -1, 0, 11, 7, 11, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        data = (uint8_t) nextInt(5); // room type
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_STAIRS:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -7, 0, 5, 11, 8, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_SPIRAL_STAIRCASE:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -7, 0, 5, 11, 5, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_FIVE_WAY_CROSSING:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -4, -3, 0, 10, 9, 11, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        if (nextBoolean()) data |= 0x01;
        if (nextBoolean()) data |= 0x02;
        if (nextBoolean()) data |= 0x04;
        if (nextInt(3) > 0) data |= 0x08;
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_CHEST_CORRIDOR:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -1, 0, 5, 5, 7, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        nextInt(5); // entrance type
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_LIBRARY:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -4, -1, 0, 14, 11, 15, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
        {
            strongholdRotatedBox(&bb0, &bb1, x, y, z, -4, -1, 0, 14, 6, 15, facing);
            if (!strongholdIsHighEnough(bb0) ||
                getNextIntersectingPiece(env->list, env->count, bb0, bb1))
                return NULL;
        }
        nextInt(5); // entrance type
        if (bb1.y - bb0.y + 1 > 6)
            data = 1;
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_PORTAL_ROOM:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -4, -1, 0, 11, 8, 16, facing);
        if (!strongholdIsHighEnough(bb0) ||
            getNextIntersectingPiece(env->list, env->count, bb0, bb1))
            return NULL;
        piece.bb0 = bb0;
        piece.bb1 = bb1;
        break;
    case SH_SMALL_CORRIDOR:
        strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -1, 0, 5, 5, 4, facing);
        intersect = getNextIntersectingPiece(env->list, env->count, bb0, bb1);
        if (intersect != NULL && intersect->bb0.y == bb0.y)
        {
            int len;
            for (len = 3; len >= 1; len--)
            {
                Pos3 test0, test1;
                strongholdRotatedBox(&test0, &test1, x, y, z, -1, -1, 0, 5, 5, len - 1, facing);
                if (!boxesIntersect(intersect->bb0, intersect->bb1, test0, test1))
                {
                    strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, -1, 0, 5, 5, len, facing);
                    piece.bb0 = bb0;
                    piece.bb1 = bb1;
                    goto L_accept_piece;
                }
            }
        }
        return NULL;
    default:
        UNREACHABLE();
    }

L_accept_piece:
    env->list[env->count] = piece;
    env->data[env->count] = data;
    return env->list + env->count++;
}

static void strongholdAppendPending(StrongholdEnv *env, Piece *p)
{
    p->next = NULL;
    if (env->pending_tail)
        env->pending_tail->next = p;
    else
        env->pending = p;
    env->pending_tail = p;
    env->pending_count++;
}

static Piece *getNextStrongholdPiece(StrongholdEnv *env,
        int x, int y, int z, int facing, int parentDepth)
{
    int totalWeight, attempt;

    if (!canAddStrongholdPieces(env, &totalWeight))
        return NULL;

    if (env->typcur >= 0)
    {
        Piece *piece = addStrongholdPiece(env, env->typcur, x, y, z, facing, parentDepth + 1);
        env->typcur = -1;
        if (piece != NULL)
            return piece;
    }

    for (attempt = 0; attempt < 5; attempt++)
    {
        int i, n = nextInt(totalWeight);
        for (i = 0; i < SH_WEIGHT_TYPES; i++)
        {
            int typ = stronghold_weights[i];
            int max = stronghold_info[typ].max;
            if (max > 0 && env->ntyp[i] >= max)
                continue;
            n -= stronghold_info[typ].weight;
            if (n >= 0)
                continue;
            if (!canStrongholdTypeSpawn(typ, env->ntyp[i], parentDepth + 1) ||
                env->typlast == typ)
                break;

            Piece *piece = addStrongholdPiece(env, typ, x, y, z, facing, parentDepth + 1);
            if (piece != NULL)
            {
                env->ntyp[i]++;
                env->typlast = typ;
                return piece;
            }
        }
    }

    return addStrongholdPiece(env, SH_SMALL_CORRIDOR, x, y, z, facing, parentDepth + 1);
}

static
Piece *extendStronghold(StrongholdEnv *env,
        int x, int y, int z, int facing, int parentDepth)
{
    MTRngState saved;
    Piece *piece;

    if (parentDepth > 50)
        return NULL;
    if (IABS(x - env->list[0].bb0.x) > 112 || IABS(z - env->list[0].bb0.z) > 112)
        return NULL;

    mt_get_state(&saved);
    piece = getNextStrongholdPiece(env, x, y, z, facing, parentDepth);
    mt_set_state(&saved);

    if (piece != NULL)
        strongholdAppendPending(env, piece);
    return piece;
}

static
Piece *extendStrongholdForward(
        StrongholdEnv *env, const Piece *p, int a, int b)
{
    switch (p->rot)
    {
    case 0: return extendStronghold(env, p->bb0.x + a, p->bb0.y + b, p->bb0.z - 1, 0, p->depth);
    case 2: return extendStronghold(env, p->bb0.x + a, p->bb0.y + b, p->bb1.z + 1, 2, p->depth);
    case 3: return extendStronghold(env, p->bb0.x - 1, p->bb0.y + b, p->bb0.z + a, 3, p->depth);
    case 1: return extendStronghold(env, p->bb1.x + 1, p->bb0.y + b, p->bb0.z + a, 1, p->depth);
    default:
        UNREACHABLE();
    }
}

static
Piece *extendStrongholdLeft(
        StrongholdEnv *env, const Piece *p, int a, int b)
{
    switch (p->rot)
    {
    case 0: return extendStronghold(env, p->bb0.x - 1, p->bb0.y + a, p->bb0.z + b, 3, p->depth);
    case 2: return extendStronghold(env, p->bb0.x - 1, p->bb0.y + a, p->bb0.z + b, 3, p->depth);
    case 3: return extendStronghold(env, p->bb0.x + b, p->bb0.y + a, p->bb0.z - 1, 0, p->depth);
    case 1: return extendStronghold(env, p->bb0.x + b, p->bb0.y + a, p->bb0.z - 1, 0, p->depth);
    default:
        UNREACHABLE();
    }
}

static
Piece *extendStrongholdRight(
        StrongholdEnv *env, const Piece *p, int a, int b)
{
    switch (p->rot)
    {
    case 0: return extendStronghold(env, p->bb1.x + 1, p->bb0.y + a, p->bb0.z + b, 1, p->depth);
    case 2: return extendStronghold(env, p->bb1.x + 1, p->bb0.y + a, p->bb0.z + b, 1, p->depth);
    case 3: return extendStronghold(env, p->bb0.x + b, p->bb0.y + a, p->bb1.z + 1, 2, p->depth);
    case 1: return extendStronghold(env, p->bb0.x + b, p->bb0.y + a, p->bb1.z + 1, 2, p->depth);
    default:
        UNREACHABLE();
    }
}

static
void extendStrongholdPiece(StrongholdEnv *env, Piece *p)
{
    uint8_t data = env->data[p - env->list];

    switch (p->type)
    {
    case SH_START:
        env->typcur = SH_FIVE_WAY_CROSSING;
        extendStrongholdForward(env, p, 1, 1);
        break;
    case SH_CORRIDOR:
        extendStrongholdForward(env, p, 1, 1);
        if (data & 0x01)
            extendStrongholdLeft(env, p, 1, 2);
        if (data & 0x02)
            extendStrongholdRight(env, p, 1, 2);
        break;
    case SH_PRISON_HALL:
    case SH_STAIRS:
    case SH_SPIRAL_STAIRCASE:
    case SH_CHEST_CORRIDOR:
        extendStrongholdForward(env, p, 1, 1);
        break;
    case SH_LEFT_TURN:
        if (p->rot != 0 && p->rot != 1)
            extendStrongholdRight(env, p, 1, 1);
        else
            extendStrongholdLeft(env, p, 1, 1);
        break;
    case SH_RIGHT_TURN:
        if (p->rot != 0 && p->rot != 1)
            extendStrongholdLeft(env, p, 1, 1);
        else
            extendStrongholdRight(env, p, 1, 1);
        break;
    case SH_ROOM_CROSSING:
        extendStrongholdForward(env, p, 4, 1);
        extendStrongholdLeft(env, p, 1, 4);
        extendStrongholdRight(env, p, 1, 4);
        break;
    case SH_FIVE_WAY_CROSSING:
    {
        int a = 3, b = 5;
        if (p->rot == 3 || p->rot == 0)
        {
            a = 8 - a;
            b = 8 - b;
        }
        extendStrongholdForward(env, p, 5, 1);
        if (data & 0x01) extendStrongholdLeft(env, p, a, 1);
        if (data & 0x02) extendStrongholdLeft(env, p, b, 7);
        if (data & 0x04) extendStrongholdRight(env, p, a, 1);
        if (data & 0x08) extendStrongholdRight(env, p, b, 7);
        break;
    }
    case SH_LIBRARY:
        break;
    case SH_PORTAL_ROOM:
        setPortalFrameEyes(env, p);
        break;
    case SH_SMALL_CORRIDOR:
        break;
    default:
        UNREACHABLE();
    }
}

static
int strongholdPieceBaseRngCount(const Piece *p, uint8_t data)
{
    int n = stronghold_info[p->type].rngCount;
    if (p->type == SH_LIBRARY && data)
        n += 270;
    return n;
}

static
int strongholdGetChestPositions(const Piece *p, uint8_t data, Pos3 out[2])
{
    int n = 0;
    if (p->type == SH_CHEST_CORRIDOR)
    {
        out[n++] = transformPos(p, 3, 1, 3);
    }
    else if (p->type == SH_ROOM_CROSSING && data == 2)
    {
        // store room
        out[n++] = transformPos(p, 3, 1, 8);
    }
    else if (p->type == SH_LIBRARY)
    {
        // first floor
        out[n++] = transformPos(p, 3, 1, 5);
        if (data)
            // second floor
            out[n++] = transformPos(p, 12, 5, 1);
    }
    return n;
}

static
int strongholdPieceRngCountInChunk(const Piece *p, uint8_t data,
        int chunkX, int chunkZ)
{
    int minChunkX = floordiv(p->bb0.x, 16);
    int maxChunkX = floordiv(p->bb1.x, 16);
    int minChunkZ = floordiv(p->bb0.z, 16);
    int maxChunkZ = floordiv(p->bb1.z, 16);
    Pos3 chests[2];
    int i, count;

    if (chunkX < minChunkX || chunkX > maxChunkX ||
        chunkZ < minChunkZ || chunkZ > maxChunkZ)
        return 0;

    count = stronghold_info[p->type].rngCount;
    if (p->type == SH_LIBRARY && data)
        count += 270; // second floor

    i = strongholdGetChestPositions(p, data, chests);
    while (i-- > 0)
    {
        if (floordiv(chests[i].x, 16) == chunkX &&
            floordiv(chests[i].z, 16) == chunkZ)
        {
            count++;
        }
    }
    return count;
}

static
void setPortalFramePos(StrongholdPortalFrame *frames, const Piece *portal)
{
    switch (portal->rot)
    {
    case 0:
    case 1:
        frames[0].pos = transformPos(portal, 4, 3, 8);
        frames[1].pos = transformPos(portal, 5, 3, 8);
        frames[2].pos = transformPos(portal, 6, 3, 8);
        frames[3].pos = transformPos(portal, 4, 3, 12);
        frames[4].pos = transformPos(portal, 5, 3, 12);
        frames[5].pos = transformPos(portal, 6, 3, 12);
        frames[6].pos = transformPos(portal, 3, 3, 9);
        frames[7].pos = transformPos(portal, 3, 3, 10);
        frames[8].pos = transformPos(portal, 3, 3, 11);
        frames[9].pos = transformPos(portal, 7, 3, 9);
        frames[10].pos = transformPos(portal, 7, 3, 10);
        frames[11].pos = transformPos(portal, 7, 3, 11);
        break;
    case 2:
    case 3:
        frames[0].pos = transformPos(portal, 6, 3, 8);
        frames[1].pos = transformPos(portal, 5, 3, 8);
        frames[2].pos = transformPos(portal, 4, 3, 8);
        frames[3].pos = transformPos(portal, 6, 3, 12);
        frames[4].pos = transformPos(portal, 5, 3, 12);
        frames[5].pos = transformPos(portal, 4, 3, 12);
        if (portal->rot == 2)
        {
            frames[6].pos = transformPos(portal, 3, 3, 9);
            frames[7].pos = transformPos(portal, 3, 3, 10);
            frames[8].pos = transformPos(portal, 3, 3, 11);
            frames[9].pos = transformPos(portal, 7, 3, 9);
            frames[10].pos = transformPos(portal, 7, 3, 10);
            frames[11].pos = transformPos(portal, 7, 3, 11);
        }
        else
        {
            frames[6].pos = transformPos(portal, 7, 3, 9);
            frames[7].pos = transformPos(portal, 7, 3, 10);
            frames[8].pos = transformPos(portal, 7, 3, 11);
            frames[9].pos = transformPos(portal, 3, 3, 9);
            frames[10].pos = transformPos(portal, 3, 3, 10);
            frames[11].pos = transformPos(portal, 3, 3, 11);
        }
        break;
    default:
        UNREACHABLE();
    }
}

/**
 * This differs from the original process as it focuses exclusively on RNG calls for eye counts.
 * It avoids unnecessary generation in chunks where portal rooms do not overlap.
 */
static
void setPortalFrameEyes(StrongholdEnv *env, const Piece *portal)
{
    StrongholdPortalFrame frames[12];
    MTRngState saved;
    int i, j;

    if (env->eyes == NULL || env->gotportal)
        return;

    setPortalFramePos(frames, portal);
    mt_get_state(&saved);
    for (i = 0; i < 12; i++)
    {
        int chunkX = floordiv(frames[i].pos.x, 16);
        int chunkZ = floordiv(frames[i].pos.z, 16);
        int rngCount = 0;
        for (j = 0; j < env->count; j++)
            rngCount += strongholdPieceRngCountInChunk(env->list + j, env->data[j], chunkX, chunkZ);
        setPopulationSeed(env->seed, chunkX, chunkZ);
        skipNextN(rngCount + i);
        env->eyes[i] = nextFloat() > 0.9f; // 10% chance
    }
    mt_set_state(&saved);
    env->gotportal = 1;
}

static
int getStrongholdPiecesInternal(Piece *list, int n, int mc,
        uint64_t seed, int chunkX, int chunkZ, uint8_t *portalEyes)
{
    StrongholdEnv env;
    Piece *p, *prev;
    int i;

    if (n <= 0)
        return 0;

    memset(&env, 0, sizeof(env));
    env.data = (uint8_t *) calloc((size_t) n, sizeof(*env.data));
    if (env.data == NULL)
        return 0;
    env.list = list;
    env.nmax = n;
    env.typlast = -1;
    env.typcur = -1;
    env.seed = seed;
    env.eyes = portalEyes;

    setPopulationSeed(seed, chunkX, chunkZ);
    next(); // burn one call

    memset(list, 0, (size_t)n * sizeof(*list));
    p = list;
    p->name = stronghold_info[SH_START].name;
    p->pos = (Pos3){(chunkX << 4) + 2, 64, (chunkZ << 4) + 2};
    p->bb0 = (Pos3){p->pos.x, 64, p->pos.z};
    p->bb1 = (Pos3){p->pos.x + 5 - 1, 64 + 11 - 1, p->pos.z + 5 - 1};
    p->rot = (nextInt(4) + 2) % 4; // bedrock use opposite
    p->depth = 0;
    p->type = SH_START;
    p->next = NULL;
    env.count = 1;

    extendStrongholdPiece(&env, p);
    while (env.pending != NULL)
    {
        int pick = nextInt(env.pending_count);
        prev = NULL;
        p = env.pending;
        while (pick-- > 0)
        {
            prev = p;
            p = p->next;
        }
        if (prev)
            prev->next = p->next;
        else
            env.pending = p->next;
        if (env.pending_tail == p)
            env.pending_tail = prev;
        env.pending_count--;
        p->next = NULL;
        extendStrongholdPiece(&env, p);
    }

    moveBelowSeaLevel(list, env.count, 63, mc >= MC_1_18 ? -64 : 0, mc >= MC_1_18 ? 10 : 5);

    for (i = 0; i < env.count; i++)
        list[i].next = NULL;
    free(env.data);
    return env.count;
}

int getStrongholdPieces(Piece *list, int n, int mc, uint64_t seed, int chunkX, int chunkZ)
{
    return getStrongholdPiecesInternal(list, n, mc, seed, chunkX, chunkZ, NULL);
}

int getStrongholdPortalFrames(StrongholdPortalFrame *frames,
        const Piece *list, int count, uint64_t seed)
{
    Piece tmp[SH_PIECES_MAX];
    uint8_t portalEyes[12] = {0};
    int i;
    const Piece *portal = NULL;
    int startChunkX, startChunkZ;

    for (i = 0; i < count; i++)
    {
        if (list[i].type == SH_PORTAL_ROOM)
        {
            portal = list + i;
            break;
        }
    }
    if (portal == NULL)
        return 0;

    startChunkX = floordiv(list[0].pos.x - 2, 16);
    startChunkZ = floordiv(list[0].pos.z - 2, 16);
    getStrongholdPiecesInternal(tmp, SH_PIECES_MAX, MC_NEWEST, seed, startChunkX, startChunkZ, portalEyes);
    setPortalFramePos(frames, portal);
    for (i = 0; i < 12; i++)
    {
        frames[i].frameId = i;
        frames[i].hasEye = portalEyes[i];
    }
    return 12;
}


static const struct
{
    Pos3 offset, size;
    int skip, repeatable, weight, max;
    const char *name;
}
fortress_info[] = {
    {{ 0, 0,0}, {18, 9,18}, 0, 0, 0, 0, "NeStart"}, // FORTRESS_START
    {{-1,-3,0}, { 4, 9,18}, 0, 1,30, 0, "NeBS"},    // BRIDGE_STRAIGHT
    {{-8,-3,0}, {18, 9,18}, 0, 0,10, 4, "NeBCr"},   // BRIDGE_CROSSING
    {{-2, 0,0}, { 6, 8, 6}, 0, 0,10, 4, "NeRC"},    // BRIDGE_FORTIFIED_CROSSING
    {{-2, 0,0}, { 6,10, 6}, 0, 0,10, 3, "NeSR"},    // BRIDGE_STAIRS
    {{-2, 0,0}, { 6, 7, 8}, 0, 0, 5, 2, "NeMT"},    // BRIDGE_SPAWNER
    {{-5,-3,0}, {12,13,12}, 0, 0, 5, 1, "NeCE"},    // BRIDGE_CORRIDOR_ENTRANCE
    {{-1, 0,0}, { 4, 6, 4}, 0, 1,25, 0, "NeSC"},    // CORRIDOR_STRAIGHT
    {{-1, 0,0}, { 4, 6, 4}, 0, 0,15, 5, "NeSCSC"},  // CORRIDOR_CROSSING
    {{-1, 0,0}, { 4, 6, 4}, 1, 0, 5,10, "NeSCRT"},  // CORRIDOR_TURN_RIGHT
    {{-1, 0,0}, { 4, 6, 4}, 1, 0, 5,10, "NeSCLT"},  // CORRIDOR_TURN_LEFT
    {{-1,-7,0}, { 4,13, 9}, 0, 1,10, 3, "NeCCS"},   // CORRIDOR_STAIRS
    {{-3, 0,0}, { 8, 6, 8}, 0, 0, 7, 2, "NeCTB"},   // CORRIDOR_T_CROSSING
    {{-5,-3,0}, {12,13,12}, 0, 0, 5, 2, "NeCSR"},   // CORRIDOR_NETHER_WART
    {{-1,-3,0}, { 4, 9, 7}, 1, 0, 0, 0, "NeBEF"},   // FORTRESS_END
};

static
Piece *addFortressPiece(PieceEnv *env, int typ, int x, int y, int z, int depth, int facing, int pending)
{
    Pos3 pos = {x, y, z};
    Pos3 b0 = pos, b1 = pos;
    Pos3 d0 = fortress_info[typ].offset, d1 = fortress_info[typ].size;
    b0.y += d0.y;
    b1.y += d0.y+d1.y;
    switch (facing)
    {
    case 0: // 0, north
        b0.x += d0.x;       b0.z += d0.z-d1.z;
        b1.x += d0.x+d1.x;  b1.z += d0.z;
        break;
    case 1: // 90, east
        b0.x += d0.z;       b0.z += d0.x;
        b1.x += d0.z+d1.z;  b1.z += d0.x+d1.x;
        break;
    case 2: // 180, south
        b0.x += d0.x;       b0.z += d0.z;
        b1.x += d0.x+d1.x;  b1.z += d0.z+d1.z;
        break;
    case 3: // 270, west
        b0.x += d0.z-d1.z;  b0.z += d0.x;
        b1.x += d0.z;       b1.z += d0.x+d1.x;
        break;
    }
    Piece *p = env->list + *env->n;
    p->name = fortress_info[typ].name;
    p->pos = pos;
    p->bb0 = b0;
    p->bb1 = b1;
    p->rot = facing;
    p->depth = depth;
    p->type = typ;
    p->next = NULL;

    int i, n = *env->n;
    for (i = 0; i < n; i++)
    {
        Piece *q = env->list + i;
        if (q->bb1.x >= p->bb0.x && q->bb0.x <= p->bb1.x &&
            q->bb1.z >= p->bb0.z && q->bb0.z <= p->bb1.z &&
            q->bb1.y >= p->bb0.y && q->bb0.y <= p->bb1.y)
        {
            return NULL; // collision
        }
    }
    // accept the piece and append it to the processing front
    skipNextN(fortress_info[typ].skip);
    //int queue = 0;
    if (pending)
    {
        (*env->n)++;
        env->ntyp[typ]++;
        if (typ != FORTRESS_END)
            env->typlast = typ;
        Piece *q = env->list;
        while (q->next) {
            q = q->next; //queue++;
        }
        q->next = p;
    }
    //printf("[%3d] typ=%2d @(%4d %4d %4d) f=%d p=%d queue=%2d   rng:%ld\n",
    //    (*env->n-1), typ, b0.x, b0.y, b0.z, facing, pending, queue, *env->rng);
    //fflush(stdout);
    return p;
}


static
void extendFortress(PieceEnv *env, Piece *p, int offh, int offv, int turn, int corridor)
{
    int x, y, z, t, i;
    int depth = p->depth + 1;
    int facing = p->rot;
    int typ0 = corridor ? CORRIDOR_STRAIGHT : BRIDGE_STRAIGHT;
    int typ1 = typ0 + (corridor ? 7 : 6);
    int valid = -1;
    int weight_tot = 0;

    y = p->bb0.y + offv;

    if (turn == 0) { // forward
        switch (facing) {
        case 0: x = p->bb0.x+offh; z = p->bb0.z-1;    break;
        case 1: x = p->bb1.x+1;    z = p->bb0.z+offh; break;
        case 2: x = p->bb0.x+offh; z = p->bb1.z+1;    break;
        case 3: x = p->bb0.x-1;    z = p->bb0.z+offh; break;
        default: UNREACHABLE();
        }
    } else if (turn == -1) { // left
        if (facing & 1) { x = p->bb0.x+offh; z = p->bb0.z-1;    facing = 0; }
        else            { x = p->bb0.x-1;    z = p->bb0.z+offh; facing = 3; }
    } else if (turn == +1) { // right
        if (facing & 1) { x = p->bb0.x+offh, z = p->bb1.z+1;    facing = 2; }
        else            { x = p->bb1.x+1;    z = p->bb0.z+offh; facing = 1; }
    } else UNREACHABLE();

    if (IABS(x - env->list->bb0.x) > 112 || IABS(z - env->list->bb0.z) > 112)
        goto L_end;

    for (valid = 0, t = typ0; t < typ1; t++)
    {
        int max = fortress_info[t].max;
        if (max > 0 && env->ntyp[t] >= max)
            continue;
        if (max > 0)
            valid = 1;
        weight_tot += fortress_info[t].weight;
    }

    if (valid == 0 || weight_tot <= 0 || depth > 30)
        goto L_end;

    for (i = 0; i < 5; i++)
    {
        int n = nextInt(weight_tot);
        for (t = typ0; t < typ1; t++)
        {
            int max = fortress_info[t].max;
            if (max > 0 && env->ntyp[t] >= max)
                continue;
            n -= fortress_info[t].weight;
            if (n >= 0)
                continue;
            if (env->typlast == t && !fortress_info[t].repeatable)
                break;
            if (addFortressPiece(env, t, x, y, z, depth, facing, 1) != NULL)
                return;
        }
    }

L_end:
    addFortressPiece(env, FORTRESS_END, x, y, z, depth, facing, 1);
}

static
void extendFortressPiece(PieceEnv *env, Piece *p)
{
    if (p->type == BRIDGE_STRAIGHT) {
        extendFortress(env, p, 1, 3,  0, 0);
    } else if (p->type == BRIDGE_CROSSING || p->type == FORTRESS_START) {
        extendFortress(env, p, 8, 3,  0, 0);
        extendFortress(env, p, 8, 3, -1, 0);
        extendFortress(env, p, 8, 3,  1, 0);
    } else if (p->type == BRIDGE_FORTIFIED_CROSSING) {
        extendFortress(env, p, 2, 0,  0, 0);
        extendFortress(env, p, 2, 0, -1, 0);
        extendFortress(env, p, 2, 0,  1, 0);
    } else if (p->type == BRIDGE_STAIRS) {
        extendFortress(env, p, 2, 6,  1, 0);
    } else if (p->type == BRIDGE_CORRIDOR_ENTRANCE) {
        extendFortress(env, p, 5, 3,  0, 1);
    } else if (p->type == CORRIDOR_STRAIGHT) {
        extendFortress(env, p, 1, 0,  0, 1);
    } else if (p->type == CORRIDOR_CROSSING) {
        extendFortress(env, p, 1, 0,  0, 1);
        extendFortress(env, p, 1, 0, -1, 1);
        extendFortress(env, p, 1, 0,  1, 1);
    } else if (p->type == CORRIDOR_TURN_RIGHT) {
        extendFortress(env, p, 1, 0,  1, 1);
    } else if (p->type == CORRIDOR_TURN_LEFT) {
        extendFortress(env, p, 1, 0, -1, 1);
    } else if (p->type == CORRIDOR_STAIRS) {
        extendFortress(env, p, 1, 0,  0, 1);
    } else if (p->type == CORRIDOR_T_CROSSING) {
        int h = (p->rot == 0 || p->rot == 3) ? 5 : 1;
        extendFortress(env, p, h, 0, -1, nextInt(8) != 0);
        extendFortress(env, p, h, 0,  1, nextInt(8) != 0);
    } else if (p->type == CORRIDOR_NETHER_WART) {
        extendFortress(env, p, 5, 3,  0, 1);
        extendFortress(env, p, 5, 11, 0, 1);
    }
}

int getFortressPieces(Piece *list, int n, int mc, uint64_t seed, int chunkX, int chunkZ)
{
    StructureConfig sconf;
    getStructureConfig(Fortress, mc, &sconf);

    if (mc >= MC_1_16) {
        Pos region = chunkToRegion(chunkX, chunkZ, sconf.regionSize);
        setRegionSeed(seed, region.x, region.z, sconf.salt);
        nextInt(26);
        nextInt(26);
        nextInt(6);
    } else {
        setFortressSeed(seed, chunkX, chunkZ);
        skipNextN(1);
        nextInt(3);
        nextInt(8);
        nextInt(8);
    }

    int count = 1;
    PieceEnv env;
    memset(&env, 0, sizeof(env));
    env.list = list;
    env.n = &count;
    env.ntyp[0] = 1;
    env.typlast = 0;
    env.nmax = n;
    Piece *p = list;
    Pos3 pos = {chunkX * 16 + 2, 64, chunkZ * 16 + 2};
    p->name = fortress_info[0].name;
    p->bb0 = p->bb1 = p->pos = pos;
    p->bb1.x += fortress_info[0].size.x;
    p->bb1.y += fortress_info[0].size.y;
    p->bb1.z += fortress_info[0].size.z;
    p->rot = (nextInt(4) + 2) % 4; // 0->2 1->3 2->0 3->1
    p->depth = 0;
    p->type = 0;
    p->next = NULL;
    extendFortressPiece(&env, p);
    while (list->next)
    {
        Piece *q = list;
        int len = 0;
        while (q->next)
        {
            q = q->next;
            len++;
        }
        int i = nextInt(len);
        for (p = list, q = list->next; i-->0; p = q, q = q->next);
        p->next = q->next;
        q->next = NULL;
        extendFortressPiece(&env, q);
    }
    moveInsideHeights(list, count, 48, 70);
    return count;
}

int getFortressSpawnerPos(const Piece *piece, Pos3 *spawner)
{
    if (piece == NULL || spawner == NULL || piece->type != BRIDGE_SPAWNER)
        return 0;

    spawner->y = piece->bb0.y + 5;
    switch (piece->rot & 3)
    {
    case 0: // north
        spawner->x = piece->bb0.x + 3;
        spawner->z = piece->bb1.z - 5;
        return 1;
    case 1: // east
        spawner->x = piece->bb0.x + 5;
        spawner->z = piece->bb0.z + 3;
        return 1;
    case 2: // south
        spawner->x = piece->bb0.x + 3;
        spawner->z = piece->bb0.z + 5;
        return 1;
    case 3: // west
        spawner->x = piece->bb1.x - 5;
        spawner->z = piece->bb0.z + 3;
        return 1;
    default:
        return 0;
    }
}

//==============================================================================
// Mineshaft Generator
//==============================================================================

typedef struct
{
    Piece *list;
    int count;
    int nmax;
    int type;
} MineshaftEnv;

static
const char *mineshaft_piece_names[] = {
    "MineshaftRoom",
    "MineshaftCorridor",
    "MineshaftCrossing",
    "MineshaftStairs",
};

static
Piece *addMineshaftPiece(MineshaftEnv *env, int typ,
        int x, int y, int z, int facing, int depth, Pos3 bb0, Pos3 bb1)
{
    Piece *p;

    if (env->count >= env->nmax)
        return NULL;
    if (getNextIntersectingPiece(env->list, env->count, bb0, bb1) != NULL)
        return NULL;

    p = env->list + env->count++;
    memset(p, 0, sizeof(*p));
    p->name = mineshaft_piece_names[typ];
    p->pos = (Pos3){x, y, z};
    p->bb0 = bb0;
    p->bb1 = bb1;
    p->rot = facing;
    p->depth = depth;
    p->type = typ;
    return p;
}

static
Piece *addMineshaftRoom(MineshaftEnv *env, int x, int y, int z)
{
    Pos3 bb0 = {x, y, z};
    Pos3 bb1;
    // z->y->x in bedrock
    bb1.z = z + nextInt(6) + 7;
    bb1.y = y + nextInt(6) + 4;
    bb1.x = x + nextInt(6) + 7;
    return addMineshaftPiece(env, MSHFT_ROOM, x, y, z, 0, 0, bb0, bb1);
}

static
Piece *findMineshaftCorridor(MineshaftEnv *env,
        int x, int y, int z, int facing, int depth)
{
    int attempts = nextInt(3) + 2;
    for (; attempts > 0; attempts--)
    {
        Pos3 bb0, bb1;
        strongholdRotatedBox(&bb0, &bb1, x, y, z, 0, 0, 0, 3, 3, attempts * 5, facing);
        if (getNextIntersectingPiece(env->list, env->count, bb0, bb1) == NULL)
        {
            Piece *p = addMineshaftPiece(env, MSHFT_CORRIDOR,
                x, y, z, facing, depth, bb0, bb1);
            if (p == NULL)
                return NULL;
            int hasRails = nextInt(3) == 0;
            if (!hasRails)
                nextInt(23); // spider corridor
            return p;
        }
    }
    return NULL;
}

static
Piece *findMineshaftCrossing(MineshaftEnv *env,
        int x, int y, int z, int facing, int depth)
{
    int sizeY = 2;
    Pos3 bb0, bb1;
    if (nextInt(4) == 0)
        sizeY += 4;
    strongholdRotatedBox(&bb0, &bb1, x, y, z, -1, 0, 0, 5, sizeY + 1, 5, facing);
    return addMineshaftPiece(env, MSHFT_CROSSING, x, y, z, facing, depth, bb0, bb1);
}

static
Piece *findMineshaftStairs(MineshaftEnv *env,
        int x, int y, int z, int facing, int depth)
{
    Pos3 bb0, bb1;
    strongholdRotatedBox(&bb0, &bb1, x, y, z, 0, -5, 0, 3, 8, 9, facing);
    return addMineshaftPiece(env, MSHFT_STAIRS, x, y, z, facing, depth, bb0, bb1);
}

static Piece *generateAndAddMineshaftPiece(MineshaftEnv *env,
        int x, int y, int z, int facing, int genDepth);

static
void addMineshaftRoomChildren(MineshaftEnv *env, const Piece *p)
{
    int axisLen, k, j, x, y, z;
    float prob = env->type == MINESHAFT_MESA ? 0.5f : 1.0f;

    if (nextFloat() > prob) // 50% spawn rate in mesa biomes
        return;

    k = p->bb1.y - p->bb0.y - 3;
    if (k < 1)
        k = 1;

    axisLen = p->bb1.x - p->bb0.x + 1;
    for (j = 0; j < axisLen; )
    {
        j += nextInt(axisLen);
        if (j + 3 > axisLen)
            break;
        x = p->bb0.x + j;
        y = p->bb0.y + nextInt(k) + 1;
        z = p->bb0.z - 1;
        generateAndAddMineshaftPiece(env, x, y, z, 0, p->depth);
        j += 4;
    }
    for (j = 0; j < axisLen; )
    {
        j += nextInt(axisLen);
        if (j + 3 > axisLen)
            break;
        x = p->bb0.x + j;
        y = p->bb0.y + nextInt(k) + 1;
        z = p->bb1.z + 1;
        generateAndAddMineshaftPiece(env, x, y, z, 2, p->depth);
        j += 4;
    }

    axisLen = p->bb1.z - p->bb0.z + 1;
    for (j = 0; j < axisLen; )
    {
        j += nextInt(axisLen);
        if (j + 3 > axisLen)
            break;
        x = p->bb0.x - 1;
        y = p->bb0.y + nextInt(k) + 1;
        z = p->bb0.z + j;
        generateAndAddMineshaftPiece(env, x, y, z, 3, p->depth);
        j += 4;
    }
    for (j = 0; j < axisLen; )
    {
        j += nextInt(axisLen);
        if (j + 3 > axisLen)
            break;
        x = p->bb1.x + 1;
        y = p->bb0.y + nextInt(k) + 1;
        z = p->bb0.z + j;
        generateAndAddMineshaftPiece(env, x, y, z, 1, p->depth);
        j += 4;
    }
}

static
void addMineshaftCorridorChildren(MineshaftEnv *env, const Piece *p)
{
    int branchType = nextInt(4);
    int yOffset = nextInt(3);
    int minX = p->bb0.x, minY = p->bb0.y, minZ = p->bb0.z;
    int maxX = p->bb1.x, maxY = p->bb1.y, maxZ = p->bb1.z;
    int x, z, branch;
    (void) maxY;

    switch (branchType)
    {
    case 0:
    case 1:
        switch (p->rot)
        {
        case 0: generateAndAddMineshaftPiece(env, minX,     minY - 1 + yOffset, minZ - 1, 0, p->depth); break;
        case 2: generateAndAddMineshaftPiece(env, minX,     minY - 1 + yOffset, maxZ + 1, 2, p->depth); break;
        case 3: generateAndAddMineshaftPiece(env, minX - 1, minY - 1 + yOffset, minZ,     3, p->depth); break;
        case 1: generateAndAddMineshaftPiece(env, maxX + 1, minY - 1 + yOffset, minZ,     1, p->depth); break;
        default: UNREACHABLE();
        }
        break;
    case 2:
        switch (p->rot)
        {
        case 0: generateAndAddMineshaftPiece(env, minX - 1, minY - 1 + yOffset, minZ,     3, p->depth); break;
        case 2: generateAndAddMineshaftPiece(env, minX - 1, minY - 1 + yOffset, maxZ - 3, 3, p->depth); break;
        case 3: generateAndAddMineshaftPiece(env, minX,     minY - 1 + yOffset, minZ - 1, 0, p->depth); break;
        case 1: generateAndAddMineshaftPiece(env, maxX - 3, minY - 1 + yOffset, minZ - 1, 0, p->depth); break;
        default: UNREACHABLE();
        }
        break;
    case 3:
        switch (p->rot)
        {
        case 0: generateAndAddMineshaftPiece(env, maxX + 1, minY - 1 + yOffset, minZ,     1, p->depth); break;
        case 2: generateAndAddMineshaftPiece(env, maxX + 1, minY - 1 + yOffset, maxZ - 3, 1, p->depth); break;
        case 3: generateAndAddMineshaftPiece(env, minX,     minY - 1 + yOffset, maxZ + 1, 2, p->depth); break;
        case 1: generateAndAddMineshaftPiece(env, maxX - 3, minY - 1 + yOffset, maxZ + 1, 2, p->depth); break;
        default: UNREACHABLE();
        }
        break;
    default:
        UNREACHABLE();
    }

    if (p->depth >= 8)
        return;

    if (p->rot == 0 || p->rot == 2)
    {
        for (z = minZ + 3; z + 3 <= maxZ; z += 5)
        {
            branch = nextInt(5);
            if      (branch == 0) generateAndAddMineshaftPiece(env, minX - 1, minY, z, 3, p->depth + 1);
            else if (branch == 1) generateAndAddMineshaftPiece(env, maxX + 1, minY, z, 1, p->depth + 1);
        }
    }
    else
    {
        for (x = minX + 3; x + 3 <= maxX; x += 5)
        {
            branch = nextInt(5);
            if      (branch == 0) generateAndAddMineshaftPiece(env, x, minY, minZ - 1, 0, p->depth + 1);
            else if (branch == 1) generateAndAddMineshaftPiece(env, x, minY, maxZ + 1, 2, p->depth + 1);
        }
    }
}

static
void addMineshaftCrossingChildren(MineshaftEnv *env, const Piece *p)
{
    int minX = p->bb0.x, minY = p->bb0.y, minZ = p->bb0.z;
    int maxX = p->bb1.x, maxZ = p->bb1.z;
    int twoFloored = (p->bb1.y - p->bb0.y + 1) > 3;

    switch (p->rot)
    {
    case 0:
        generateAndAddMineshaftPiece(env, minX + 1, minY, minZ - 1, 0, p->depth);
        generateAndAddMineshaftPiece(env, minX - 1, minY, minZ + 1, 3, p->depth);
        generateAndAddMineshaftPiece(env, maxX + 1, minY, minZ + 1, 1, p->depth);
        break;
    case 2:
        generateAndAddMineshaftPiece(env, minX + 1, minY, maxZ + 1, 2, p->depth);
        generateAndAddMineshaftPiece(env, minX - 1, minY, minZ + 1, 3, p->depth);
        generateAndAddMineshaftPiece(env, maxX + 1, minY, minZ + 1, 1, p->depth);
        break;
    case 3:
        generateAndAddMineshaftPiece(env, minX + 1, minY, minZ - 1, 0, p->depth);
        generateAndAddMineshaftPiece(env, minX + 1, minY, maxZ + 1, 2, p->depth);
        generateAndAddMineshaftPiece(env, minX - 1, minY, minZ + 1, 3, p->depth);
        break;
    case 1:
        generateAndAddMineshaftPiece(env, minX + 1, minY, minZ - 1, 0, p->depth);
        generateAndAddMineshaftPiece(env, minX + 1, minY, maxZ + 1, 2, p->depth);
        generateAndAddMineshaftPiece(env, maxX + 1, minY, minZ + 1, 1, p->depth);
        break;
    default:
        UNREACHABLE();
    }

    if (!twoFloored)
        return;

    if (nextBoolean()) generateAndAddMineshaftPiece(env, minX + 1, minY + 4, minZ - 1, 0, p->depth);
    if (nextBoolean()) generateAndAddMineshaftPiece(env, minX - 1, minY + 4, minZ + 1, 3, p->depth);
    if (nextBoolean()) generateAndAddMineshaftPiece(env, maxX + 1, minY + 4, minZ + 1, 1, p->depth);
    if (nextBoolean()) generateAndAddMineshaftPiece(env, minX + 1, minY + 4, maxZ + 1, 2, p->depth);
}

static
void addMineshaftStairsChildren(MineshaftEnv *env, const Piece *p)
{
    switch (p->rot)
    {
    case 0: generateAndAddMineshaftPiece(env, p->bb0.x, p->bb0.y, p->bb0.z - 1, 0, p->depth); break;
    case 2: generateAndAddMineshaftPiece(env, p->bb0.x, p->bb0.y, p->bb1.z + 1, 2, p->depth); break;
    case 3: generateAndAddMineshaftPiece(env, p->bb0.x - 1, p->bb0.y, p->bb0.z, 3, p->depth); break;
    case 1: generateAndAddMineshaftPiece(env, p->bb1.x + 1, p->bb0.y, p->bb0.z, 1, p->depth); break;
    default: UNREACHABLE();
    }
}

static
void addMineshaftChildren(MineshaftEnv *env, const Piece *p)
{
    switch (p->type)
    {
    case MSHFT_ROOM:
        addMineshaftRoomChildren(env, p);
        break;
    case MSHFT_CORRIDOR:
        addMineshaftCorridorChildren(env, p);
        break;
    case MSHFT_CROSSING:
        addMineshaftCrossingChildren(env, p);
        break;
    case MSHFT_STAIRS:
        addMineshaftStairsChildren(env, p);
        break;
    default:
        UNREACHABLE();
    }
}

static
Piece *createRandomMineshaftPiece(MineshaftEnv *env,
        int x, int y, int z, int facing, int depth)
{
    int pick = nextInt(100);
    if      (pick >= 80) return findMineshaftCrossing(env, x, y, z, facing, depth);
    else if (pick >= 70) return findMineshaftStairs  (env, x, y, z, facing, depth);
    else                 return findMineshaftCorridor(env, x, y, z, facing, depth);
}

static
Piece *generateAndAddMineshaftPiece(MineshaftEnv *env,
        int x, int y, int z, int facing, int genDepth)
{
    Piece *p;

    if (genDepth > 8)
        return NULL;
    if (env->count >= env->nmax)
        return NULL;
    if (IABS(x - env->list[0].bb0.x) > 80 || IABS(z - env->list[0].bb0.z) > 80)
        return NULL;

    p = createRandomMineshaftPiece(env, x, y, z, facing, genDepth + 1);
    if (p != NULL)
        addMineshaftChildren(env, p);
    return p;
}

int getMineshaftPieces(Piece *list, int n, int mc, uint64_t seed, int chunkX, int chunkZ, int type)
{
    MineshaftEnv env;
    Piece *start;
    int yOffset;
    int boxMinY, boxMaxY, boxHeight;
    int i;

    if (list == NULL || n <= 0)
        return 0;

    if (mc >= MC_1_11)
        setCarverSeed(seed, chunkX, chunkZ);
    else
        setSeed(seed ^ chunkZ ^ chunkX);
    skipNextN(1);
    if (nextFloat() >= 0.004f)
        return 0;
    if (nextInt(80) >= (IABS(chunkX) > IABS(chunkZ) ? IABS(chunkX) : IABS(chunkZ)))
        return 0;

    memset(&env, 0, sizeof(env));
    env.list = list;
    env.nmax = n;
    env.type = type;
    memset(list, 0, (size_t) n * sizeof(*list));

    start = addMineshaftRoom(&env, (chunkX << 4) + 2, 50, (chunkZ << 4) + 2);
    if (start == NULL)
        return 0;

    addMineshaftChildren(&env, start);

    boxMinY = list[0].bb0.y;
    boxMaxY = list[0].bb1.y;
    for (i = 1; i < env.count; i++)
    {
        if (list[i].bb0.y < boxMinY) boxMinY = list[i].bb0.y;
        if (list[i].bb1.y > boxMaxY) boxMaxY = list[i].bb1.y;
    }
    boxHeight = boxMaxY - boxMinY + 1;

    // move boxes
    if (type == MINESHAFT_MESA)
    {
        if (mc >= MC_1_18)
        {
            yOffset = 0;// TODO
        }
        else
        {
            yOffset = boxHeight / 2 - boxMaxY + 63 + 5;
        }
        offsetPiecesVertically(list, env.count, yOffset);
    }
    else
    {
        moveBelowSeaLevel(list, env.count, 63, mc >= MC_1_18 ? -64 : 0, 10);
    }

    return env.count;
}

//==============================================================================
// Dungeon Generator
//==============================================================================

typedef struct {
    int64_t key;
    int8_t val;
    uint8_t used;
} DungeonWSCell;
enum {
    AIR               = 0,
    SOLID             = 1,
    COBBLESTONE       = SOLID,
    MOSSY_COBBLESTONE = SOLID,
    CHEST             = SOLID,
    SPAWNER           = SOLID,
};

static uint64_t dungeonWsHash(int64_t key)
{
    uint64_t h = (uint64_t) key;
    h ^= h >> 33; h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h;
}

static int64_t dungeonWsKey(int x, int y, int z, int originX, int originZ)
{
    return ((int64_t)(uint16_t)(x - originX) << 32)
         | ((int64_t)(uint16_t)(z - originZ) << 16)
         | (uint16_t)(y);
}

static void dungeonWsSet(DungeonWSCell *tab, int cap, int64_t key, int val)
{
    uint64_t h = dungeonWsHash(key) & (uint32_t)(cap - 1);
    for (int i = 0; i < cap; i++)
    {
        int idx = (int)((h + (unsigned)i) & (uint32_t)(cap - 1));
        if (!tab[idx].used || tab[idx].key == key)
        {
            tab[idx].used = 1;
            tab[idx].key = key;
            tab[idx].val = (int8_t) val;
            return;
        }
    }
}

static int dungeonWsFind(const DungeonWSCell *tab, int cap, int64_t key, int *val)
{
    uint64_t h = dungeonWsHash(key) & (uint32_t)(cap - 1);
    for (int i = 0; i < cap; i++)
    {
        int idx = (int)((h + (unsigned)i) & (uint32_t)(cap - 1));
        if (!tab[idx].used)
            return 0;
        if (tab[idx].key == key)
        {
            *val = tab[idx].val;
            return 1;
        }
    }
    return 0;
}

static int dungeonCachedSolid(const BiomeNoise *bn, const TerrainNoise *tn,
        const TerrainShaper *ts, const CaveNoise *cn,
        TerrainCache *tc,
        DungeonWSCell *ws, int wscap, int originX, int originZ,
        int x, int y, int z)
{
    int64_t key = dungeonWsKey(x, y, z, originX, originZ);
    int val;
    if (dungeonWsFind(ws, wscap, key, &val))
        return val == SOLID;

    int solid;
    if (y <= -55)
        solid = 1; // replace lava
    else
        solid = isSolidCached(bn, tn, ts, cn, tc, x, y, z);

    val = solid ? SOLID : AIR;
    dungeonWsSet(ws, wscap, key, val);
    return val == SOLID;
}

static int dungeonPlace(const BiomeNoise *bn, const TerrainNoise *tn,
        const TerrainShaper *ts, const CaveNoise *cn, TerrainCache *tc,
        DungeonWSCell *ws, int wscap, int originX, int originZ,
        int ox, int oy, int oz, DungeonData *out, int *n, int nout)
{
    int xr = nextInt(2) + 2;
    int zr = nextInt(2) + 2;
    int minX = -xr - 1;
    int maxX =  xr + 1;
    int minY = -1;
    int maxY =  4;
    int minZ = -zr - 1;
    int maxZ =  zr + 1;

    // validate floor and ceiling
    int floorY   = oy + minY;
    int ceilingY = oy + maxY;
    for (int dx = minX; dx <= maxX; dx++)
    for (int dz = minZ; dz <= maxZ; dz++)
    {
        if (!dungeonCachedSolid(bn,tn,ts,cn,tc, ws, wscap, originX, originZ, ox+dx, floorY, oz+dz))
            return 0;
        if (!dungeonCachedSolid(bn,tn,ts,cn,tc, ws, wscap, originX, originZ, ox+dx, ceilingY, oz+dz))
            return 0;
    }

    // count side holes at floor level
    int holeCount = 0;
    for (int dx = minX; dx <= maxX; dx++)
    for (int dz = minZ; dz <= maxZ; dz++)
    {
        if (dx != minX && dx != maxX && dz != minZ && dz != maxZ)
            continue;

        int hx = ox + dx;
        int hz = oz + dz;
        if (!dungeonCachedSolid(bn,tn,ts,cn,tc, ws, wscap, originX, originZ, hx, oy,   hz) &&
            !dungeonCachedSolid(bn,tn,ts,cn,tc, ws, wscap, originX, originZ, hx, oy+1, hz))
        {
            holeCount++;
            if (holeCount > 5)
                return 0;
        }
    }

    if (holeCount < 1)
        return 0;

    // build walls
    for (int dx = minX; dx <= maxX; dx++)
    for (int dy = minY; dy <= maxY; dy++)
    for (int dz = minZ; dz <= maxZ; dz++)
    {
        int wx = ox+dx, wy = oy+dy, wz = oz+dz;
        int64_t key = dungeonWsKey(wx, wy, wz, originX, originZ);

        if (dx == minX || dy == minY || dz == minZ ||
            dx == maxX || dy == maxY || dz == maxZ)
        {
            if (!dungeonCachedSolid(bn,tn,ts,cn,tc, ws, wscap, originX, originZ, wx, wy-1, wz))
                dungeonWsSet(ws, wscap, key, AIR);
            else
            {
                if (dy == -1 && nextInt(4) != 0) 
                    dungeonWsSet(ws, wscap, key, MOSSY_COBBLESTONE);
                else 
                    dungeonWsSet(ws, wscap, key, COBBLESTONE);
            }
        }
        else
        {
            dungeonWsSet(ws, wscap, key, AIR);
        }
    }

    // place chests
    int xsize = xr * 2 + 1;
    int zsize = zr * 2 + 1;
    for (int i = 0; i < 2; i++) // 2 chests
    for (int j = 0; j < 3; j++) // 3 tries
    {
        // z -> x in bedrock
        int zc = oz + nextInt(zsize) - zr;
        int yc = oy;
        int xc = ox + nextInt(xsize) - xr;
        int64_t ckey = dungeonWsKey(xc, yc, zc, originX, originZ);

        int val;
        dungeonWsFind(ws, wscap, ckey, &val);
        if (val == AIR)
        {
            static const int ddx[] = {-1, 1, 0, 0};
            static const int ddz[] = { 0, 0, 1,-1};
            int wallCount = 0;
            int ok = 1;
            for (int k = 0; k < 4; k++)
            {
                int64_t nk = dungeonWsKey(xc+ddx[k], yc, zc+ddz[k], originX, originZ);
                dungeonWsFind(ws, wscap, nk, &val);
                if (val == SOLID)
                    wallCount++;
                // early exit
                if (wallCount > 1) {
                    ok = 0;
                    break;
                }
            }
            if (ok && wallCount == 1)
            {
                //printf("chest %d %d %d\n", xc, yc, zc);
                dungeonWsSet(ws, wscap, ckey, CHEST);
                next(); // loot seed
                out[*n].chests[i].x = xc;
                out[*n].chests[i].y = yc;
                out[*n].chests[i].z = zc;
                break;
            }
        }
    }

    // place spawner + pick mob type
    dungeonWsSet(ws, wscap, dungeonWsKey(ox, oy, oz, originX, originZ), SPAWNER);
    int type = DUNGEON_ZOMBIE; // default
    switch (nextInt(4))
    {
        case 0: type = DUNGEON_SKELETON; break;
        case 1: type = DUNGEON_ZOMBIE;   break;
        case 2: type = DUNGEON_ZOMBIE;   break;
        case 3: type = DUNGEON_SPIDER;   break;
    }

    if (out && *n < nout)
    {
        out[*n].pos.x = ox;
        out[*n].pos.y = oy;
        out[*n].pos.z = oz;
        out[*n].mobType = type;
    }
    (*n)++;
    return 1;
}

// skip nearby dungeons
// they generate after mineshafts,
// causing RNG shifts within the same chunk.
static int mineshaftNearby(int mc, uint64_t seed, int chunkX, int chunkZ)
{
    const int x0 = chunkX * 16;
    const int z0 = chunkZ * 16;
    const int x1 = x0 + 15;
    const int z1 = z0 + 15;
    const int radius = 7;
    Piece pieces[512];

    for (int cz = chunkZ - radius; cz <= chunkZ + radius; cz++)
    {
        for (int cx = chunkX - radius; cx <= chunkX + radius; cx++)
        {
            int n = getMineshaftPieces(pieces, sizeof(pieces)/sizeof(pieces[0]), mc, seed, cx, cz, MINESHAFT_NORMAL);
            for (int i = 0; i < n; i++)
            {
                if (pieces[i].bb1.x >= x0 && pieces[i].bb0.x <= x1 &&
                    pieces[i].bb1.z >= z0 && pieces[i].bb0.z <= z1)
                    return 1;
            }
        }
    }
    return 0;
}

int getDungeons(
    const BiomeNoise    *bn,
    const TerrainNoise  *tn,
    const TerrainShaper *ts,
    const CaveNoise     *cn,
    int mc, uint64_t seed,
    int chunkX, int chunkZ,
    DungeonData *out, int nout)
{
    if (mineshaftNearby(mc, seed, chunkX, chunkZ) || 
        mc < MC_1_18) // not supported
        return 0;

    DungeonWSCell *ws = (DungeonWSCell*) calloc(DUNGEON_CAP, sizeof(DungeonWSCell));
    if (!ws)
        return 0;
    TerrainCache *tc = (TerrainCache*) malloc(sizeof(TerrainCache));
    if (!tc)
    {
        free(ws);
        return 0;
    }
    initTerrainCache(tc);

    int originX = chunkX * 16 + 8;
    int originZ = chunkZ * 16 + 8;
    int n = 0;

    setPopulationSeed(seed, chunkX, chunkZ);
    if (mc >= MC_26_20) // actually 1.21.120+
        skipNextN(1);

    // below dungeon
    for (int idx = 0; idx < 4; idx++)
    {
        int x = nextInt(16) + originX;
        int y = nextIntRange(-58, 0); // -58 ~ -1
        int z = nextInt(16) + originZ;
        dungeonPlace(bn,tn,ts,cn, tc, ws, DUNGEON_CAP, originX, originZ, x, y, z, out, &n, nout);
    }
    // above dungeon
    for (int idx = 0; idx < 10; idx++)
    {
        int x = nextInt(16) + originX;
        int y = nextIntRange(0, 320); // 0 ~ 319
        int z = nextInt(16) + originZ;
        dungeonPlace(bn,tn,ts,cn, tc, ws, DUNGEON_CAP, originX, originZ, x, y, z, out, &n, nout);
    }

    free(ws);
    free(tc);
    return n;
}

//==============================================================================
// Outpost Generator
//==============================================================================

static const struct
{
    Pos3 size;
    const char* name;
}
outpost_info[] = {
    // vanilla\structures\pillageroutpost
    {{6, 5, 6}, "feature_cage1"}, // cage1 is the golem cage in Bedrock
    {{6, 5, 6}, "feature_cage2"},
    {{7, 4, 7}, "feature_cage_with_allays"},
    {{6, 3, 7}, "feature_logs"},
    {{3, 3, 7}, "feature_targets"},
    {{6, 5, 7}, "feature_tent1"},
    {{6, 5, 7}, "feature_tent2"},
};

const struct {
    int x, z;
}
scatter_bases[4] = {
    { 16, 16},
    {-16, 16},
    { 16,-16},
    {-16,-16}
};

int getOutpostPieces(Piece* pieces, uint64_t seed, int chunkX, int chunkZ)
{
    StructureConfig sc;
    getStructureConfig(Outpost, MC_NEWEST, &sc);
    Pos rpos = chunkToRegion(chunkX, chunkZ, sc.regionSize);
    setRegionSeed(seed, rpos.x, rpos.z, sc.salt);
    skipNextN(4);

    int rotation = nextInt(4);
    int baseX = chunkX << 4;
    int baseZ = chunkZ << 4;

    int size = 15;
    int ox, oz;
    switch (rotation) {
        case 0: ox =  size; oz =  size; break;
        case 1: ox = -size; oz =  size; break;
        case 2: ox = -size; oz = -size; break;
        case 3: ox =  size; oz = -size; break;
    }

    int centerX = baseX + ox;
    int centerZ = baseZ + oz;
    if (baseX <= centerX) centerX = baseX;
    if (baseZ <= centerZ) centerZ = baseZ;
    centerX += 8;
    centerZ += 8;

    Pos3 positions[4];
    for (int i = 0; i < 4; i++) {
        positions[i].x = centerX + scatter_bases[i].x - (4 - nextInt(8));
        positions[i].y = 90;
        positions[i].z = centerZ + scatter_bases[i].z - (4 - nextInt(8));
    }

    int n = 0;
    for (int i = 0; i < 4; i++) {
        if (nextFloat() >= 0.9f)
            continue;

        int rot = nextInt(4);
        int type = nextInt(7);

        Piece* p = &pieces[n];
        p->name = outpost_info[type].name;
        p->pos.x = positions[i].x;
        p->pos.y = positions[i].y;
        p->pos.z = positions[i].z;
        p->bb0.x = p->pos.x;
        p->bb0.y = p->pos.y;
        p->bb0.z = p->pos.z;
        p->bb1.x = p->pos.x;
        p->bb1.y = p->pos.y;
        p->bb1.z = p->pos.z;

        int sizeX = outpost_info[type].size.x;
        int sizeY = outpost_info[type].size.y;
        int sizeZ = outpost_info[type].size.z;

        switch(rot) {
            case 0: // North
                p->bb1.x += sizeX - 1;
                p->bb1.y += sizeY - 1;
                p->bb1.z += sizeZ - 1;
                break;
            case 1: // East
                p->bb0.x -= sizeZ + 1;
                p->bb1.y += sizeY - 1;
                p->bb1.z += sizeX - 1;
                break;
            case 2: // South
                p->bb0.x -= sizeX + 1;
                p->bb1.y += sizeY - 1;
                p->bb0.z -= sizeZ + 1;
                break;
            case 3: // West
                p->bb1.x += sizeZ - 1;
                p->bb1.y += sizeY - 1;
                p->bb0.z -= sizeX + 1;
                break;
        }
        p->rot = (uint8_t)rot;
        p->type = (int8_t)type;
    
        n++;
    }
    return n;
}

//==============================================================================
// Village Generator (1.0-1.10)
//==============================================================================

uint64_t getHouseList(int *out, uint64_t seed, int chunkX, int chunkZ)
{
    setRegionSeed(seed, chunkX, chunkZ, 10387312);
    skipNextN(2);

    out[HouseSmall] = nextInt(4 - 2 + 1) + 2;
    out[Church]     = nextInt(1 - 0 + 1) + 0;
    out[Library]    = nextInt(2 - 0 + 1) + 0;
    out[WoodHut]    = nextInt(5 - 2 + 1) + 2;
    out[Butcher]    = nextInt(2 - 0 + 1) + 0;
    out[FarmLarge]  = nextInt(4 - 1 + 1) + 1;
    out[FarmSmall]  = nextInt(4 - 2 + 1) + 2;
    out[Blacksmith] = nextInt(1 - 0 + 1) + 0;
    out[HouseLarge] = nextInt(3 - 0 + 1) + 0;

    return 1;
}

#define PV_F_DOWN  0
#define PV_F_UP    1
#define PV_F_NORTH 2
#define PV_F_SOUTH 3
#define PV_F_WEST  4
#define PV_F_EAST  5

typedef struct { int minX, minY, minZ, maxX, maxY, maxZ; } PV_BB;

static PV_BB pvbb_make(int x1,int y1,int z1,int x2,int y2,int z2)
{ PV_BB b={x1,y1,z1,x2,y2,z2}; return b; }

static PV_BB pvbb_empty(void)
{ return pvbb_make(0x7fffffff,0x7fffffff,0x7fffffff,(int)0x80000000,(int)0x80000000,(int)0x80000000); }

static int pvbb_intersects(const PV_BB *a, const PV_BB *b)
{
    return a->maxX >= b->minX && a->minX <= b->maxX &&
           a->maxZ >= b->minZ && a->minZ <= b->maxZ &&
           a->maxY >= b->minY && a->minY <= b->maxY;
}
static int pvbb_xsize(const PV_BB *b){ return b->maxX - b->minX + 1; }
static int pvbb_zsize(const PV_BB *b){ return b->maxZ - b->minZ + 1; }

static PV_BB pvbb_component(int sx,int sy,int sz,
                             int xMin,int yMin,int zMin,
                             int xMax,int yMax,int zMax,int facing)
{
    switch(facing){
    case PV_F_NORTH:
        return pvbb_make(sx+xMin, sy+yMin, sz-zMax+1+zMin, sx+xMax-1+xMin, sy+yMax-1+yMin, sz+zMin);
    case PV_F_SOUTH:
        return pvbb_make(sx+xMin, sy+yMin, sz+zMin, sx+xMax-1+xMin, sy+yMax-1+yMin, sz+zMax-1+zMin);
    case PV_F_WEST:
        return pvbb_make(sx-zMax+1+zMin, sy+yMin, sz+xMin, sx+zMin, sy+yMax-1+yMin, sz+xMax-1+xMin);
    case PV_F_EAST:
        return pvbb_make(sx+zMin, sy+yMin, sz+xMin, sx+zMax-1+zMin, sy+yMax-1+yMin, sz+xMax-1+xMin);
    default:
        return pvbb_make(sx+xMin, sy+yMin, sz+zMin, sx+xMax-1+xMin, sy+yMax-1+yMin, sz+zMax-1+zMin);
    }
}

typedef struct {
    int vpType;
    int facing;
    int depth;
    PV_BB bb;
    int length;
} PV_Piece;

#define PV_MAX_PIECES 512
#define PV_MAX_PENDING 512

typedef struct { PV_Piece arr[PV_MAX_PIECES]; int n; } PV_PieceArr;
typedef struct { int idx[PV_MAX_PENDING]; int n; } PV_PendingList;

static void pv_pa_add(PV_PieceArr *pa, const PV_Piece *p)
{ if(pa->n < PV_MAX_PIECES) pa->arr[pa->n++] = *p; }

static void pv_pl_add(PV_PendingList *pl, int i)
{ if(pl->n < PV_MAX_PENDING) pl->idx[pl->n++] = i; }

static int pv_pl_remove(PV_PendingList *pl, int pos)
{
    int v = pl->idx[pos];
    for(int k=pos; k<pl->n-1; k++) pl->idx[k]=pl->idx[k+1];
    pl->n--;
    return v;
}

#define PV_NUM_PW 9
typedef struct { int vpType; int weight; int spawned; int limit; } PV_PW;
typedef struct { PV_PW pw[PV_NUM_PW]; int n; } PV_PWList;

static const int PV_PW_VPTYPES[PV_NUM_PW] = {
    VP_HOUSE4G, VP_CHURCH, VP_HOUSE1, VP_WOODHUT, VP_HALL,
    VP_FIELD1, VP_FIELD2, VP_HOUSE2, VP_HOUSE3
};
static const int PV_PW_WEIGHTS[PV_NUM_PW] = { 4,20,20,3,15,3,3,15,8 };

static void pv_pw_init(PV_PWList *pwl, const int limits[PV_NUM_PW])
{
    pwl->n = 0;
    for (int i=0; i<PV_NUM_PW; i++)
    {
        if (limits[i]>0)
        {
            pwl->pw[pwl->n].vpType = PV_PW_VPTYPES[i];
            pwl->pw[pwl->n].weight = PV_PW_WEIGHTS[i];
            pwl->pw[pwl->n].spawned = 0;
            pwl->pw[pwl->n].limit = limits[i];
            pwl->n++;
        }
    }
}

static int pv_pw_total(const PV_PWList *pwl)
{
    int total=0, hasAvail=0;
    for (int i=0; i<pwl->n; i++)
    {
        if (pwl->pw[i].spawned < pwl->pw[i].limit) hasAvail=1;
        total += pwl->pw[i].weight;
    }
    return hasAvail ? total : -1;
}

static void pv_pw_remove(PV_PWList *pwl, int wi)
{
    for (int m=wi; m<pwl->n-1; m++) pwl->pw[m]=pwl->pw[m+1];
    pwl->n--;
}

typedef struct {
    PV_PieceArr *pieces;
    PV_PendingList pendingRoads;
    PV_PendingList pendingHouses;
    PV_PWList weights;
    int lastPlacedPWIdx;
    int terrainType;
    PV_BB startBB;
} PV_VStart;

static int pv_find_intersecting(const PV_PieceArr *pa, const PV_BB *bb)
{
    for (int i=0; i<pa->n; i++)
    {
        if (pvbb_intersects(&pa->arr[i].bb, bb))
        {
            return i;
        }
    }
    return -1;
}

static int pv_path_find_box(PV_VStart *vs, int x,int y,int z,int facing, PV_BB *out)
{
    int initLen = 7 * nextIntRange(3,6);
    for (int i=initLen; i>=7; i-=7)
    {
        PV_BB bb = pvbb_component(x,y,z, 0,0,0, 3,3,i, facing);
        if (pv_find_intersecting(vs->pieces, &bb) < 0)
        {
            *out=bb;
            return 1;
        }
    }
    return 0;
}

static int pv_torch_find_box(PV_VStart *vs, int x,int y,int z,int facing, PV_BB *out)
{
    PV_BB bb = pvbb_component(x,y,z, 0,0,0, 3,4,2, facing);
    if (pv_find_intersecting(vs->pieces, &bb) < 0)
    {
        *out=bb;
        return 1;
    }
    return 0;
}

static int pv_gen_add_road(PV_VStart *vs, int x,int y,int z,int facing,int depth)
{
    if (depth > 3 + vs->terrainType) return -1;
    int dx = x - vs->startBB.minX; if(dx<0) dx=-dx;
    int dz = z - vs->startBB.minZ; if(dz<0) dz=-dz;
    if (dx>112 || dz>112) return -1;

    PV_BB bb;
    if (!pv_path_find_box(vs,x,y,z,facing,&bb)) return -1;
    if (bb.minY <= 10) return -1;

    PV_Piece p;
    p.vpType = VP_PATH; p.facing = facing; p.depth = depth; p.bb = bb;
    int xs = pvbb_xsize(&bb);
    int zs = pvbb_zsize(&bb);
    p.length = xs > zs ? xs : zs;
    int idx = vs->pieces->n;
    pv_pa_add(vs->pieces, &p);
    pv_pl_add(&vs->pendingRoads, idx);
    return idx;
}

static int pv_create_building(PV_VStart *vs, int vpType,
                               int x,int y,int z,int facing,int depth)
{
    PV_BB bb;
    switch(vpType)
    {
    case VP_HOUSE4G:
        bb = pvbb_component(x,y,z, 0,0,0, 5,6,5, facing);
        if (pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        nextBoolean(); break;
    case VP_CHURCH:
        bb = pvbb_component(x,y,z, 0,0,0, 5,12,9, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        break;
    case VP_HOUSE1:
        bb = pvbb_component(x,y,z, 0,0,0, 9,9,6, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        break;
    case VP_WOODHUT:
        bb = pvbb_component(x,y,z, 0,0,0, 4,6,5, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        nextBoolean(); nextInt(3); break;
    case VP_HALL:
        bb = pvbb_component(x,y,z, 0,0,0, 9,7,11, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        break;
    case VP_FIELD1:
        bb = pvbb_component(x,y,z, 0,0,0, 13,4,9, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        nextInt(10); nextInt(10); nextInt(10); nextInt(10); break;
    case VP_FIELD2:
        bb = pvbb_component(x,y,z, 0,0,0, 7,4,9, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        nextInt(10); nextInt(10); break;
    case VP_HOUSE2:
        bb = pvbb_component(x,y,z, 0,0,0, 10,6,7, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        break;
    case VP_HOUSE3:
        bb = pvbb_component(x,y,z, 0,0,0, 9,7,12, facing);
        if (bb.minY<=10 || pv_find_intersecting(vs->pieces,&bb)>=0) return -1;
        break;
    default: return -1;
    }
    PV_Piece p; p.vpType=vpType; p.facing=facing; p.depth=depth; p.bb=bb; p.length=0;
    int idx = vs->pieces->n;
    pv_pa_add(vs->pieces, &p);
    return idx;
}

static int pv_gen_component(PV_VStart *vs, int x,int y,int z,int facing,int depth)
{
    int total = pv_pw_total(&vs->weights);
    if (total<=0)
        return -1;
    for (int attempt=0; attempt<5; attempt++)
    {
        int k = nextInt(total);
        for (int wi=0; wi<vs->weights.n; wi++)
        {
            PV_PW *pw = &vs->weights.pw[wi];
            k -= pw->weight;
            if (k>=0)
                continue;
            int canSpawn = (pw->spawned < pw->limit);
            int isLast = (wi == vs->lastPlacedPWIdx && vs->weights.n > 1);
            if (!canSpawn || isLast)
                break;
            int idx = pv_create_building(vs, pw->vpType, x, y, z, facing, depth);
            if (idx<0)
                continue;
            pw->spawned++;
            vs->lastPlacedPWIdx = wi;
            if (pw->spawned >= pw->limit)
            {
                pv_pw_remove(&vs->weights, wi);
                if (vs->lastPlacedPWIdx == wi)
                {
                    vs->lastPlacedPWIdx = -1;
                }
                else if (vs->lastPlacedPWIdx > wi)
                {
                    vs->lastPlacedPWIdx--;
                }
            }
            return idx;
        }
    }
    PV_BB bb;
    if (pv_torch_find_box(vs,x,y,z,facing,&bb))
    {
        PV_Piece p; p.vpType=VP_TORCH; p.facing=facing; p.depth=depth; p.bb=bb; p.length=0;
        int idx = vs->pieces->n;
        pv_pa_add(vs->pieces, &p);
        return idx;
    }
    return -1;
}

static int pv_gen_add_component(PV_VStart *vs, int x,int y,int z,int facing,int depth)
{
    if (depth>50)
        return -1;
    int dx = x - vs->startBB.minX; if(dx<0) dx=-dx;
    int dz = z - vs->startBB.minZ; if(dz<0) dz=-dz;
    if (dx>112 || dz>112)
        return -1;
    int idx = pv_gen_component(vs,x,y,z,facing,depth+1);
    if (idx>=0)
    {
        pv_pl_add(&vs->pendingHouses, idx);
    }
    return idx;
}

static int pv_get_next_nn(PV_VStart *vs, int pi, int yOff, int zOff)
{
    const PV_Piece *p = &vs->pieces->arr[pi];
    switch (p->facing)
    {
    case PV_F_NORTH:
    case PV_F_SOUTH:
        return pv_gen_add_component(vs, p->bb.minX-1, p->bb.minY+yOff, p->bb.minZ+zOff, PV_F_WEST, p->depth);
    case PV_F_WEST:
    case PV_F_EAST:
        return pv_gen_add_component(vs, p->bb.minX+zOff, p->bb.minY+yOff, p->bb.minZ-1, PV_F_NORTH, p->depth);
    default: return -1;
    }
}

static int pv_get_next_pp(PV_VStart *vs, int pi, int yOff, int zOff)
{
    const PV_Piece *p = &vs->pieces->arr[pi];
    switch (p->facing)
    {
    case PV_F_NORTH:
    case PV_F_SOUTH:
        return pv_gen_add_component(vs, p->bb.maxX+1, p->bb.minY+yOff, p->bb.minZ+zOff, PV_F_EAST, p->depth);
    case PV_F_WEST:
    case PV_F_EAST:
        return pv_gen_add_component(vs, p->bb.minX+zOff, p->bb.minY+yOff, p->bb.maxZ+1, PV_F_SOUTH, p->depth);
    default: return -1;
    }
}

static void pv_path_build(PV_VStart *vs, int pi)
{
    int length = vs->pieces->arr[pi].length;
    int facing = vs->pieces->arr[pi].facing;
    int flag = 0;
    for (int i=nextInt(5); i<length-8; i+=2+nextInt(5))
    {
        int idx = pv_get_next_nn(vs,pi,0,i);
        if (idx>=0)
        {
            int xs=pvbb_xsize(&vs->pieces->arr[idx].bb);
            int zs=pvbb_zsize(&vs->pieces->arr[idx].bb);
            i += (xs>zs?xs:zs); flag=1;
        }
    }
    for (int j=nextInt(5); j<length-8; j+=2+nextInt(5))
    {
        int idx = pv_get_next_pp(vs,pi,0,j);
        if (idx>=0)
        {
            int xs=pvbb_xsize(&vs->pieces->arr[idx].bb);
            int zs=pvbb_zsize(&vs->pieces->arr[idx].bb);
            j += (xs>zs?xs:zs); flag=1;
        }
    }
    if (flag && nextInt(3)>0 && facing!=-1)
    {
        const PV_BB *b = &vs->pieces->arr[pi].bb;
        int dep = vs->pieces->arr[pi].depth;
        switch (facing)
        {
        default:
        case PV_F_NORTH: pv_gen_add_road(vs,b->minX-1,b->minY,b->minZ+0,PV_F_WEST, dep); break;
        case PV_F_SOUTH: pv_gen_add_road(vs,b->minX-1,b->minY,b->maxZ-2,PV_F_WEST, dep); break;
        case PV_F_WEST:  pv_gen_add_road(vs,b->minX+0,b->minY,b->minZ-1,PV_F_NORTH,dep); break;
        case PV_F_EAST:  pv_gen_add_road(vs,b->maxX-2,b->minY,b->minZ-1,PV_F_NORTH,dep); break;
        }
    }
    if (flag && nextInt(3)>0 && facing!=-1)
    {
        const PV_BB *b = &vs->pieces->arr[pi].bb;
        int dep = vs->pieces->arr[pi].depth;
        switch (facing)
        {
        default:
        case PV_F_NORTH: pv_gen_add_road(vs,b->maxX+1,b->minY,b->minZ+0,PV_F_EAST, dep); break;
        case PV_F_SOUTH: pv_gen_add_road(vs,b->maxX+1,b->minY,b->maxZ-2,PV_F_EAST, dep); break;
        case PV_F_WEST:  pv_gen_add_road(vs,b->minX+0,b->minY,b->maxZ+1,PV_F_SOUTH,dep); break;
        case PV_F_EAST:  pv_gen_add_road(vs,b->maxX-2,b->minY,b->maxZ+1,PV_F_SOUTH,dep); break;
        }
    }
}

static void pv_well_build(PV_VStart *vs)
{
    const PV_BB *b = &vs->pieces->arr[0].bb;
    int dep = vs->pieces->arr[0].depth;
    pv_gen_add_road(vs, b->minX-1, b->maxY-4, b->minZ+1, PV_F_WEST,  dep);
    pv_gen_add_road(vs, b->maxX+1, b->maxY-4, b->minZ+1, PV_F_EAST,  dep);
    pv_gen_add_road(vs, b->minX+1, b->maxY-4, b->minZ-1, PV_F_NORTH, dep);
    pv_gen_add_road(vs, b->minX+1, b->maxY-4, b->maxZ+1, PV_F_SOUTH, dep);
}

static int pv_facing_to_rot(int facing)
{
    switch (facing)
    {
    case PV_F_SOUTH: return 0;
    case PV_F_WEST:  return 1;
    case PV_F_NORTH: return 2;
    case PV_F_EAST:  return 3;
    default: return 0;
    }
}

int getPreVillagePieces(Piece *list, int n, uint64_t seed, int chunkX, int chunkZ)
{
    static const int hf[4] = { PV_F_SOUTH, PV_F_WEST, PV_F_NORTH, PV_F_EAST };
    int regX = chunkX < 0 ? chunkX - 40+1 : chunkX;
    int regZ = chunkZ < 0 ? chunkZ - 40+1 : chunkZ;
    setRegionSeed(seed, regX, regZ, 10387312);
    next(); // separation x
    next(); // separation z

    int limits[PV_NUM_PW];
    limits[0] = nextIntRange(2,5); // HOUSE4G
    limits[1] = nextIntRange(0,2); // CHURCH
    limits[2] = nextIntRange(0,3); // HOUSE1
    limits[3] = nextIntRange(2,6); // WOODHUT
    limits[4] = nextIntRange(0,3); // HALL
    limits[5] = nextIntRange(1,5); // FIELD1
    limits[6] = nextIntRange(2,5); // FIELD2
    limits[7] = nextIntRange(0,2); // HOUSE2
    limits[8] = nextIntRange(0,4); // HOUSE3

    PV_PieceArr pa;
    memset(&pa, 0, sizeof(pa));

    PV_VStart vs;
    memset(&vs, 0, sizeof(vs));
    vs.pieces          = &pa;
    vs.lastPlacedPWIdx = -1;
    vs.terrainType     = 0;
    pv_pw_init(&vs.weights, limits);

    int wx = (chunkX << 4) + 2;
    int wz = (chunkZ << 4) + 2;

    PV_Piece wellPiece;
    wellPiece.vpType = VP_WELL;
    wellPiece.facing = hf[nextInt(4)];
    wellPiece.depth = 0;
    wellPiece.bb = pvbb_make(wx, 64, wz, wx+5, 78, wz+5);
    wellPiece.length = 0;

    vs.startBB = wellPiece.bb;
    pv_pa_add(&pa, &wellPiece);
    pv_well_build(&vs);

    while (vs.pendingRoads.n > 0 || vs.pendingHouses.n > 0)
    {
        if (vs.pendingRoads.n == 0)
        {
            int i = nextInt(vs.pendingHouses.n);
            pv_pl_remove(&vs.pendingHouses, i);
        }
        else
        {
            int j = nextInt(vs.pendingRoads.n);
            int idx = pv_pl_remove(&vs.pendingRoads, j);
            pv_path_build(&vs, idx);
        }
    }

    int count = pa.n < n ? pa.n : n;
    for (int i = 0; i < count; i++)
    {
        const PV_Piece *src = &pa.arr[i];
        Piece *dst = &list[i];
        dst->name  = NULL;
        dst->pos.x = (src->bb.minX + src->bb.maxX) / 2;
        dst->pos.y = src->bb.minY;
        dst->pos.z = (src->bb.minZ + src->bb.maxZ) / 2;
        dst->bb0.x = src->bb.minX;
        dst->bb0.y = src->bb.minY;
        dst->bb0.z = src->bb.minZ;
        dst->bb1.x = src->bb.maxX;
        dst->bb1.y = src->bb.maxY;
        dst->bb1.z = src->bb.maxZ;
        dst->rot   = (uint8_t)pv_facing_to_rot(src->facing);
        dst->depth = (int8_t)(src->depth < 127 ? src->depth : 127);
        dst->type  = (int8_t)src->vpType;
        dst->next  = NULL;
    }
    return count;
}

//==============================================================================
// Chorus Plant Generator (Pre-RNG consumption for Bedrock End Gateway)
//==============================================================================

typedef struct {
    Pos3 blocks[1024];
    int count;
} Blocks;

static
void setBlock(Blocks *m, int x, int y, int z) {
    for (int i = 0; i < m->count; i++) {
        if (m->blocks[i].x == x && m->blocks[i].y == y && m->blocks[i].z == z)
            return;
    }
    m->blocks[m->count++] = (Pos3){x, y, z};
}

static
int isEmptyBlock(Blocks *blocks, int x, int y, int z) {
    for (int i = 0; i < blocks->count; i++) {
        if (blocks->blocks[i].x == x && blocks->blocks[i].y == y && blocks->blocks[i].z == z)
            return 0;
    }
    return 1;
}

static
Pos3 relative(Pos3 pos, int direction)
{
    int dx[4] = {0, 1, 0, -1};
    int dz[4] = {-1, 0, 1, 0};
    Pos3 p = {pos.x + dx[direction], pos.y, pos.z + dz[direction]};
    return p;
}

static
int allNeighborsEmpty(Blocks *blocks, Pos3 b, int direction)
{
    for (int i = 0; i < 4; i++)
    {
        Pos3 p = relative(b, i);
        if (i != direction && !isEmptyBlock(blocks, p.x, p.y, p.z))
            return 0;
    }
    return 1;
}

static
void growTreeRecursive(Blocks *blocks, Pos3 pos1, Pos3 pos2, int depth)
{
    int opposite[4] = {2, 3, 0, 1};
    int f = nextInt(4) + 1;
    if (depth == 0)
        f++;
    for (int g = 0; g < f; g++)
    {
        Pos3 h = {pos1.x, pos1.y + (g + 1), pos1.z};
        if (!allNeighborsEmpty(blocks, h, -1))
            return;
        setBlock(blocks, h.x, h.y, h.z);
        setBlock(blocks, h.x, h.y - 1, h.z);
    }
    int br = 0;
    if (depth < 4)
    {
        int j = nextInt(4);
        if (depth == 0)
            j++;
        for (int k = 0; k < j; k++)
        {
            int direction = nextInt(4);
            Pos3 v = {pos1.x, pos1.y + f, pos1.z};
            Pos3 m = relative(v, direction);
            if (abs(m.x - pos2.x) < 8 &&
                abs(m.z - pos2.z) < 8 &&
                isEmptyBlock(blocks, m.x, m.y, m.z) &&
                isEmptyBlock(blocks, m.x, m.y - 1, m.z) &&
                allNeighborsEmpty(blocks, m, opposite[direction]))
            {
                br = 1;
                setBlock(blocks, m.x, m.y, m.z);
                Pos3 n = relative(m, opposite[direction]);
                setBlock(blocks, n.x, n.y, n.z);
                growTreeRecursive(blocks, m, pos2, depth + 1);
            }
        }
    }
    if (!br)
        setBlock(blocks, pos1.x, pos1.y + 1, pos1.z);
}

static
void generatePlant(Blocks *blocks, Pos3 pos)
{
    setBlock(blocks, pos.x, pos.y, pos.z);
    growTreeRecursive(blocks, pos, pos, 0);
}

int getEndGatewayPos(uint64_t seed, EndNoise en, SurfaceNoise sn, int chunkX, int chunkZ, Pos *pos)
{
    if (chunkX*chunkX + chunkZ*chunkZ < 64*64)
        return 0;
    int id;
    mapEndBiome(&en, &id, chunkX, chunkZ, 1, 1);
    if (id != end_highlands)
        return 0;
    Blocks blocks = {0};
    setPopulationSeed(seed, chunkX, chunkZ);
    if (en.mc >= MC_1_18)
        skipNextN(1);
    // need to skip chorus plant rng
    int i = nextInt(5);// plant count
    for (int j = 0; j < i; j++)
    {
        int bx = chunkX*16 + nextInt(16)+8;
        int bz = chunkZ*16 + nextInt(16)+8;
        float by;
        mapEndSurfaceHeight(&by, &en, &sn, bx, bz, 1, 1, 1, 0);
        Pos3 p = {bx, (int)by + 1, bz};
        generatePlant(&blocks, p);
    }
    // end gateway generation
    if (nextInt(700) != 0) return 0;
    int bx = chunkX*16 + nextInt(16)+8;
    int bz = chunkZ*16 + nextInt(16)+8;
    float by;
    mapEndSurfaceHeight(&by, &en, &sn, bx, bz, 1, 1, 1, 0);
    if ((int)by <= 0)
        return 0;// blockAbove->isAir() && blockBelow->isEndStone()
    pos->x = bx;
    pos->z = bz;
    //y = by + nextInt(7) + 3;
    return 1;
}

void getFixedEndGateways(int mc, uint64_t seed, Pos src[20])
{
    (void) mc;
    static const Pos fixed[20] = {
        { 96,  0}, { 91, 29}, { 77, 56}, { 56, 77}, { 29, 91},
        { -1, 96}, {-29, 91}, {-57, 77}, {-78, 56}, {-91, 29},
        {-96, -1}, {-91,-29}, {-78,-57}, {-57,-78}, {-29,-92},
        {  0,-96}, { 29,-91}, { 56,-78}, { 77,-57}, { 91,-29},
    };

    uint8_t order[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
    };

    int i;
    setSeed(seed);

    for (i = 1; i < 20; i++)
    {
        uint8_t j = nextInt(i+1);
        uint8_t tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    for (i = 0; i < 20; i++)
        src[i] = fixed[ order[19 - i] ];
}

Pos getLinkedGatewayChunk(const EndNoise *en, const SurfaceNoise *sn, uint64_t seed,
    Pos src, Pos *dst)
{
    double invr = 1.0 / sqrt(src.x * src.x + src.z * src.z);
    double dx = src.x * invr;
    double dz = src.z * invr;
    double px = dx * 1024;
    double pz = dz * 1024;
    dx *= 16;
    dz *= 16;

    int i;
    Pos c;
    c.x = (int) floor(px) >> 4;
    c.z = (int) floor(pz) >> 4;

    if (isEndChunkEmpty(en, sn, seed, c.x, c.z))
    {   // look forward for the first non-empty chunk
        for (i = 0; i < 15; i++)
        {
            int qx = (int) floor(px += dx) >> 4;
            int qz = (int) floor(pz += dz) >> 4;
            if (qx == c.x && qz == c.z)
                continue;
            c.x = qx;
            c.z = qz;
            if (!isEndChunkEmpty(en, sn, seed, c.x, c.z))
                break;
        }
    }
    else
    {   // look backward for the last non-empty chunk
        for (i = 0; i < 15; i++)
        {
            int qx = (int) floor(px -= dx) >> 4;
            int qz = (int) floor(pz -= dz) >> 4;
            if (isEndChunkEmpty(en, sn, seed, qx, qz))
                break;
            c.x = qx;
            c.z = qz;
        }
    }
    if (dst)
    {
        dst->x = (int) floor(px);
        dst->z = (int) floor(pz);
    }
    return c;
}

Pos3 getLinkedGatewayPos(const EndNoise *en, const SurfaceNoise *sn, uint64_t seed, Pos src)
{
    float y[33*33]; // buffer for [16][16] and [33][33]
    int ymin = 0;
    int i, j;

    Pos dst;
    Pos c = getLinkedGatewayChunk(en, sn, seed, src, &dst);

    //if (en->mc > MC_1_16)
    //{
    //    // The original java implementation has a bug where the result
    //    // variable for the in-chunk block search is assigned a reference
    //    // to the mutable iterator, which ends up as the last iteration
    //    // position and discards the found location.
    //    dst.x = c.x * 16 + 15;
    //    dst.z = c.z * 16 + 15;
    //}
    //else
    {
        mapEndSurfaceHeight(y, en, sn, c.x*16, c.z*16, 16, 16, 1, 30);
        mapEndIslandHeight(y, en, seed, c.x*16, c.z*16, 16, 16, 1);

        uint64_t d = 0;
        for (j = 0; j < 16; j++)
        {
            for (i = 0; i < 16; i++)
            {
                int v = (int) y[j*16 + i];
                if (v < 30) continue;
                uint64_t dx = 16*c.x + i;
                uint64_t dz = 16*c.z + j;
                uint64_t dr = dx*dx + dz*dz + v*v;
                if (dr > d)
                {
                    d = dr;
                    dst.x = dx;
                    dst.z = dz;
                }
            }
        }
        // use the previous result to retrieve the minimum y-level we know,
        // we can skip generation of surfaces that are lower than this
        for (i = 0; i < 16*16; i++)
            if (y[i] > ymin)
                ymin = (int) floor(y[i]);
    }

    Pos sp = { dst.x-16, dst.z-16 };
    // checking end islands is much cheaper than surface height generation, so
    // we can also skip surface generation lower than the highest island around
    memset(y, 0, sizeof(float)*33*33);
    mapEndIslandHeight(y, en, seed, sp.x, sp.z, 33, 33, 1);
    for (i = 0; i < 33*33; i++)
        if (y[i] > ymin)
            ymin = (int) floor(y[i]);

    mapEndSurfaceHeight(y, en, sn, sp.x, sp.z, 33, 33, 1, ymin);
    mapEndIslandHeight(y, en, seed, sp.x, sp.z, 33, 33, 1);

    float v = -1;
    for (i = 0; i < 33; i++)
    {
        for (j = 0; j < 33; j++)
        {
            if (y[j*33 + i] <= v)
                continue;
            v = y[j*33 + i];
            dst.x = sp.x + i;
            dst.z = sp.z + j;
        }
    }

    Pos3 result = { dst.x, (int)v + 1, dst.z };
    return result;
}


//==============================================================================
// Seed Filters
//==============================================================================


double inverf(double x)
{   // compute the inverse error function via newton's method
    double t = x, dt = 1;
    while (fabs(dt) > FLT_EPSILON)
    {
        dt = 0.5 * sqrt(PI) * (erf(t) - x) / exp(-t*t);
        t -= dt;
    }
    return t;
}

void wilson(double n, double p, double z, double *lo, double *hi)
{   // compute the wilson score interval
    double s = z * z / n;
    double t = 1 / (1 + s);
    double w = t * (p + 0.5 * s);
    double d = t * z * sqrt( (p*(1-p) + 0.25*s) / n ) + FLT_EPSILON;
    *lo = w - d;
    *hi = w + d;
}

int monteCarloBiomes(
        Generator         * g,
        Range               r,
        uint64_t          * rng,
        double              coverage,
        double              confidence,
        int (*eval)(Generator *g, int scale, int x, int y, int z, void*),
        void              * data
        )
{
    if (r.sy == 0)
        r.sy = 1;

    struct touple { int x, y, z; } *buf = 0;
    size_t n = (size_t)r.sx*r.sy*r.sz;

    // z-score (i.e. probit, or standard deviations) for the confidence
    double zscore = sqrt(2.0) * inverf(confidence);

    // One standard deviation is approximately given by sqrt(n) elements,
    // hence we will take zscore * sqrt(n) as the maximum number of samples
    // to reach the desired confidence. This gives us a wilson score interval
    // for the upper and lower bound of successes we aim for.
    double wn = zscore * sqrt(n);
    double wlo, whi;
    wilson(wn, coverage, zscore, &wlo, &whi);

    // When the number of samples approaches the total number elements,
    // we can avoid repeated samples by shuffling a buffer.
    // (TODO: adjust for hypergeometric distribution?)
    if (n < 4 * wn && n < INT_MAX)
        buf = (struct touple*) malloc(n * sizeof(*buf));

    if (buf)
    {
        size_t idx = 0;
        int i, k, j;
        for (k = 0; k < r.sy; k++)
        {
            for (j = 0; j < r.sz; j++)
            {
                for (i = 0; i < r.sx; i++)
                {
                    buf[idx].x = i;
                    buf[idx].y = k;
                    buf[idx].z = j;
                    idx++;
                }
            }
        }
    }

    size_t i = 0;
    double m = 0; // number of samples
    double x = 0; // number of successes
    int ret = 1;

    // iterate over the area in a random order
    for (i = 0; i < n; i++)
    {
        struct touple t;
        if (buf)
        {
            int j = n - i;
            int k = JnextInt(rng, j);
            t = buf[k];
            if (k != j-1)
            {
                buf[k] = buf[j-1];
                buf[j-1] = t;
            }
        }
        else
        {
            t.x = JnextInt(rng, r.sx);
            t.y = JnextInt(rng, r.sy);
            t.z = JnextInt(rng, r.sz);
        }

        int status = eval(g, r.scale, r.x+t.x, r.y+t.y, r.z+t.z, data);
        if (status == -1)
            continue;
        else if (status == 0)
            ;
        else if (status == 1)
            x += 1.0;
        else
        {
            ret = 0;
            break;
        }
        m += 1.0;

        // check if we can abort early with the current confidence interval
        double per_m = 1.0 / m;
        double lo, hi;
        wilson(m, x * per_m, zscore, &lo, &hi);

        if (lo - per_m > coverage)
        {
            ret = 1;
            break;
        }
        if (hi + per_m < coverage)
        {
            ret = 0;
            break;
        }

        if (hi - lo < whi - wlo)
        {   // should occur around i ~ wn
            ret = x * per_m > coverage;
            break;
        }
    }
    if (buf)
        free(buf);
    return ret;
}


void setupBiomeFilter(
    BiomeFilter *bf,
    int mc, uint32_t flags,
    const int *required, int requiredLen,
    const int *excluded, int excludedLen,
    const int *matchany, int matchanyLen)
{
    int i, id;

    memset(bf, 0, sizeof(*bf));
    bf->flags = flags;

    // The matchany set is built from the intersection of each member,
    // individually treated as a required biome. The search can be aborted with
    // a positive result, as soon as any of those biomes is encountered.
    for (i = 0; i < matchanyLen; i++)
    {
        id = matchany[i];
        if (id < 128)
            bf->biomeToPick |= (1ULL << id);
        else
            bf->biomeToPickM |= (1ULL << (id-128));

        BiomeFilter ibf;
        setupBiomeFilter(&ibf, mc, 0, &id, 1, 0, 0, 0, 0);
        if (i == 0)
        {
            bf->tempsToFind  = ibf.tempsToFind;
            bf->otempToFind  = ibf.otempToFind;
            bf->majorToFind  = ibf.majorToFind;
            bf->edgesToFind  = ibf.edgesToFind;
            bf->raresToFind  = ibf.raresToFind;
            bf->raresToFindM = ibf.raresToFindM;
            bf->shoreToFind  = ibf.shoreToFind;
            bf->shoreToFindM = ibf.shoreToFindM;
            bf->riverToFind  = ibf.riverToFind;
            bf->riverToFindM = ibf.riverToFindM;
            bf->oceanToFind  = ibf.oceanToFind;
        }
        else
        {
            bf->tempsToFind  &= ibf.tempsToFind;
            bf->otempToFind  &= ibf.otempToFind;
            bf->majorToFind  &= ibf.majorToFind;
            bf->edgesToFind  &= ibf.edgesToFind;
            bf->raresToFind  &= ibf.raresToFind;
            bf->raresToFindM &= ibf.raresToFindM;
            bf->shoreToFind  &= ibf.shoreToFind;
            bf->shoreToFindM &= ibf.shoreToFindM;
            bf->riverToFind  &= ibf.riverToFind;
            bf->riverToFindM &= ibf.riverToFindM;
            bf->oceanToFind  &= ibf.oceanToFind;
        }
    }

    // The excluded set is built by checking which of the biomes from each
    // layer have the potential to yield something other than one of the
    // excluded biomes.
    for (i = 0; i < excludedLen; i++)
    {
        id = excluded[i];
        if (id & ~0xbf) // i.e. not in ranges [0,64),[128,192)
        {
            fprintf(stderr, "setupBiomeFilter: biomeID=%d not supported.\n", id);
            exit(-1);
        }
        if (id < 128)
            bf->biomeToExcl |= (1ULL << id);
        else
            bf->biomeToExclM |= (1ULL << (id-128));
    }
    if (excludedLen && mc >= MC_1_0)
    {   // TODO: this does not fully work yet...
        uint64_t b, m;
        int j;
        for (j = Oceanic; j <= Freezing+Special; j++)
        {
            b = m = 0;
            int temp = (j <= Freezing) ? j : ((j - Special) | 0xf00);
            genPotential(&b, &m, L_SPECIAL_1024, mc, flags, temp);
            if ((bf->biomeToExcl & b) || (bf->biomeToExclM & m))
                bf->tempsToExcl |= (1ULL << j);
        }
        for (j = 0; j < 256; j++)
        {
            if (!isOverworld(mc, j))
                continue;
            if (j < 128)
            {
                b = m = 0;
                genPotential(&b, &m, L_BIOME_256, mc, flags, j);
                if ((~bf->biomeToExcl & b) || (~bf->biomeToExclM & m))
                    bf->majorToExcl |= (1ULL << j);
            }
            b = m = 0;
            genPotential(&b, &m, L_BIOME_EDGE_64, mc, flags, j);
            if ((~bf->biomeToExcl & b) || (~bf->biomeToExclM & m))
            {
                if (j < 128)
                    bf->edgesToExcl |= (1ULL << j);
                else // bamboo_jungle are mapped onto & 0x3F
                    bf->edgesToExcl |= (1ULL << (j-128));
            }
            b = m = 0;
            genPotential(&b, &m, L_SUNFLOWER_64, mc, flags, j);
            if ((~bf->biomeToExcl & b) || (~bf->biomeToExclM & m))
            {
                if (j < 128)
                    bf->raresToExcl |= (1ULL << j);
                else
                    bf->raresToExclM |= (1ULL << (j-128));
            }
            b = m = 0;
            genPotential(&b, &m, L_SHORE_16, mc, flags, j);
            if ((~bf->biomeToExcl & b) || (~bf->biomeToExclM & m))
            {
                if (j < 128)
                    bf->shoreToExcl |= (1ULL << j);
                else
                    bf->shoreToExclM |= (1ULL << (j-128));
            }
            b = m = 0;
            genPotential(&b, &m, L_RIVER_MIX_4, mc, flags, j);
            if ((~bf->biomeToExcl & b) || (~bf->biomeToExclM & m))
            {
                if (j < 128)
                    bf->riverToExcl |= (1ULL << j);
                else
                    bf->riverToExclM |= (1ULL << (j-128));
            }
        }
    }

    // The required set is built from the biomes that should be present at each
    // of the layers. The search can be aborted with a negative result as soon
    // as a biome is missing at the corresponding layer.
    for (i = 0; i < requiredLen; i++)
    {
        id = required[i];
        if (id & ~0xbf) // i.e. not in ranges [0,64),[128,192)
        {
            fprintf(stderr, "setupBiomeFilter: biomeID=%d not supported.\n", id);
            exit(-1);
        }

        switch (id)
        {
        case mushroom_fields:
            // mushroom shores can generate with hills and at rivers
            bf->raresToFind |= (1ULL << mushroom_fields);
            // fall through
        case mushroom_field_shore:
            bf->tempsToFind |= (1ULL << Oceanic);
            bf->majorToFind |= (1ULL << mushroom_fields);
            bf->riverToFind |= (1ULL << id);
            break;

        case badlands_plateau:
        case wooded_badlands_plateau:
        case badlands:
        case eroded_badlands:
        case modified_badlands_plateau:
        case modified_wooded_badlands_plateau:
            bf->tempsToFind |= (1ULL << (Warm+Special));
            if (id == badlands_plateau || id == modified_badlands_plateau)
                bf->majorToFind |= (1ULL << badlands_plateau);
            if (id == wooded_badlands_plateau || id == modified_wooded_badlands_plateau)
                bf->majorToFind |= (1ULL << wooded_badlands_plateau);
            if (id < 128) {
                bf->raresToFind |= (1ULL << id);
                bf->riverToFind |= (1ULL << id);
            } else {
                bf->raresToFindM |= (1ULL << (id-128));
                bf->riverToFindM |= (1ULL << (id-128));
            }
            break;

        case jungle:
        case jungle_edge:
        case jungle_hills:
        case modified_jungle:
        case modified_jungle_edge:
        case bamboo_jungle:
        case bamboo_jungle_hills:
            bf->tempsToFind |= (1ULL << (Lush+Special));
            bf->majorToFind |= (1ULL << jungle);
            if (id == bamboo_jungle || id == bamboo_jungle_hills) {
                // bamboo%64 are End biomes, so we can reuse the edgesToFind
                bf->edgesToFind |= (1ULL << (bamboo_jungle & 0x3f));
                bf->raresToFindM |= (1ULL << (id-128));
                bf->riverToFindM |= (1ULL << (id-128));
            } else if (id == jungle_edge) {
                // un-modified jungle_edge can be created at shore layer
                bf->riverToFind |= (1ULL << jungle_edge);
            } else {
                if (id == modified_jungle_edge)
                    bf->edgesToFind |= (1ULL << jungle_edge);
                else
                    bf->edgesToFind |= (1ULL << jungle);
                if (id < 128) {
                    bf->raresToFind |= (1ULL << id);
                    bf->riverToFind |= (1ULL << id);
                } else {
                    bf->raresToFindM |= (1ULL << (id-128));
                    bf->riverToFindM |= (1ULL << (id-128));
                }
            }
            break;

        case giant_tree_taiga:
        case giant_tree_taiga_hills:
        case giant_spruce_taiga:
        case giant_spruce_taiga_hills:
            bf->tempsToFind |= (1ULL << (Cold+Special));
            bf->majorToFind |= (1ULL << giant_tree_taiga);
            bf->edgesToFind |= (1ULL << giant_tree_taiga);
            if (id < 128) {
                bf->raresToFind |= (1ULL << id);
                bf->riverToFind |= (1ULL << id);
            } else {
                bf->raresToFindM |= (1ULL << (id-128));
                bf->riverToFindM |= (1ULL << (id-128));
            }
            break;

        case savanna:
        case savanna_plateau:
        case shattered_savanna:
        case shattered_savanna_plateau:
        case desert_hills:
        case desert_lakes:
            bf->tempsToFind |= (1ULL << Warm);
            if (id == desert_hills || id == desert_lakes) {
                bf->majorToFind |= (1ULL << desert);
                bf->edgesToFind |= (1ULL << desert);
            } else {
                bf->majorToFind |= (1ULL << savanna);
                bf->edgesToFind |= (1ULL << savanna);
            }
            if (id < 128) {
                bf->raresToFind |= (1ULL << id);
                bf->riverToFind |= (1ULL << id);
            } else {
                bf->raresToFindM |= (1ULL << (id-128));
                bf->riverToFindM |= (1ULL << (id-128));
            }
            break;

        case dark_forest:
        case dark_forest_hills:
        case birch_forest:
        case birch_forest_hills:
        case tall_birch_forest:
        case tall_birch_hills:
        case swamp:
        case swamp_hills:
            bf->tempsToFind |= (1ULL << Lush);
            if (id == dark_forest || id == dark_forest_hills) {
                bf->majorToFind |= (1ULL << dark_forest);
                bf->edgesToFind |= (1ULL << dark_forest);
            }
            else if (id == birch_forest || id == birch_forest_hills ||
                     id == tall_birch_forest || id == tall_birch_hills) {
                bf->majorToFind |= (1ULL << birch_forest);
                bf->edgesToFind |= (1ULL << birch_forest);
            }
            else if (id == swamp || id == swamp_hills) {
                bf->majorToFind |= (1ULL << swamp);
                bf->edgesToFind |= (1ULL << swamp);
            }
            if (id < 128) {
                bf->raresToFind |= (1ULL << id);
                bf->riverToFind |= (1ULL << id);
            } else {
                bf->raresToFindM |= (1ULL << (id-128));
                bf->riverToFindM |= (1ULL << (id-128));
            }
            break;

        case snowy_taiga:
        case snowy_taiga_hills:
        case snowy_taiga_mountains:
        case snowy_plains:
        case snowy_mountains:
        case ice_spikes:
        case frozen_river:
            bf->tempsToFind |= (1ULL << Freezing);
            if (id == snowy_taiga || id == snowy_taiga_hills ||
                id == snowy_taiga_mountains)
                bf->edgesToFind |= (1ULL << snowy_taiga);
            else
                bf->edgesToFind |= (1ULL << snowy_plains);
            if (id == frozen_river) {
                bf->raresToFind |= (1ULL << snowy_plains);
                bf->riverToFind |= (1ULL << id);
            } else if (id < 128) {
                bf->raresToFind |= (1ULL << id);
                bf->riverToFind |= (1ULL << id);
            } else {
                bf->raresToFindM |= (1ULL << (id-128));
                bf->riverToFindM |= (1ULL << (id-128));
            }
            break;

        case sunflower_plains:
            bf->raresToFindM |= (1ULL << (id-128));
            bf->riverToFindM |= (1ULL << (id-128));
            break;

        case snowy_beach:
            bf->tempsToFind |= (1ULL << Freezing);
            // fall through
        case beach:
        case stone_shore:
            bf->riverToFind |= (1ULL << id);
            break;

        case mountains:
            bf->majorToFind |= (1ULL << mountains);
            // fall through
        case wooded_mountains:
            bf->raresToFind |= (1ULL << id);
            bf->riverToFind |= (1ULL << id);
            break;
        case gravelly_mountains:
            bf->majorToFind |= (1ULL << mountains);
            // fall through
        case modified_gravelly_mountains:
            bf->raresToFindM |= (1ULL << (id-128));
            bf->riverToFindM |= (1ULL << (id-128));
            break;

        case taiga:
        case taiga_hills:
            bf->edgesToFind |= (1ULL << taiga);
            bf->raresToFind |= (1ULL << id);
            bf->riverToFind |= (1ULL << id);
            break;
        case taiga_mountains:
            bf->edgesToFind |= (1ULL << taiga);
            bf->raresToFindM |= (1ULL << (id-128));
            bf->riverToFindM |= (1ULL << (id-128));
            break;

        case plains:
        case forest:
        case wooded_hills:
            bf->raresToFind |= (1ULL << id);
            bf->riverToFind |= (1ULL << id);
            break;
        case flower_forest:
            bf->raresToFindM |= (1ULL << (id-128));
            bf->riverToFindM |= (1ULL << (id-128));
            break;

        case desert: // can generate at shore layer
            bf->riverToFind |= (1ULL << id);
            break;

        default:
            if (isOceanic(id)) {
                bf->tempsToFind |= (1ULL << Oceanic);
                bf->oceanToFind |= (1ULL << id);
                if (isShallowOcean(id)) {
                    if (id != lukewarm_ocean && id != cold_ocean)
                        bf->otempToFind |= (1ULL << id);
                } else {
                    if (id == deep_warm_ocean)
                        bf->otempToFind |= (1ULL << warm_ocean);
                    else if (id == deep_ocean)
                        bf->otempToFind |= (1ULL << ocean);
                    else if (id == deep_frozen_ocean)
                        bf->otempToFind |= (1ULL << frozen_ocean);
                    if (!(flags & FORCE_OCEAN_VARIANTS)) {
                        bf->raresToFind |= (1ULL << deep_ocean);
                        bf->riverToFind |= (1ULL << deep_ocean);
                    }
                }
            } else {
                if (id < 64)
                    bf->riverToFind |= (1ULL << id);
                else
                    bf->riverToFindM |= (1ULL << (id-128));
            }
            break;
        }
    }

    bf->biomeToFind = bf->riverToFind;
    bf->biomeToFind &= ~((1ULL << ocean) | (1ULL << deep_ocean));
    bf->biomeToFind |= bf->oceanToFind;
    bf->biomeToFindM = bf->riverToFindM;

    bf->shoreToFind = bf->riverToFind;
    bf->shoreToFind &= ~((1ULL << river) | (1ULL << frozen_river));
    bf->shoreToFindM = bf->riverToFindM;

    bf->specialCnt = 0;
    bf->specialCnt += !!(bf->tempsToFind & (1ULL << (Warm+Special)));
    bf->specialCnt += !!(bf->tempsToFind & (1ULL << (Lush+Special)));
    bf->specialCnt += !!(bf->tempsToFind & (1ULL << (Cold+Special)));
}


typedef struct {
    Generator *g;
    int *ids;
    Range r;
    uint32_t flags;
    uint64_t b, m;
    uint64_t breq, mreq;
    uint64_t bexc, mexc;
    uint64_t bany, many;
    volatile char *stop;
} gdt_info_t;

static int f_graddesc_test(void *data, int x, int z, double p)
{
    (void) p;
    gdt_info_t *info = (gdt_info_t *) data;
    if (info->stop && *info->stop)
        return 1;
    int idx = (z - info->r.z) * info->r.sx + (x - info->r.x);
    if (info->ids[idx] != -1)
        return 0;
    int id = getBiomeAt(info->g, info->r.scale, x, info->r.y, z);
    info->ids[idx] = id;
    if (id < 128) info->b |= (1ULL << id);
    else info->m |= (1ULL << (id-128));

    // check if we know enough to stop
    int match_exc = (info->bexc|info->mexc) == 0;
    int match_any = (info->bany|info->many) == 0;
    int match_req = (info->breq|info->mreq) == 0;
    if (!match_exc && ((info->b & info->bexc) || (info->m & info->mexc)))
        return 1; // encountered an excluded biome -> stop
    match_any |= ((info->b & info->bany) || (info->m & info->many));
    match_req |= ((info->b & info->breq) == info->breq &&
                  (info->m & info->mreq) == info->mreq);
    if (match_exc && match_any && match_req)
        return 1; // all conditions met -> stop
    return 0;
}

int checkForBiomes(
        Generator         * g,
        int               * cache,
        Range               r,
        int                 dim,
        uint64_t            seed,
        const BiomeFilter * filter,
        volatile char     * stop
        )
{
    if (stop && *stop)
        return 0;
    int i, j, k, ret;
    if (r.sy == 0)
        r.sy = 1;

    if (g->mc <= MC_B1_7)
    {   // TODO: optimize
        int *ids;
        if (cache)
            ids = cache;
        else
            ids = allocCache(g, r);

        if (g->dim != dim || g->seed != seed)
            applySeed(g, dim, seed);

        genBiomes(g, ids, r);
        uint64_t b = 0;
        for (i = 0; i < r.sx*r.sz; i++)
            b |= (1ULL << ids[i]);

        if (ids != cache)
            free(ids);

        int match_exc = (filter->biomeToExcl) == 0;
        int match_any = (filter->biomeToPick) == 0;
        int match_req = (filter->biomeToFind) == 0;
        match_exc |= (b & filter->biomeToExcl) == 0;
        match_any |= (b & filter->biomeToPick) != 0;
        match_req |= (b & filter->biomeToFind) == filter->biomeToFind;
        return match_exc && match_any && match_req;
    }
    if (g->mc < MC_1_18 && dim == DIM_OVERWORLD)
    {
        Layer *entry = (Layer*) getLayerForScale(g, r.scale);
        ret = checkForBiomesAtLayer(&g->ls, entry, cache, seed,
            r.x, r.z, r.sx, r.sz, filter);
        if (ret == 0 && r.sy > 1 && cache)
        {
            for (i = 0; i < r.sy; i++)
            {   // overworld has no vertical noise: expanding 2D into 3D
                for (j = 0; j < r.sx*r.sz; j++)
                    cache[i*r.sx*r.sz + j] = cache[j];
            }
        }
        return ret;
    }

    int *ids, id;
    if (cache)
        ids = cache;
    else
        ids = allocCache(g, r);

    if (g->dim != dim || g->seed != seed)
    {
        applySeed(g, dim, seed);
    }

    gdt_info_t info[1];
    info->g = g;
    info->ids = ids;
    info->r = r;
    info->flags = filter->flags;
    info->b = info->m = 0;
    info->breq = filter->biomeToFind;
    info->mreq = filter->biomeToFindM;
    info->bexc = filter->biomeToExcl;
    info->mexc = filter->biomeToExclM;
    info->bany = filter->biomeToPick;
    info->many = filter->biomeToPickM;
    info->stop = stop;

    ret = 0;
    memset(ids, -1, r.sx * r.sz * sizeof(int));

    int n = r.sx*r.sy*r.sz;
    int trials = n;
    struct touple { int i, x, y, z; } *buf = NULL;

    if (r.scale == 4 && r.sx * r.sz > 64 && dim == DIM_OVERWORLD)
    {
        // Do a gradient descent to find the min/max of some climate parameters
        // and check the biomes along the way. This has a much better chance
        // of finding the biomes with exteme climates early.
        double tmin, tmax;
        int err = 0;
        do
        {
            err = getParaRange(&g->bn.climate[NP_TEMPERATURE], &tmin, &tmax,
                r.x, r.z, r.sx, r.sz, info, f_graddesc_test);
            if (err) break;
            err = getParaRange(&g->bn.climate[NP_HUMIDITY], &tmin, &tmax,
                r.x, r.z, r.sx, r.sz, info, f_graddesc_test);
            if (err) break;
            err = getParaRange(&g->bn.climate[NP_EROSION], &tmin, &tmax,
                r.x, r.z, r.sx, r.sz, info, f_graddesc_test);
            if (err) break;
            //err = getParaRange(&g->bn.climate[NP_CONTINENTALNESS], &tmin, &tmax,
            //    r.x, r.z, r.sx, r.sz, info, f_graddesc_test);
            //if (err) break;
            //err = getParaRange(&g->bn.climate[NP_WEIRDNESS], &tmin, &tmax,
            //    r.x, r.z, r.sx, r.sz, info, f_graddesc_test);
            //if (err) break;
        }
        while (0);
        if (err || (stop && *stop) || (filter->flags & BF_APPROX))
            goto L_end;
    }

    // We'll shuffle the coordinates so we'll generate the biomes in a
    // stochasitc mannor.
    buf = (struct touple*) malloc(n * sizeof(*buf));

    id = 0;
    for (k = 0; k < r.sy; k++)
    {
        for (j = 0; j < r.sz; j++)
        {
            for (i = 0; i < r.sx; i++)
            {
                buf[id].i = id;
                buf[id].x = i;
                buf[id].y = k;
                buf[id].z = j;
                id++;
            }
        }
    }

    // Determine a number of trials that gives a decent chance to sample all
    // the biomes that are present, assuming a completely random and
    // independent biome distribution. (This is actually not at all the case.)
    if (filter->flags & BF_APPROX)
    {
        int t = 400 + (int) sqrt(n);
        if (trials > t)
            trials = t;
    }

    for (i = 0; i < trials; i++)
    {
        struct touple t;
        j = n - i;
        k = rand() % j;
        t = buf[k];
        if (k != j-1)
        {
            buf[k] = buf[j-1];
            buf[j-1] = t;
        }

        if (stop && *stop)
            break;
        if (t.y == 0 && info->ids[t.i] != -1)
            continue;
        id = getBiomeAt(g, r.scale, r.x+t.x, r.y+t.y, r.z+t.z);
        info->ids[t.i] = id;
        if (id < 128) info->b |= (1ULL << id);
        else info->m |= (1ULL << (id-128));

        // check if we know enough to yield a result
        int match_exc = (info->bexc|info->mexc) == 0;
        int match_any = (info->bany|info->many) == 0;
        int match_req = (info->breq|info->mreq) == 0;
        if (!match_exc && ((info->b & info->bexc) || (info->m & info->mexc)))
            break; // encountered an excluded biome
        match_any |= ((info->b & info->bany) || (info->m & info->many));
        match_req |= ((info->b & info->breq) == info->breq &&
                      (info->m & info->mreq) == info->mreq);
        if (match_exc && match_any && match_req)
            break; // all conditions met
    }

L_end:
    if (stop && *stop)
    {
        ret = 0;
    }
    else
    {   // given the biome set {info.b, info.m} determine if we have a match
        int match_exc = (info->bexc|info->mexc) == 0;
        int match_any = (info->bany|info->many) == 0;
        int match_req = (info->breq|info->mreq) == 0;
        match_exc |= ((info->b & info->bexc) || (info->m & info->mexc)) == 0;
        match_any |= ((info->b & info->bany) || (info->m & info->many));
        match_req |= ((info->b & info->breq) == info->breq &&
                      (info->m & info->mreq) == info->mreq);
        ret = (match_exc && match_any && match_req);
    }

    if (buf)
        free(buf);
    if (ids != cache)
        free(ids);
    return ret;
}


STRUCT(filter_data_t)
{
    const BiomeFilter *bf;
    int (*map)(const Layer *, int *, int, int, int, int);
};

enum { M_STOP=1, M_DONE=2 };

static int mapFilterSpecial(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    int i, j;
    uint64_t temps;

    /// pre-gen checks
    int specialcnt = f->bf->specialCnt;
    if (specialcnt > 0)
    {
        uint64_t ss = l->startSeed;
        uint64_t cs;

        for (j = 0; j < h; j++)
        {
            for (i = 0; i < w; i++)
            {
                cs = getChunkSeed(ss, x+i, z+j);
                if (mcFirstIsZero(cs, 13))
                    specialcnt--;
            }
        }
        if (specialcnt > 0)
            return M_STOP;
    }

    int err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    temps = 0;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int id = out[i + w*j];
            int isspecial = id & 0xf00;
            id &= ~0xf00;
            if (isspecial && id != Freezing)
               temps |= (1ULL << (id+Special));
            else
               temps |= (1ULL << id);
        }
    }
    if ((temps & f->bf->tempsToFind) ^ f->bf->tempsToFind)
        return M_STOP;
    return 0;
}

static int mapFilterMushroom(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    int i, j;
    int err;

    if (w*h < 100 && (f->bf->majorToFind & (1ULL << mushroom_fields)))
    {
        uint64_t ss = l->startSeed;
        uint64_t cs;

        for (j = 0; j < h; j++)
        {
            for (i = 0; i < w; i++)
            {
                cs = getChunkSeed(ss, i+x, j+z);
                if (mcFirstIsZero(cs, 100))
                    goto L_generate;
            }
        }
        return M_STOP;
    }

L_generate:
    err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    if (f->bf->majorToFind & (1ULL << mushroom_fields))
    {
        for (i = 0; i < w*h; i++)
            if (out[i] == mushroom_fields)
                return 0;
        return M_STOP;
    }
    return 0;
}

static int mapFilterBiome(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    int i, j;
    uint64_t b;

    int err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0;
    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int id = out[i + w*j];
            b |= (1ULL << id);
        }
    }
    if ((b & f->bf->majorToFind) ^ f->bf->majorToFind)
        return M_STOP;
    return 0;
}

static int mapFilterOceanTemp(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    int i, j;
    uint64_t b;

    int err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0;
    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int id = out[i + w*j];
            b |= (1ULL << id);
        }
    }
    if ((b & f->bf->otempToFind) ^ f->bf->otempToFind)
        return M_STOP;
    return 0;
}

static int mapFilterBiomeEdge(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    uint64_t b;
    int i;
    int err;

    err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0;
    for (i = 0; i < w*h; i++)
        b |= (1ULL << (out[i] & 0x3f));
    if ((b & f->bf->edgesToFind) ^ f->bf->edgesToFind)
        return M_STOP;
    return 0;
}

static int mapFilterRareBiome(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    uint64_t b, bm;
    int i;
    int err;

    err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0; bm = 0;
    for (i = 0; i < w*h; i++)
    {
        int id = out[i];
        if (id < 128) b |= (1ULL << id);
        else bm |= (1ULL << (id-128));
    }
    if ((b & f->bf->raresToFind) ^ f->bf->raresToFind)
        return M_STOP;
    if ((bm & f->bf->raresToFindM) ^ f->bf->raresToFindM)
        return M_STOP;
    return 0;
}

static int mapFilterShore(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    uint64_t b, bm;
    int i;

    int err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0; bm = 0;
    for (i = 0; i < w*h; i++)
    {
        int id = out[i];
        if (id < 128) b |= (1ULL << id);
        else bm |= (1ULL << (id-128));
    }
    if ((b & f->bf->shoreToFind) ^ f->bf->shoreToFind)
        return M_STOP;
    if ((bm & f->bf->shoreToFindM) ^ f->bf->shoreToFindM)
        return M_STOP;
    return 0;
}

static int mapFilterRiverMix(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    uint64_t b, bm;
    int i;

    int err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0; bm = 0;
    for (i = 0; i < w*h; i++)
    {
        int id = out[i];
        if (id < 128) b |= (1ULL << id);
        else bm |= (1ULL << (id-128));
    }
    if ((b & f->bf->riverToFind) ^ f->bf->riverToFind)
        return M_STOP;
    if ((bm & f->bf->riverToFindM) ^ f->bf->riverToFindM)
        return M_STOP;
    return 0;
}

static int mapFilterOceanMix(const Layer * l, int * out, int x, int z, int w, int h)
{
    const filter_data_t *f = (const filter_data_t*) l->data;
    uint64_t b;
    int i;
    int err;

    if (f->bf->riverToFind)
    {
        err = l->p->getMap(l->p, out, x, z, w, h); // RiverMix
        if (err)
            return err;
    }

    err = f->map(l, out, x, z, w, h);
    if unlikely(err != 0)
        return err;

    b = 0;
    for (i = 0; i < w*h; i++)
    {
        int id = out[i];
        if (id < 128) b |= (1ULL << id);
    }

    if ((b & f->bf->oceanToFind) ^ f->bf->oceanToFind)
        return M_STOP;
    return 0;
}

static
void swapMap(filter_data_t *fd, const BiomeFilter *bf, Layer *l,
        int (*map)(const Layer *, int *, int, int, int, int))
{
    fd->bf = bf;
    fd->map = l->getMap;
    l->data = (void*) fd;
    l->getMap = map;
}

static
void restoreMap(filter_data_t *fd, Layer *l)
{
    l->getMap = fd->map;
    l->data = NULL;
}

static
int testExclusion(Layer *layer, int *cache, int x, int z, const BiomeFilter *bf)
{
    int err = layer->getMap(layer, cache, x, z, 1, 1);
    if (err)
        return 0; // skip, but don't treat error as valid
    int id = cache[0];
    if (id < 128)
        return (bf->biomeToExcl & (1ULL << id)) != 0;
    return (bf->biomeToExclM & (1ULL << (id-128))) != 0;
}

int checkForBiomesAtLayer(
        LayerStack        * g,
        Layer             * entry,
        int               * cache,
        uint64_t            seed,
        int                 x,
        int                 z,
        unsigned int        w,
        unsigned int        h,
        const BiomeFilter * filter
        )
{
    Layer *l;
    int *ids;
    int ret, err;
    int memsiz, mem1x1;

    if (filter->flags & BF_APPROX) // TODO: protoCheck for 1.6-
    {
        l = entry;

        int i, j;
        int bx = x * l->scale;
        int bz = z * l->scale;
        int bw = w * l->scale;
        int bh = h * l->scale;
        int x0, z0, x1, z1;
        uint64_t ss, cs;
        uint64_t potential, required;

        int specialcnt = filter->specialCnt;
        if (specialcnt > 0)
        {
            l = &g->layers[L_SPECIAL_1024];
            x0 = (bx) / l->scale; if (x < 0) x0--;
            z0 = (bz) / l->scale; if (z < 0) z0--;
            x1 = (bx + bw) / l->scale; if (x+(int)w >= 0) x1++;
            z1 = (bz + bh) / l->scale; if (z+(int)h >= 0) z1++;
            ss = getStartSeed(seed, l->layerSalt);

            for (j = z0; j <= z1; j++)
            {
                for (i = x0; i <= x1; i++)
                {
                    cs = getChunkSeed(ss, i, j);
                    if (mcFirstIsZero(cs, 13))
                        specialcnt--;
                }
            }
            if (specialcnt > 0)
                return 0;
        }

        l = &g->layers[L_BIOME_256];
        x0 = bx / l->scale; if (x < 0) x0--;
        z0 = bz / l->scale; if (z < 0) z0--;
        x1 = (bx + bw) / l->scale; if (x+(int)w >= 0) x1++;
        z1 = (bz + bh) / l->scale; if (z+(int)h >= 0) z1++;

        if (filter->majorToFind & (1ULL << mushroom_fields))
        {
            ss = getStartSeed(seed, g->layers[L_MUSHROOM_256].layerSalt);

            for (j = z0; j <= z1; j++)
            {
                for (i = x0; i <= x1; i++)
                {
                    cs = getChunkSeed(ss, i, j);
                    if (mcFirstIsZero(cs, 100))
                        goto L_has_proto_mushroom;
                }
            }
            return 0;
        }
L_has_proto_mushroom:

        potential = 0;
        required = filter->majorToFind & (
                (1ULL << badlands_plateau) | (1ULL << wooded_badlands_plateau) |
                (1ULL << desert) | (1ULL << savanna) | (1ULL << plains) |
                (1ULL << forest) | (1ULL << dark_forest) | (1ULL << mountains) |
                (1ULL << birch_forest) | (1ULL << swamp));

        ss = getStartSeed(seed, l->layerSalt);

        for (j = z0; j <= z1; j++)
        {
            for (i = x0; i <= x1; i++)
            {
                cs = getChunkSeed(ss, i, j);
                int cs6 = mcFirstInt(cs, 6);
                int cs3 = mcFirstInt(cs, 3);
                int cs4 = mcFirstInt(cs, 4);

                if (cs3) potential |= (1ULL << badlands_plateau);
                else potential |= (1ULL << wooded_badlands_plateau);

                switch (cs6)
                {
                case 0: potential |= (1ULL << desert) | (1ULL << forest); break;
                case 1: potential |= (1ULL << desert) | (1ULL << dark_forest); break;
                case 2: potential |= (1ULL << desert) | (1ULL << mountains); break;
                case 3: potential |= (1ULL << savanna) | (1ULL << plains); break;
                case 4: potential |= (1ULL << savanna) | (1ULL << birch_forest); break;
                case 5: potential |= (1ULL << plains) | (1ULL << swamp); break;
                }

                if (cs4 == 3) potential |= (1ULL << snowy_taiga);
                else potential |= (1ULL << snowy_plains);
            }
        }

        if ((potential & required) ^ required)
            return 0;
    }

    l = g->layers;
    if (cache)
    {
        memsiz = 0;
        ids = cache;
    }
    else
    {
        memsiz = getMinLayerCacheSize(entry, w, h);
        ids = (int*) calloc(memsiz, sizeof(int));
    }

    if ((filter->biomeToExcl | filter->biomeToExclM) && w*h > 1)
    {
        err = 0;
        if (memsiz == 0)
            memsiz = getMinLayerCacheSize(entry, w, h);
        mem1x1 = getMinLayerCacheSize(entry, 1, 1);
        if (mem1x1 * 2 < memsiz)
        {
            setLayerSeed(entry, seed);
            err = testExclusion(entry, ids, x+w/2, z+h/2, filter);
        }
        if (mem1x1 * 5 < memsiz)
        {
            if (!err) err = testExclusion(entry, ids, x,     z,     filter);
            if (!err) err = testExclusion(entry, ids, x+w-1, z+h-1, filter);
            if (!err) err = testExclusion(entry, ids, x,     z+h-1, filter);
            if (!err) err = testExclusion(entry, ids, x+w-1, z,     filter);
        }
        if (err)
        {
            if (cache == NULL)
                free(ids);
            return 0;
        }
    }

    filter_data_t fd[9];
    swapMap(fd+0, filter, l+L_OCEAN_MIX_4,    mapFilterOceanMix);
    swapMap(fd+1, filter, l+L_RIVER_MIX_4,    mapFilterRiverMix);
    swapMap(fd+2, filter, l+L_SHORE_16,       mapFilterShore);
    swapMap(fd+3, filter, l+L_SUNFLOWER_64,   mapFilterRareBiome);
    swapMap(fd+4, filter, l+L_BIOME_EDGE_64,  mapFilterBiomeEdge);
    swapMap(fd+5, filter, l+L_OCEAN_TEMP_256, mapFilterOceanTemp);
    swapMap(fd+6, filter, l+L_BIOME_256,      mapFilterBiome);
    swapMap(fd+7, filter, l+L_MUSHROOM_256,   mapFilterMushroom);
    swapMap(fd+8, filter, l+L_SPECIAL_1024,   mapFilterSpecial);

    ret = 0;
    setLayerSeed(entry, seed);
    err = entry->getMap(entry, ids, x, z, w, h);
    if (err == 0)
    {
        uint64_t b = 0, m = 0;
        unsigned int i;
        for (i = 0; i < w*h; i++)
        {
            int id = ids[i];
            if (id < 128) b |= (1ULL << id);
            else m |= (1ULL << (id-128));
        }

        int match_exc = (filter->biomeToExcl|filter->biomeToExclM) == 0;
        int match_any = (filter->biomeToPick|filter->biomeToPickM) == 0;
        int match_req = (filter->biomeToFind|filter->biomeToFindM) == 0;
        match_exc |= ((b & filter->biomeToExcl) || (m & filter->biomeToExclM)) == 0;
        match_any |= ((b & filter->biomeToPick) || (m & filter->biomeToPickM));
        match_req |= ((b & filter->biomeToFind)  == filter->biomeToFind &&
                      (m & filter->biomeToFindM) == filter->biomeToFindM);
        if (match_exc && match_any && match_req)
            ret = 1;
    }
    else if (err == M_STOP)
    {   // biome requirements not met
        ret = 0;
    }
    else if (err == M_DONE)
    {   // exclusion biomes cannot generate
        ret = 2;
    }

    restoreMap(fd+8, l+L_SPECIAL_1024);
    restoreMap(fd+7, l+L_MUSHROOM_256);
    restoreMap(fd+6, l+L_BIOME_256);
    restoreMap(fd+5, l+L_OCEAN_TEMP_256);
    restoreMap(fd+4, l+L_BIOME_EDGE_64);
    restoreMap(fd+3, l+L_SUNFLOWER_64);
    restoreMap(fd+2, l+L_SHORE_16);
    restoreMap(fd+1, l+L_RIVER_MIX_4);
    restoreMap(fd+0, l+L_OCEAN_MIX_4);

    if (cache == NULL)
        free(ids);

    return ret;
}


int checkForTemps(LayerStack *g, uint64_t seed, int x, int z, int w, int h, const int tc[9])
{
    uint64_t ls = getLayerSalt(3); // L_SPECIAL_1024 layer seed
    uint64_t ss = getStartSeed(seed, ls);

    int i, j;
    int scnt = 0;

    if (tc[Special+Warm] > 0) scnt += tc[Special+Warm];
    if (tc[Special+Lush] > 0) scnt += tc[Special+Lush];
    if (tc[Special+Cold] > 0) scnt += tc[Special+Cold];

    if (scnt > 0)
    {
        for (j = 0; j < h; j++)
        {
            for (i = 0; i < w; i++)
            {
                if (mcFirstIsZero(getChunkSeed(ss, x+i, z+j), 13))
                    scnt--;
            }
        }
        if (scnt > 0)
            return 0;
    }

    Layer *l = &g->layers[L_SPECIAL_1024];
    int ccnt[9] = {0};
    int *area = (int*) calloc(getMinLayerCacheSize(l, w, h), sizeof(int));
    int ret = 1;

    setLayerSeed(l, seed);
    genArea(l, area, x, z, w, h);

    for (i = 0; i < w*h; i++)
    {
        int id = area[i];
        int t = id & 0xff;
        if (id != t && t != Freezing)
            t += Special;
        ccnt[t]++;
    }
    for (i = 0; i < 9; i++)
    {
        if (ccnt[i] < tc[i] || (ccnt[i] && tc[i] < 0))
        {
            ret = 0;
            break;
        }
    }

    free(area);
    return ret;
}


struct locate_info_t
{
    Generator *g;
    int *ids;
    Range r;
    int match, tol;
    volatile char *stop;
};

static
int floodFillGen(struct locate_info_t *info, int i, int j, Pos *p)
{
    typedef struct { int i, j, d; } entry_t;
    entry_t *queue = (entry_t*) malloc(info->r.sx*info->r.sz * sizeof(*queue));
    int qn = 1;
    queue->i = i;
    queue->j = j;
    queue->d = 0;
    int64_t sumx = 0;
    int64_t sumz = 0;
    int n = 0;
    while (--qn >= 0)
    {
        if (info->stop && *info->stop)
        {
            free(queue);
            return 0;
        }
        int d = queue[qn].d;
        i = queue[qn].i;
        j = queue[qn].j;
        int k = j * info->r.sx + i;
        int id = info->ids[k];
        if (id == INT_MAX)
            continue;
        info->ids[k] = INT_MAX;
        int x = info->r.x + i;
        int z = info->r.z + j;
        if (info->g->mc >= MC_1_18)
            id = getBiomeAt(info->g, info->r.scale, x, info->r.y, z);
        if (id == info->match)
        {
            sumx += x;
            sumz += z;
            n++;
            d = 0;
        }
        else
        {
            if (++d >= info->tol)
                continue;
        }
        entry_t next[] = { {i,j-1,d}, {i,j+1,d}, {i-1,j,d}, {i+1,j,d} };
        for (k = 0; k < 4; k++)
        {
            i = next[k].i; j = next[k].j;
            if (i < 0 || i >= info->r.sx || j < 0 || j >= info->r.sz)
                continue;
            if (info->ids[j * info->r.sx + i] == INT_MAX)
                continue;
            queue[qn++] = next[k];
        }
    }
    free(queue);
    if (n)
    {
        p->x = (int) round((sumx / (double)n + 0.5) * info->r.scale);
        p->z = (int) round((sumz / (double)n + 0.5) * info->r.scale);
    }
    return n;
}


int getBiomeCenters(Pos *pos, int *siz, int nmax, Generator *g, Range r,
    int match, int minsiz, int tol, volatile char *stop)
{
    if (minsiz <= 0)
        minsiz = 1;
    int i, j, k, n = 0;
    int *ids = (int*) malloc(r.sx*r.sz * sizeof(int));
    memset(ids, -1, r.sx*r.sz * sizeof(int));
    if (tol <= 0)
        tol = 1;
    int step = tol;
    struct locate_info_t info;
    info.g = g;
    info.ids = ids;
    info.r = r;
    info.stop = stop;
    info.match = match;
    info.tol = tol;

    if (g->mc >= MC_1_18)
    {
        const int *lim = getBiomeParaLimits(g->mc, match);

        int para[] = {
            NP_TEMPERATURE,
            NP_HUMIDITY,
            NP_EROSION,
            NP_CONTINENTALNESS,
            NP_WEIRDNESS,
        };
        int npara = sizeof(para) / sizeof(para[0]);
        if (tol == 1)
            step = 1 + floor(sqrt(minsiz) * 0.5);

        for (j = 0; j < r.sz; j += step)
        {
            for (i = 0; i < r.sx; i += step)
            {
                if (stop && *stop)
                    break;
                for (k = 0; k < npara; k++)
                {
                    const int *plim = lim + 2*para[k];
                    if (plim[0] == INT_MIN && plim[1] == INT_MAX)
                        continue;
                    DoublePerlinNoise *dpn = &g->bn.climate[para[k]];
                    double px = (r.x+i) * r.scale / 4.0;
                    double pz = (r.z+j) * r.scale / 4.0;
                    int p = 10000 * sampleDoublePerlin(dpn, px, 0, pz);
                    if (p < plim[0] || p > plim[1])
                    {
                        ids[j*r.sx + i] = -2;
                        break;
                    }
                }
            }
        }
        match = -1; // id entries that are still -1 are our candidates
    }
    else // 1.17-
    {
        int ts = 32 / r.scale;
        if (r.sx + r.sz < 32)
            ts = 8;

        int tx = (int) floor(r.x / (double)ts);
        int tz = (int) floor(r.z / (double)ts);
        int tw = (int) ceil((r.x+r.sx) / (double)ts) - tx;
        int th = (int) ceil((r.z+r.sz) / (double)ts) - tz;
        int ti, tj;

        BiomeFilter bf;
        setupBiomeFilter(&bf, g->mc, 0, &match, 1, 0, 0, 0, 0);
        //applySeed(g, 0, g->seed);

        Range tr = { r.scale, 0, 0, ts, ts, 0, 1 };
        int *cache = allocCache(g, r);

        for (tj = 0; tj < th; tj++)
        {
            for (ti = 0; ti < tw; ti++)
            {
                if (stop && *stop)
                    break;
                tr.x = (tx+ti) * ts;
                tr.z = (tz+tj) * ts;
                if (checkForBiomes(g, cache, tr, DIM_OVERWORLD, g->seed,
                    &bf, stop) != 1)
                {
                    continue;
                }
                for (j = 0; j < ts; j++)
                {
                    int jj = tr.z + j - r.z;
                    if (jj < 0 || jj >= r.sz)
                        continue;
                    for (i = 0; i < ts; i++)
                    {
                        int ii = tr.x + i - r.x;
                        if (ii < 0 || ii >= r.sx)
                            continue;
                        ids[jj*r.sx + ii] = cache[j*tr.sx + i];
                    }
                }
            }
        }
        free(cache);
    }

    applySeed(g, DIM_OVERWORLD, g->seed);
    for (j = 0; j < r.sz; j += step)
    {
        for (i = 0; i < r.sx; i += step)
        {
            if (stop && *stop)
                break;
            if (ids[j*r.sx + i] != match)
                continue;
            Pos center;
            int area = floodFillGen(&info, i, j, &center);
            if (area >= minsiz)
            {
                pos[n] = center;
                if (siz) siz[n] = area;
                if (++n >= nmax)
                    goto L_end;
            }
        }
    }

L_end:
    free(ids);

    return n;
}


int canBiomeGenerate(int layerId, int mc, uint32_t flags, int id)
{
    int dofilter = 0;

    if (mc >= MC_1_13)
    {
        if (layerId == L_OCEAN_TEMP_256)
            return isShallowOcean(id);
        if ((flags & FORCE_OCEAN_VARIANTS) && isOceanic(id))
            return id != deep_warm_ocean;
    }

    if (dofilter || layerId == L_BIOME_256)
    {
        dofilter = 1;
        if (id >= 64)
            return 0;
    }
    if (dofilter || (layerId == L_BAMBOO_256 && mc >= MC_1_14))
    {
        dofilter = 1;
        switch (id)
        {
        case jungle_edge:
        case wooded_mountains:
        case badlands:
            return 0;
        }
    }
    if (dofilter || (layerId == L_BIOME_EDGE_64 && mc >= MC_1_0))
    {
        dofilter = 1;
        if (id >= 64 && id != bamboo_jungle)
            return 0;
        switch (id)
        {
        case snowy_mountains:
        case desert_hills:
        case wooded_hills:
        case taiga_hills:
        case jungle_hills:
        case birch_forest_hills:
        case snowy_taiga_hills:
        case giant_tree_taiga_hills:
        case savanna_plateau:
            return 0;
        }
    }
    if (dofilter || (layerId == L_ZOOM_64 && mc <= MC_1_0))
    {
        dofilter = 1;
        if (id == mushroom_field_shore)
            return 0;
    }
    if (dofilter || layerId == L_HILLS_64)
    {
        dofilter = 1;
        if (id == frozen_ocean)
            return 0;
        // sunflower_plains actually generates at Hills layer as well
    }
    if (dofilter || (layerId == L_ZOOM_16 && mc < MC_1_0))
    {
        dofilter = 1;
        if (id == mountain_edge)
            return 0;
    }
    if (dofilter || (layerId == L_SUNFLOWER_64 && mc >= MC_1_0))
    {
        dofilter = 1;
        switch (id)
        {
        case beach:
        case stone_shore:
        case snowy_beach:
            return 0;
        case mushroom_field_shore:
            if (mc != MC_1_0)
                return 0;
            break;
        }
    }
    if (dofilter || layerId == L_SHORE_16)
    {
        dofilter = 1;
        if (id == river)
            return 0;
    }
    if (dofilter || (layerId == L_SWAMP_RIVER_16 && mc < MC_1_0))
    {
        dofilter = 1;
        if (id == frozen_river)
            return 0;
    }
    if (dofilter || layerId == L_RIVER_MIX_4)
    {
        dofilter = 1;
        if (isDeepOcean(id) && id != deep_ocean)
            return 0;
        if (isShallowOcean(id) && id != ocean)
        {
            if (mc >= MC_1_0 || id != frozen_ocean)
                return 0;
        }
    }
    if (dofilter || (layerId == L_OCEAN_MIX_4 && mc >= MC_1_4))
    {
        dofilter = 1;
    }

    if (!dofilter && layerId != L_VORONOI_1)
    {
        printf("canBiomeGenerate(): unsupported layer (%d) or version (%d)\n",
            layerId, mc);
        return 0;
    }
    return isOverworld(mc, id);
}

void getAvailableBiomes(uint64_t *mL, uint64_t *mM, int layerId, int mc, uint32_t flags)
{
    *mL = *mM = 0;
    int i;
    if (mc <= MC_B1_7 || mc >= MC_1_18)
    {
        for (i = 0; i < 64; i++)
        {
            if (isOverworld(mc, i))
                *mL |= (1ULL << i);
            if (isOverworld(mc, i+128))
                *mM |= (1ULL << i);
        }
    }
    else if (mc >= MC_1_13 && layerId == L_OCEAN_TEMP_256)
    {
        *mL =
            (1ULL << ocean) |
            (1ULL << frozen_ocean) |
            (1ULL << warm_ocean) |
            (1ULL << lukewarm_ocean) |
            (1ULL << cold_ocean);
    }
    else
    {
        for (i = 0; i < 64; i++)
        {
            if (canBiomeGenerate(layerId, mc, i, flags))
                *mL |= (1ULL << i);
            if (canBiomeGenerate(layerId, mc, i+128, flags))
                *mM |= (1ULL << i);
        }
    }
}

struct _gp_args
{
    uint64_t *mL, *mM;
    int mc;
    uint32_t flags;
};

static void _genPotential(struct _gp_args *a, int layer, int id)
{
    int mc = a->mc;
    // filter out bad biomes
    if (layer >= L_BIOME_256 && !canBiomeGenerate(layer, mc, a->flags, id))
        return;

    switch (layer)
    {
    case L_SPECIAL_1024: // biomes added in (L_SPECIAL_1024, L_MUSHROOM_256]
        if (mc < MC_1_0) goto L_bad_layer;
        if (id == Oceanic)
            _genPotential(a, L_MUSHROOM_256, mushroom_fields);
        if ((id & ~0xf00) >= Oceanic && (id & ~0xf00) <= Freezing)
            _genPotential(a, L_MUSHROOM_256, id);
        break;

    case L_MUSHROOM_256: // biomes added in (L_MUSHROOM_256, L_DEEP_OCEAN_256]
        if (mc >= MC_1_0) {
            if (id == Oceanic)
                _genPotential(a, L_DEEP_OCEAN_256, deep_ocean);
            if (id == mushroom_fields)
                _genPotential(a, L_DEEP_OCEAN_256, id);
            if ((id & ~0xf00) >= Oceanic && (id & ~0xf00) <= Freezing)
                _genPotential(a, L_DEEP_OCEAN_256, id);
        } else { // (L_MUSHROOM_256, L_BIOME_256] for 1.6
            if (id == ocean || id == mushroom_fields) {
                _genPotential(a, L_BIOME_256, id);
            } else {
                _genPotential(a, L_BIOME_256, desert);
                _genPotential(a, L_BIOME_256, forest);
                _genPotential(a, L_BIOME_256, mountains);
                _genPotential(a, L_BIOME_256, swamp);
                _genPotential(a, L_BIOME_256, plains);
                _genPotential(a, L_BIOME_256, taiga);
                if (mc >= MC_1_2)
                    _genPotential(a, L_BIOME_256, jungle);
                if (id != plains)
                    _genPotential(a, L_BIOME_256, snowy_plains);
            }
        }
        break;

    case L_DEEP_OCEAN_256: // biomes added in (L_DEEP_OCEAN_256, L_BIOME_256]
        if (mc < MC_1_0) goto L_bad_layer;
        switch (id & ~0xf00)
        {
        case Warm:
            if (id & 0xf00) {
                _genPotential(a, L_BIOME_256, badlands_plateau);
                _genPotential(a, L_BIOME_256, wooded_badlands_plateau);
            } else {
                _genPotential(a, L_BIOME_256, desert);
                _genPotential(a, L_BIOME_256, savanna);
                _genPotential(a, L_BIOME_256, plains);
            }
            break;
        case Lush:
            if (id & 0xf00) {
                _genPotential(a, L_BIOME_256, jungle);
            } else {
                _genPotential(a, L_BIOME_256, forest);
                _genPotential(a, L_BIOME_256, dark_forest);
                _genPotential(a, L_BIOME_256, mountains);
                _genPotential(a, L_BIOME_256, plains);
                _genPotential(a, L_BIOME_256, birch_forest);
                _genPotential(a, L_BIOME_256, swamp);
            }
            break;
        case Cold:
            if (id & 0xf00) {
                _genPotential(a, L_BIOME_256, giant_tree_taiga);
            } else {
                _genPotential(a, L_BIOME_256, forest);
                _genPotential(a, L_BIOME_256, mountains);
                _genPotential(a, L_BIOME_256, taiga);
                _genPotential(a, L_BIOME_256, plains);
            }
            break;
        case Freezing:
            _genPotential(a, L_BIOME_256, snowy_plains);
            _genPotential(a, L_BIOME_256, snowy_taiga);
            break;
        default:
            id &= ~0xf00;
            _genPotential(a, L_BIOME_256, id);
        }
        break;

    case L_BIOME_256: // biomes added in (L_BIOME_256, L_BIOME_EDGE_64]
    case L_BAMBOO_256:
    case L_ZOOM_64:
        if (mc <= MC_1_13 && layer == L_BAMBOO_256) goto L_bad_layer;
        if (mc >= MC_1_0) {
            if (mc >= MC_1_14 && id == jungle)
                _genPotential(a, L_BIOME_EDGE_64, bamboo_jungle);
            if (id == wooded_badlands_plateau || id == badlands_plateau)
                _genPotential(a, L_BIOME_EDGE_64, badlands);
            else if(id == giant_tree_taiga)
                _genPotential(a, L_BIOME_EDGE_64, taiga);
            else if (id == desert)
                _genPotential(a, L_BIOME_EDGE_64, wooded_mountains);
            else if (id == swamp) {
                _genPotential(a, L_BIOME_EDGE_64, jungle_edge);
                _genPotential(a, L_BIOME_EDGE_64, plains);
            }
            _genPotential(a, L_BIOME_EDGE_64, id);
            break;
        }
        // (L_BIOME_256, L_HILLS_64] for 1.6
        // fallthrough

    case L_BIOME_EDGE_64: // biomes added in (L_BIOME_EDGE_64, L_HILLS_64]
        if (mc < MC_1_0 && layer == L_BIOME_EDGE_64) goto L_bad_layer;
        if (!isShallowOcean(id) && getMutated(mc, id) > 0)
             _genPotential(a, L_HILLS_64, getMutated(mc, id));
        switch (id)
        {
        case desert:
            _genPotential(a, L_HILLS_64, desert_hills);
            break;
        case forest:
            _genPotential(a, L_HILLS_64, wooded_hills);
            break;
        case birch_forest:
            _genPotential(a, L_HILLS_64, birch_forest_hills);
            _genPotential(a, L_HILLS_64, getMutated(mc, birch_forest_hills));
            break;
        case dark_forest:
            _genPotential(a, L_HILLS_64, plains);
            _genPotential(a, L_HILLS_64, getMutated(mc, plains));
            break;
        case taiga:
            _genPotential(a, L_HILLS_64, taiga_hills);
            break;
        case giant_tree_taiga:
            _genPotential(a, L_HILLS_64, giant_tree_taiga_hills);
            _genPotential(a, L_HILLS_64, getMutated(mc, giant_tree_taiga_hills));
            break;
        case snowy_taiga:
            _genPotential(a, L_HILLS_64, snowy_taiga_hills);
            break;
        case plains:
            if (mc >= MC_1_0)
                _genPotential(a, L_HILLS_64, wooded_hills);
            _genPotential(a, L_HILLS_64, forest);
            _genPotential(a, L_HILLS_64, getMutated(mc, forest));
            break;
        case snowy_plains:
            _genPotential(a, L_HILLS_64, snowy_mountains);
            break;
        case jungle:
            _genPotential(a, L_HILLS_64, jungle_hills);
            break;
        case bamboo_jungle:
            _genPotential(a, L_HILLS_64, bamboo_jungle_hills);
            break;
        case ocean:
            if (mc >= MC_1_0)
                _genPotential(a, L_HILLS_64, deep_ocean);
            break;
        case mountains:
            if (mc >= MC_1_0) {
                _genPotential(a, L_HILLS_64, wooded_mountains);
                _genPotential(a, L_HILLS_64, getMutated(mc, wooded_mountains));
            }
            break;
        case savanna:
            _genPotential(a, L_HILLS_64, savanna_plateau);
            _genPotential(a, L_HILLS_64, getMutated(mc, savanna_plateau));
            break;
        default:
            if (areSimilar(mc, id, wooded_badlands_plateau))
            {
                _genPotential(a, L_HILLS_64, badlands);
                _genPotential(a, L_HILLS_64, getMutated(mc, badlands));
            }
            else if (isDeepOcean(id))
            {
                _genPotential(a, L_HILLS_64, plains);
                _genPotential(a, L_HILLS_64, forest);
                _genPotential(a, L_HILLS_64, getMutated(mc, plains));
                _genPotential(a, L_HILLS_64, getMutated(mc, forest));
            }
        }
        _genPotential(a, L_HILLS_64, id);
        break;

    case L_HILLS_64: // biomes added in (L_HILLS_64, L_RARE_BIOME_64]
        if (mc < MC_1_0) { // (L_HILLS_64, L_SHORE_16] for 1.6
            if (id == mushroom_fields)
                _genPotential(a, L_SHORE_16, mushroom_field_shore);
            else if (id == mountains)
                _genPotential(a, L_SHORE_16, mountain_edge);
            else if (id != ocean && id != river && id != swamp)
                _genPotential(a, L_SHORE_16, beach);
            _genPotential(a, L_SHORE_16, id);
        } else {
            if (id == plains)
                _genPotential(a, L_SUNFLOWER_64, sunflower_plains);
            _genPotential(a, L_SUNFLOWER_64, id);
        }
        break;

    case L_SUNFLOWER_64: // biomes added in (L_SUNFLOWER_64, L_SHORE_16] 1.7+
        if (mc < MC_1_0) goto L_bad_layer;
        // fallthrough
    case L_ZOOM_16:
        if (mc <= MC_1_0 && layer == L_ZOOM_16) {
            _genPotential(a, L_SHORE_16, id);
            break;
        }
        if (id == mushroom_fields)
            _genPotential(a, L_SHORE_16, mushroom_field_shore);
        else if (getCategory(mc, id) == jungle) {
            _genPotential(a, L_SHORE_16, beach);
            _genPotential(a, L_SHORE_16, jungle_edge);
        }
        else if (id == mountains || id == wooded_mountains || id == mountain_edge)
            _genPotential(a, L_SHORE_16, stone_shore);
        else if (isSnowy(id))
            _genPotential(a, L_SHORE_16, snowy_beach);
        else if (id == badlands || id == wooded_badlands_plateau)
            _genPotential(a, L_SHORE_16, desert);
        else if (id != ocean && id != deep_ocean && id != river && id != swamp)
            _genPotential(a, L_SHORE_16, beach);
        _genPotential(a, L_SHORE_16, id);
        break;

    case L_SHORE_16: // biomes added in (L_SHORE_16, L_RIVER_MIX_4]
    case L_SWAMP_RIVER_16:
    case L_ZOOM_4:
        if (id == snowy_plains)
            _genPotential(a, L_RIVER_MIX_4, frozen_river);
        else if (id == mushroom_fields || id == mushroom_field_shore)
            _genPotential(a, L_RIVER_MIX_4, mushroom_field_shore);
        else if (id != ocean && (mc < MC_1_0 || !isOceanic(id)))
            _genPotential(a, L_RIVER_MIX_4, river);
        _genPotential(a, L_RIVER_MIX_4, id);
        break;

    case L_RIVER_MIX_4: // biomes added in (L_RIVER_MIX_4, L_VORONOI_1]
        if (mc >= MC_1_13 && isOceanic(id)) {
            if (id == ocean) {
                _genPotential(a, L_VORONOI_1, ocean);
                _genPotential(a, L_VORONOI_1, warm_ocean);
                _genPotential(a, L_VORONOI_1, lukewarm_ocean);
                _genPotential(a, L_VORONOI_1, cold_ocean);
                _genPotential(a, L_VORONOI_1, frozen_ocean);
            } else if (id == deep_ocean) {
                _genPotential(a, L_VORONOI_1, deep_ocean);
                _genPotential(a, L_VORONOI_1, deep_lukewarm_ocean);
                _genPotential(a, L_VORONOI_1, deep_cold_ocean);
                _genPotential(a, L_VORONOI_1, deep_frozen_ocean);
            }
            else break;
        }
        _genPotential(a, L_VORONOI_1, id);
        break;

    case L_OCEAN_MIX_4:
        if (mc <= MC_1_2) goto L_bad_layer;
        // fallthrough

    case L_VORONOI_1:
        if (id < 128)   *a->mL |= 1ULL << id;
        else            *a->mM |= 1ULL << (id-128);
        break;

    default:
        printf("genPotential() not implemented for layer %d\n", layer);
    }
    if (0)
    {
    L_bad_layer:
        printf("genPotential() bad layer %d for version\n", layer);
    }
}

void genPotential(uint64_t *mL, uint64_t *mM, int layerId, int mc, uint32_t flags, int id)
{
    struct _gp_args args = { mL, mM, mc, flags };
    _genPotential(&args, layerId, id);
}


double getParaDescent(const DoublePerlinNoise *para, double factor,
    int x, int z, int w, int h, int i0, int j0, int maxrad,
    int maxiter, double alpha, void *data, int (*func)(void*,int,int,double))
{
    /// Do a gradient descent on a grid...
    /// To start with, we will just consider a step size of 1 in one axis:
    ///   Try going in positive x: if gradient is upwards go to negative x
    ///   then do the same with z - if all 4 directions go upwards then we have
    ///   found a minimum, otherwise repeat.
    /// We can remember and try the direction from the previous cycle first to
    /// reduce the number of wrong guesses.
    ///
    /// We can also use a larger step size than 1, as long as we believe that
    /// the minimum is not in between. To determine if this is viable, we check
    /// the step size of 1 first, and then jump if the gradient appears large
    /// enough in that direction.
    ///
    ///TODO:
    /// The perlin noise can be sampled continuously, so more established
    /// minima algorithms can also be considered...

    int dirx = 0, dirz = 0, dira;
    int k, i, j;
    double v, vd, va;
    v = factor * sampleDoublePerlin(para, x+i0, 0, z+j0);
    if (func)
    {
        if (func(data, x+i0, z+j0, factor < 0 ? -v : v))
            return nan("");
    }

    i = i0; j = j0;
    for (k = 0; k < maxiter; k++)
    {
        if (dirx == 0) dirx = +1;
        if (i+dirx >= 0 && i+dirx < w)
            vd = factor * sampleDoublePerlin(para, x+i+dirx, 0, z+j);
        else vd = v;
        if (vd >= v)
        {
            dirx *= -1;
            if (i+dirx >= 0 && i+dirx < w)
                vd = factor * sampleDoublePerlin(para, x+i+dirx, 0, z+j);
            else vd = v;
            if (vd >= v)
                dirx = 0;
        }
        if (dirx)
        {
            dira = (int)(dirx * alpha * (v - vd));
            if (abs(dira) > 2 && i+dira >= 0 && i+dira < w)
            {   // try jumping by more than 1
                va = factor * sampleDoublePerlin(para, x+i+dira, 0, z+j);
                if (va < vd)
                {
                    i += dira;
                    v = va;
                    goto L_x_end;
                }
            }
            v = vd;
            i += dirx;
        L_x_end:
            if (func)
            {
                if (func(data, x+i, z+j, factor < 0 ? -v : v))
                    return nan("");
            }
        }

        if (dirz == 0) dirz = +1;
        if (j+dirz >= 0 && j+dirz < h)
            vd = factor * sampleDoublePerlin(para, x+i, 0, z+j+dirz);
        else vd = v;
        if (vd >= v)
        {
            dirz *= -1;
            if (j+dirz >= 0 && j+dirz < h)
                vd = factor * sampleDoublePerlin(para, x+i, 0, z+j+dirz);
            else vd = v;
            if (vd >= v)
                dirz = 0;
        }
        if (dirz)
        {
            dira = (int)(dirz * alpha * (v - vd));
            if (abs(dira) > 2 && j+dira >= 0 && j+dira < h)
            {   // try jumping by more than 1
                va = factor * sampleDoublePerlin(para, x+i, 0, z+j+dira);
                if (va < vd)
                {
                    j += dira;
                    v = va;
                    goto L_z_end;
                }
            }
            j += dirz;
            v = vd;
        L_z_end:
            if (func)
            {
                if (func(data, x+i, z+j, factor < 0 ? -v : v))
                    return nan("");
            }
        }
        if (dirx == 0 && dirz == 0)
        {   // this is very likely a fix point
            // but there could be a minimum along a diagonal path in rare cases
            int c;
            for (c = 0; c < 4; c++)
            {
                dirx = (c & 1) ? -1 : +1;
                dirz = (c & 2) ? -1 : +1;
                if (i+dirx < 0 || i+dirx >= w || j+dirz < 0 || j+dirz >= h)
                    continue;
                vd = factor * sampleDoublePerlin(para, x+i+dirx, 0, z+j+dirz);
                if (vd < v)
                {
                    v = vd;
                    i += dirx;
                    j += dirz;
                    break;
                }
            }
            if (c >= 4)
                break;
        }
        if (abs(i - i0) > maxrad || abs(j - j0) > maxrad)
            break; // we have gone too far from the origin
    }

    return v;
}


int getParaRange(const DoublePerlinNoise *para, double *pmin, double *pmax,
    int x, int z, int w, int h, void *data, int (*func)(void*,int,int,double))
{
    const double beta = 1.5;
    const double factor = 10000;
    const double perlin_grad = 2.0 * 1.875; // max perlin noise gradient
    double v, lmin, lmax, dr, vdif, small_regime;
    char *skip = NULL;
    int i, j, step, ii, jj, ww, hh, skipsiz;
    int maxrad, maxiter;
    int err = 1;

    if (pmin) *pmin = DBL_MAX;
    if (pmax) *pmax = -DBL_MAX;

    lmin = DBL_MAX, lmax = 0;
    for (i = 0; i < para->octA.octcnt; i++)
    {
        double lac = para->octA.octaves[i].lacunarity;
        if (lac < lmin) lmin = lac;
        if (lac > lmax) lmax = lac;
    }

    // Sort out the small area cases where we are less likely to improve upon
    // checking all positions.
    small_regime = 1e3 * sqrt(lmax);
    if (w*h < small_regime)
    {
        for (j = 0; j < h; j++)
        {
            for (i = 0; i < w; i++)
            {
                v = factor * sampleDoublePerlin(para, x+i, 0, z+j);
                if (func)
                {
                    err = func(data, x+i, z+j, v);
                    if (err)
                        return err;
                }
                if (pmin && v < *pmin) *pmin = v;
                if (pmax && v > *pmax) *pmax = v;
            }
        }
        return 0;
    }

    // Start with the largest noise period to get some bounds for pmin, pmax
    step = (int) (0.5 / lmin - FLT_EPSILON) + 1;

    dr = lmax / lmin * beta;
    for (j = 0; j < h; j += step)
    {
        for (i = 0; i < w; i += step)
        {
            if (pmin)
            {
                v = getParaDescent(para, +factor, x, z, w, h, i, j,
                    step, step, dr, data, func);
                if (v != v) goto L_end;
                if (v < *pmin) *pmin = v;
            }
            if (pmax)
            {
                v = -getParaDescent(para, -factor, x, z, w, h, i, j,
                    step, step, dr, data, func);
                if (v != v) goto L_end;
                if (v > *pmax) *pmax = v;
            }
        }
    }

    //(*(double*)data) = -1e9+1; // testing

    step = (int) (1.0 / (perlin_grad * lmax + FLT_EPSILON)) + 1;

    /// We can determine the maximum contribution we expect from all noise
    /// periods for a distance of step. If this does not account for the
    /// necessary difference, we can skip that point.
    vdif = 0;
    for (i = 0; i < para->octA.octcnt; i++)
    {
        const PerlinNoise *p = para->octA.octaves + i;
        double contrib = step * p->lacunarity * 1.0;
        if (contrib > 1.0) contrib = 1;
        vdif += contrib * p->amplitude;
    }
    for (i = 0; i < para->octB.octcnt; i++)
    {
        const double lac_factB = 337.0 / 331.0;
        const PerlinNoise *p = para->octB.octaves + i;
        double contrib = step * p->lacunarity * lac_factB;
        if (contrib > 1.0) contrib = 1;
        vdif += contrib * p->amplitude;
    }
    vdif = fabs(factor * vdif * para->amplitude);
    //printf("%g %g %g\n", para->amplitude, 1./lmin, 1./lmax);
    //printf("first pass: [%g %g] diff=%g step:%d\n", *pmin, *pmax, vdif, step);

    maxrad = step;
    maxiter = step*2;
    ww = (w+step-1) / step;
    hh = (h+step-1) / step;
    skipsiz = (ww+1) * (hh+1) * sizeof(*skip);
    skip = (char*) malloc(skipsiz);

    if (pmin)
    {   // look for minima
        memset(skip, 0, skipsiz);

        for (jj = 0; jj <= hh; jj++)
        {
            j = jj * step; if (j >= h) j = h-1;
            for (ii = 0; ii <= ww; ii++)
            {
                i = ii * step; if (i >= w) i = w-1;
                if (skip[jj*ww+ii]) continue;

                v = factor * sampleDoublePerlin(para, x+i, 0, z+j);
                if (func)
                {
                    int e = func(data, x+i, z+j, v);
                    if (e)
                    {
                        err = e;
                        goto L_end;
                    }
                }
                // not looking for maxima yet, but update the bounds anyway
                if (pmax && v > *pmax) *pmax = v;

                dr = beta * (v - *pmin) / vdif;
                if (dr > 1.0)
                {   // difference is too large -> mark visinity to be skipped
                    int a, b, r = (int) dr;
                    for (b = 0; b < r; b++)
                    {
                        if (b+jj < 0 || b+jj >= hh) continue;
                        for (a = -r+1; a < r; a++)
                        {
                            if (a+ii < 0 || a+ii >= ww) continue;
                            skip[(b+jj)*ww + (a+ii)] = 1;
                        }
                    }
                    continue;
                }
                v = getParaDescent(para, +factor, x, z, w, h, i, j,
                    maxrad, maxiter, dr, data, func);
                if (v != v) goto L_end;
                if (v < *pmin) *pmin = v;
            }
        }
    }

    if (pmax)
    {   // look for maxima
        memset(skip, 0, skipsiz);

        for (jj = 0; jj <= hh; jj++)
        {
            j = jj * step; if (j >= h) j = h-1;
            for (ii = 0; ii <= ww; ii++)
            {
                i = ii * step; if (i >= w) i = w-1;
                if (skip[jj*ww+ii]) continue;

                v = -factor * sampleDoublePerlin(para, x+i, 0, z+j);
                if (func)
                {
                    int e = func(data, x+i, z+j, -v);
                    if (e)
                    {
                        err = e;
                        goto L_end;
                    }
                }

                dr = beta * (v + *pmax) / vdif;
                if (dr > 1.0)
                {   // difference too large -> mark visinity to be skipped
                    int a, b, r = (int) dr;
                    for (b = 0; b < r; b++)
                    {
                        if (b+jj < 0 || b+jj >= hh) continue;
                        for (a = -r+1; a < r; a++)
                        {
                            if (a+ii < 0 || a+ii >= ww) continue;
                            skip[(b+jj)*ww + (a+ii)] = 1;
                        }
                    }
                    continue;
                }
                v = -getParaDescent(para, -factor, x, z, w, h, i, j,
                    maxrad, maxiter, dr, data, func);
                if (v != v) goto L_end;
                if (v > *pmax) *pmax = v;
            }
        }
    }

    err = 0;
L_end:
    if (skip)
        free(skip);
    return err;
}

#define IMIN INT_MIN
#define IMAX INT_MAX
static const int g_biome_para_range_18[][13] = {
/// biome                   temperature  humidity     continental. erosion      depth        weirdness
{ocean                   , -1500, 2000,  IMIN, IMAX, -4550,-1900,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{plains                  , -4500, 5500,  IMIN, 1000, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{desert                  ,  5500, IMAX,  IMIN, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{windswept_hills         ,  IMIN, 2000,  IMIN, 1000, -1899, IMAX,  4500, 5500,  IMIN, IMAX,  IMIN, IMAX},
{forest                  , -4500, 5500, -1000, 3000, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{taiga                   ,  IMIN,-1500,  1000, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{swamp                   , -4500, IMAX,  IMIN, IMAX, -1100, IMAX,  5500, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{river                   , -4500, IMAX,  IMIN, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500,  500},
{frozen_ocean            ,  IMIN,-4501,  IMIN, IMAX, -4550,-1900,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{frozen_river            ,  IMIN,-4501,  IMIN, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500,  500},
{snowy_plains            ,  IMIN,-4500,  IMIN, 1000, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{mushroom_fields         ,  IMIN, IMAX,  IMIN, IMAX, IMIN,-10500,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{beach                   , -4500, 5500,  IMIN, IMAX, -1900,-1100, -2225, IMAX,  IMIN, IMAX,  IMIN, 2666},
{jungle                  ,  2000, 5500,  1000, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{sparse_jungle           ,  2000, 5500,  1000, 3000, -1899, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500, IMAX},
{deep_ocean              , -1500, 2000,  IMIN, IMAX,-10500,-4551,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{stony_shore             ,  IMIN, IMAX,  IMIN, IMAX, -1900,-1100,  IMIN,-2225,  IMIN, IMAX,  IMIN, IMAX},
{snowy_beach             ,  IMIN,-4500,  IMIN, IMAX, -1900,-1100, -2225, IMAX,  IMIN, IMAX,  IMIN, 2666},
{birch_forest            , -1500, 2000,  1000, 3000, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{dark_forest             , -1500, 2000,  3000, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{snowy_taiga             ,  IMIN,-4500, -1000, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{old_growth_pine_taiga   , -4500,-1500,  3000, IMAX, -1899, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500, IMAX},
{windswept_forest        ,  IMIN, 2000,  1000, IMAX, -1899, IMAX,  4500, 5500,  IMIN, IMAX,  IMIN, IMAX},
{savanna                 ,  2000, 5500,  IMIN,-1000, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{savanna_plateau         ,  2000, 5500,  IMIN,-1000, -1100, IMAX,  IMIN,  500,  IMIN, IMAX,  IMIN, IMAX},
{badlands                ,  5500, IMAX,  IMIN, 1000, -1899, IMAX,  IMIN,  500,  IMIN, IMAX,  IMIN, IMAX},
{wooded_badlands         ,  5500, IMAX,  1000, IMAX, -1899, IMAX,  IMIN,  500,  IMIN, IMAX,  IMIN, IMAX},
{warm_ocean              ,  5500, IMAX,  IMIN, IMAX,-10500,-1900,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{lukewarm_ocean          ,  2001, 5500,  IMIN, IMAX, -4550,-1900,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{cold_ocean              , -4500,-1501,  IMIN, IMAX, -4550,-1900,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{deep_lukewarm_ocean     ,  2001, 5500,  IMIN, IMAX,-10500,-4551,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{deep_cold_ocean         , -4500,-1501,  IMIN, IMAX,-10500,-4551,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{deep_frozen_ocean       ,  IMIN,-4501,  IMIN, IMAX,-10500,-4551,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{sunflower_plains        , -1500, 2000,  IMIN,-3500, -1899, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500, IMAX},
{windswept_gravelly_hills,  IMIN,-1500,  IMIN,-1000, -1899, IMAX,  4500, 5500,  IMIN, IMAX,  IMIN, IMAX},
{flower_forest           , -1500, 2000,  IMIN,-3500, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, -500},
{ice_spikes              ,  IMIN,-4500,  IMIN,-3500, -1899, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500, IMAX},
{old_growth_birch_forest , -1500, 2000,  1000, 3000, -1899, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500, IMAX},
{old_growth_spruce_taiga , -4500,-1500,  3000, IMAX, -1900, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, -500},
{windswept_savanna       , -1500, IMAX,  IMIN, 3000, -1899,  300,  4500, 5500,  IMIN, IMAX,   501, IMAX},
{eroded_badlands         ,  5500, IMAX,  IMIN,-1000, -1899, IMAX,  IMIN,  500,  IMIN, IMAX,  IMIN, IMAX},
{bamboo_jungle           ,  2000, 5500,  3000, IMAX, -1899, IMAX,  IMIN, IMAX,  IMIN, IMAX,  -500, IMAX},
{dripstone_caves         ,  IMIN, IMAX,  IMIN, 6999,  3001, IMAX,  IMIN, IMAX,  1000, 9500,  IMIN, IMAX},
{lush_caves              ,  IMIN, IMAX,  2001, IMAX,  IMIN, IMAX,  IMIN, IMAX,  1000, 9500,  IMIN, IMAX},
{meadow                  , -4500, 2000,  IMIN, 3000,   300, IMAX, -7799,  500,  IMIN, IMAX,  IMIN, IMAX},
{grove                   ,  IMIN, 2000, -1000, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN, IMAX,  IMIN, IMAX},
{snowy_slopes            ,  IMIN, 2000,  IMIN,-1000, -1899, IMAX,  IMIN,-3750,  IMIN, IMAX,  IMIN, IMAX},
{jagged_peaks            ,  IMIN, 2000,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN, IMAX, -9333,-4001},
{frozen_peaks            ,  IMIN, 2000,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN, IMAX,  4000, 9333},
{stony_peaks             ,  2000, 5500,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN, IMAX, -9333, 9333},
{-1,0,0,0,0,0,0,0,0,0,0,0,0}};

static const int g_biome_para_range_19_diff[][13] = {
{eroded_badlands         ,  5500, IMAX,  IMIN,-1000, -1899, IMAX,  IMIN,  500,  IMIN, IMAX,  -500, IMAX},
{grove                   ,  IMIN, 2000, -1000, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN,10499,  IMIN, IMAX},
{snowy_slopes            ,  IMIN, 2000,  IMIN,-1000, -1899, IMAX,  IMIN,-3750,  IMIN,10499,  IMIN, IMAX},
{jagged_peaks            ,  IMIN, 2000,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN,10499, -9333,-4001},
{deep_dark               ,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, 1818, 10500, IMAX,  IMIN, IMAX},
{mangrove_swamp          ,  2000, IMAX,  IMIN, IMAX, -1100, IMAX,  5500, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{-1,0,0,0,0,0,0,0,0,0,0,0,0}};

static const int g_biome_para_range_20_diff[][13] = {
{swamp                   , -4500, 2000,  IMIN, IMAX, -1100, IMAX,  5500, IMAX,  IMIN, IMAX,  IMIN, IMAX},
{grove                   ,  IMIN, 2000, -1000, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN,10500,  IMIN, IMAX},
{snowy_slopes            ,  IMIN, 2000,  IMIN,-1000, -1899, IMAX,  IMIN,-3750,  IMIN,10500,  IMIN, IMAX},
{jagged_peaks            ,  IMIN, 2000,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN,10500, -9333,-4000},
{frozen_peaks            ,  IMIN, 2000,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN,10500,  4000, 9333},
{stony_peaks             ,  2000, 5500,  IMIN, IMAX, -1899, IMAX,  IMIN,-3750,  IMIN,10500, -9333, 9333},
{cherry_grove            , -4500, 2000,  IMIN,-1000,   300, IMAX, -7799,  500,  IMIN, IMAX,  2666, IMAX},
{-1,0,0,0,0,0,0,0,0,0,0,0,0}};

static const int g_biome_para_range_21wd_diff[][13] = {
{pale_garden             , -1500, 2000,  3000, IMAX,   300, IMAX, -7799,  500,  IMIN, IMAX,  2666, IMAX},
{-1,0,0,0,0,0,0,0,0,0,0,0,0}};

static const int g_biome_para_range_262_diff[][13] = {
{sulfur_caves            ,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX,  IMIN, IMAX,  2000, 9000,  IMIN,-9500},
{-1,0,0,0,0,0,0,0,0,0,0,0,0}};

static const int g_biome_para_range_264_diff[][13] = {
{dappled_forest          , -4500,-1500,-10000,-3500,   300, 3000, -2225,  500,  IMIN, IMAX,  5666, 7666},
{-1,0,0,0,0,0,0,0,0,0,0,0,0}};

/**
 * Gets the min/max parameter values within which a biome change can occur.
 */
const int *getBiomeParaExtremes(int mc)
{
    if (mc <= MC_B1_7)
    {
        static const int extremes_beta[] = {
            0, 10000,
            0, 10000,
            0,0, 0,0, 0,0, 0,0,
        };
        return extremes_beta;
    }
    if (mc < MC_1_18)
        return NULL;
    static const int extremes[] = {
        -4501, 5500,
        -3500, 6999,
        -10500, 300,
        -7799, 5500,
        1000, 10500, // depth has more dependencies
        -9333, 9333,
    };
    return extremes;
}

/**
 * Gets the min/max possible noise parameter values at which the given biome
 * can generate. The values are in min/max pairs in order:
 * temperature, humidity, continentalness, erosion, depth, weirdness.
 */
const int *getBiomeParaLimits(int mc, int id)
{
    if (mc < MC_1_18)
        return NULL;
    int i;
    if (mc > MC_26_30)
    {
        for (i = 0; g_biome_para_range_264_diff[i][0] != -1; i++)
        {
            if (g_biome_para_range_264_diff[i][0] == id)
                return &g_biome_para_range_264_diff[i][1];
        }
    }
    if (mc > MC_1_21_60)
    {
        for (i = 0; g_biome_para_range_262_diff[i][0] != -1; i++)
        {
            if (g_biome_para_range_262_diff[i][0] == id)
                return &g_biome_para_range_262_diff[i][1];
        }
    }
    if (mc > MC_1_21)
    {
        for (i = 0; g_biome_para_range_21wd_diff[i][0] != -1; i++)
        {
            if (g_biome_para_range_21wd_diff[i][0] == id)
                return &g_biome_para_range_21wd_diff[i][1];
        }
    }
    if (mc > MC_1_19)
    {
        for (i = 0; g_biome_para_range_20_diff[i][0] != -1; i++)
        {
            if (g_biome_para_range_20_diff[i][0] == id)
                return &g_biome_para_range_20_diff[i][1];
        }
    }
    if (mc > MC_1_18)
    {
        for (i = 0; g_biome_para_range_19_diff[i][0] != -1; i++)
        {
            if (g_biome_para_range_19_diff[i][0] == id)
                return &g_biome_para_range_19_diff[i][1];
        }
    }
    for (i = 0; g_biome_para_range_18[i][0] != -1; i++)
    {
        if (g_biome_para_range_18[i][0] == id)
            return &g_biome_para_range_18[i][1];
    }
    return NULL;
}

/**
 * Determines which biomes are able to generate given climate parameter limits.
 * Possible biomes are marked non-zero in the 'ids'.
 */
void getPossibleBiomesForLimits(char ids[256], int mc, int limits[6][2])
{
    int i, j;
    memset(ids, 0, 256*sizeof(char));

    for (i = 0; i < 256; i++)
    {
        if (!isOverworld(mc, i))
            continue;
        const int *bp = getBiomeParaLimits(mc, i);
        if (!bp)
            continue;

        for (j = 0; j < 6; j++)
        {
            if (limits[j][0] > bp[2*j+1] || limits[j][1] < bp[2*j+0])
                break;
        }
        if (j >= 6)
            ids[bp[-1]] = 1;
    }
}

int getLargestRec(int match, const int *ids, int sx, int sz, Pos *p0, Pos *p1)
{
    typedef struct { int n, j, w; } entry_t;
    entry_t *meta = (entry_t*) calloc(sx > sz ? sx : sz, sizeof(*meta));
    int i, j, w, m, ret;
    ret = m = 0;

    for (i = sx-1; i >= 0; i--)
    {
        for (j = 0; j < sz; j++)
        {
            if (ids[j*sx + i] == match)
                meta[j].n++;
            else
                meta[j].n = 0;
        }
        for (w = j = 0; j < sz; j++)
        {
            int n = meta[j].n;
            if (n > w)
            {
                meta[m].j = j;
                meta[m].w = w;
                m++;
                w = n;
            }
            if (n == w)
                continue;
            do
            {
                entry_t e = meta[--m];
                int area = w * (j - e.j);
                if (area > ret)
                {
                    p0->x = i; p0->z = e.j;
                    p1->x = i+w-1; p1->z = j-1;
                    ret = area;
                }
                w = e.w;
            }
            while (n < w);
            if ((w = n))
                m++;
        }
    }
    free(meta);
    return ret;
}


