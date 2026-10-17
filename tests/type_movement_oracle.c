#include "common/common.h"
#include "vars.h"
#include "tiles.h"
static bool ULTIMA_2bd4(byte actorTile, int targetTile)
{
    bool local_4;

    if ((D_54d4[targetTile >> 3] & (0x80 >> (targetTile & 7))) != 0)
        local_4 = 0;
    else
        local_4 = 1;

    if ((((actorTile & 0xfe) != TILE_ACTOR_AVATAR) && ((actorTile & 0xf0) != TILE_ACTOR_WIZARD)) && (((byte)targetTile & 0xfc) == TILE_MAP_CHAIR_90))
    {
        local_4 = 0;
    }

    return local_4;
}

// is walkable water tile? (walkable/passable for sea monster / flying / ghost)
static int ULTIMA_2c2e(int tile)
{
    return tile < 4 || ((byte)tile & 0xf0) == TILE_MAP_60;
}

// CHECKED
int TYPE_ORACLE_Walkable(int actorTile, int targetTile)
{
    switch (D_54f4[actorTile >> 2])
    {
    case 0: // normal
        return ULTIMA_2bd4(actorTile, targetTile);
    case 1: // sea monsters
        return ULTIMA_2c2e(targetTile);
    case 2: // flying
        if (((byte)targetTile & 0xf0) == TILE_MAP_60 || ULTIMA_2c2e(targetTile) != 0 || ULTIMA_2bd4(actorTile, targetTile) != 0)
        {
            return 1;
        }
        return 0;
    default:
        return 0;
    case 3: // horse
        if (ULTIMA_2bd4(actorTile, targetTile) == 0 || targetTile == TILE_MAP_LAVA || targetTile == 4)
        {
            return 0;
        }
        return 1;
    case 4: // ghost / shadowlord
        return ULTIMA_2c2e(targetTile) == 0;
    case 5: // skiff
        if (((byte)targetTile & 0xfc) == TILE_MAP_34)
        {
            return (8 >> (actorTile & 3) & (uint)D_5544[targetTile - TILE_MAP_34]) != 0;
        }
        if (ULTIMA_2c2e(targetTile) == 0)
        {
            return 0;
        }
        if (targetTile < TILE_MAP_60)
        {
            return 1;
        }
        if (!((uint)D_5534[targetTile - TILE_MAP_60] & (8 >> (actorTile & 3))))
            return 0;
        else
            return 1;
    case 6: // frigate
        if (targetTile > 2)
        {
            return 0;
        }
        return 1;
    case 7: // rotworm
        return targetTile == TILE_MAP_POISON;
    case 8: // corpser
        return targetTile == TILE_MAP_GRASS;
    case 9: // whirlpool
        return targetTile == TILE_MAP_WATER_1;
    case 10: // sand trap
        return targetTile == TILE_MAP_7;
    }
}
