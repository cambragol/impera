#include "funcs.h"
#include "mod/contents.h"
#include "mod/actors.h"
#include "comsubs.h"
#include "combat.h"
#include "mod/package.h"
#include "mod/types.h"
#include "mod/types_game.h"
#include "savegame.h"
#include "tiles.h"
static void customTypeGame(void) {
    const unsigned recordSize = 256 + 4 + 32 * 32 * 4, size = 32 + recordSize * 4;
    unsigned char *defs = calloc(1, size);
    assert(defs);
    memcpy(defs, "ITYPES01", 8);
    defs[8] = 17;
    TYPES_Put32(defs + 24, 4);
    for (unsigned i = 0; i < 4; i++) {
        TypeDef d;
        TYPES_Vanilla(i < 2 ? 0 : 1, i < 2 ? (i ? 0x44 : 0xb8) : 144, &d);
        d.id = 40000 + i;
        d.frames = 1;
        d.ground = 5;
        d.render = TYPES_SPRITE | TYPES_MASONRY;
        if (i < 2) {
            d.result[0] = 40001;
            d.result[1] = 40000;
        } else {
            d.kind = 2;
            d.stats[5] = i == 2 ? 30 : 90;
            d.stats[6] = 1;
            d.actorFlags = i == 2 ? 0x410 : 0x2000;
            d.result[1] = d.result[2] = d.result[3] = 40003;
        }
        strcpy(d.name, i ? "Open crystal door" : "Closed crystal door");
        unsigned char *r = defs + 32 + i * recordSize;
        TYPES_Encode(r, &d);
        TYPES_Put32(r, recordSize);
        r[257] = 32;
        r[258] = 100;
        for (unsigned p = 0; p < 32 * 32; p++) {
            r[260 + p * 4] = p % 32;
            r[261 + p * 4] = p / 32;
            r[262 + p * 4] = 200;
            r[263 + p * 4] = 255;
        }
        /* Opaque black and a transparent pixel are deliberately distinct. */
        memset(r + 260, 0, 4);
        r[263] = 255;
        memset(r + 264, 0, 4);
    }
    assert(TYPES_Validate(defs, size));
    unsigned char map[48] = {0};
    memcpy(map, "ITYMAP01", 8);
    map[8] = 17;
    TYPES_Put32(map + 24, 1);
    TypeCell c = {0, 13, 0, 10, 11, 40000};
    TYPES_CellEncode(map + 32, &c);
    assert(TYPES_ValidateMap(map, 48, defs, size));
    byte oldMap = D_5893_map_id, oldFloor = D_5895_map_level, oldX = D_5896_map_x,
         oldY = D_5897_map_y;
    unsigned char base[1024];
    memcpy(base, D_6608, 1024);
    D_5893_map_id = 13;
    D_5895_map_level = 0;
    D_5896_map_x = 10;
    D_5897_map_y = 11;
    memset(D_6608, 5, 1024);
    TYPES_Mount(defs, size, map, 48);
    assert(TYPES_GAME_At(10, 11) == 40000);
    assert(ACTORS_Stats(40002).maxHp == 30 && ACTORS_Stats(40003).maxHp == 90);
    assert(ACTORS_Def(40002).base == ACTORS_Def(40003).base);
    assert(GRAP_BUF_SpritePixel(40002, -1, 0) == -1);
    assert(GRAP_BUF_SpritePixel(40002, 16, 0) == -1);
    assert((ACTORS_Flags(40002) & 0x410) == 0x410 && ACTORS_Flags(40003) == 0x2000);
    assert(ACTORS_Target(40002, 1, 0x26) == 40003);
    assert(ACTORS_Target(40002, 2, 0) == 40003);
    TypeCell npcType = {3, 13, 0, 5, 0, 40002, 0};
    assert(TYPES_Set(npcType, 40002));
    NpcFmt oldNpc = D_5f5e[5];
    ActorFmt oldTypedActor = D_5c5a[12];
    D_5f5e[5]._0 = 1;
    D_5f5e[5]._c = 12;
    D_5c5a[12]._0_tile = D_5c5a[12]._1_animTile = 144;
    assert(ACTORS_Instance(12) == 40002);
    assert(ACTORS_Sprite(12, 400) == 40002);
    D_5f5e[5] = oldNpc;
    D_5c5a[12] = oldTypedActor;

    assert(*ULTIMA_4402_GetTileAddr(10, 11) == 0xb8);
    assert(!TYPES_GAME_Walkable(0x1c, 10, 11));
    assert(TYPES_GAME_Property(10, 11, 2, 0));
    TYPES_GAME_Change(10, 11, 0, 0x44);
    assert(TYPES_GAME_At(10, 11) == 40001);
    assert(TYPES_GAME_Walkable(0x1c, 10, 11));
    TYPES_GAME_Change(10, 11, 1, 0xb8);
    assert(TYPES_GAME_At(10, 11) == 40000);
    assert(TILESET_Select(TILESET_DOS));
    unsigned char pixel = 0;
    TILESET_Register(&pixel, 1);
    int color = GRAP_BUF_TilePixel(40000, 0, 0);
    assert(color == 16);
    TILESET_RecordSprite(&pixel, color, true);
    pixel = color;
    assert(TILESET_BufferColor(&pixel, 0, 0, ega) == 0xff000000u);
    assert(TILESET_BufferColor(&pixel, 1, 0, ega) == ega[0]);
    color = GRAP_BUF_TilePixel(40000, 1, 1);
    TILESET_Record(&pixel, color);
    pixel = color;
    assert(TILESET_BufferColor(&pixel, 0, 0, ega) == 0xff0202c8u);
    assert(TILESET_BufferColor(&pixel, 1, 1, ega) == 0xff0303c8u);
    TILESET_Unregister(&pixel);
    /* Loading a library must not make ordinary vanilla terrain persistent. */
    TYPES_GAME_Write(20, 20, 0x44);
    TypeCell ordinary = TYPES_GAME_Cell(20, 20), ordinaryState;
    assert(!TYPES_State(ordinary, &ordinaryState));
    assert(*ULTIMA_4402_GetTileAddr(20, 20) == 0x44);
    assert(FILE_WriteSavegameFile("custom-types.gam") == 0);
    TYPES_GAME_Write(10, 11, 5);
    assert(TYPES_GAME_At(10, 11) == 5);
    assert(FILE_ReadSavegameFile("custom-types.gam") == 0);
    assert(TYPES_GAME_At(10, 11) == 40000);
    TypeCell loadedNpc;
    assert(TYPES_State(npcType, &loadedNpc) && loadedNpc.type == 40002);
    remove("custom-types.gam");
    /* Vanilla doors temporarily become floor; the original custom type must
     * survive save/load even without a separately authored open-door tile. */
    TYPES_Put32(defs + 32 + 24, 0x44);
    TYPES_GAME_Change(10, 11, 0, 0x44);
    assert(TYPES_GAME_At(10, 11) == 0x44);
    assert(FILE_WriteSavegameFile("custom-open-door.gam") == 0);
    TYPES_GAME_Change(10, 11, 1, 0xb8);
    assert(TYPES_GAME_At(10, 11) == 40000);
    assert(FILE_ReadSavegameFile("custom-open-door.gam") == 0);
    assert(TYPES_GAME_At(10, 11) == 0x44);
    TYPES_GAME_Change(10, 11, 1, 0xb8);
    assert(TYPES_GAME_At(10, 11) == 40000);
    remove("custom-open-door.gam");
    /* Combat uses the same authored damage range, not the proxy terrain byte. */
    TypeCell combat = {1, 0, 0, 2, 3, 40001, 0};
    CONTENTS_BeginEncounter(1, 0);
    assert(TYPES_Set(combat, 40001));
    assert(TYPES_State(npcType, &loadedNpc) && loadedNpc.type == 40002);

    defs[32 + recordSize + 17] = TYPES_BURN;
    defs[32 + recordSize + 18] = 2;
    defs[32 + recordSize + 19] = 2;
    CombatEntity oldCombat = D_ba14[0];
    ActorFmt oldActor = D_5c5a[0];
    S_55a8 oldParty = D_55a8_party[0];
    TextWindow oldWindows[4];
    memcpy(oldWindows, D_535e_textWindows, sizeof(oldWindows));
    int oldAdvance = D_538e;
    D_538e = 1;
    for (int i = 0; i < 4; i++)
        D_535e_textWindows[i] = (TextWindow){0, 0, 39, 24, 0, 0, 15, 0};
    byte *oldCharset = D_5398_currentCharset;
    TextWindow *oldWindow = D_539a_currentTextWindow;
    byte charset[1024] = {0};
    D_5398_currentCharset = charset;
    D_539a_currentTextWindow = &D_535e_textWindows[2];
    byte sound = D_a9ce;
    D_a9ce = 0;
    D_5893_map_id = 128;
    D_ba14[0] = (CombatEntity){0};
    D_ba14[0].flags = COMBAT_FLAGS_PLAYER;
    D_ba14[0].x = 2;
    D_ba14[0].y = 3;
    D_5c5a[0]._0_tile = TILE_ACTOR_AVATAR;
    D_55a8_party[0].hp = 100;
    D_55a8_party[0].status = STATUS_GOOD;
    assert(TYPES_GAME_CombatEffects(0));
    assert(D_55a8_party[0].hp == 98);
    D_ba14[0] = oldCombat;
    D_5c5a[0] = oldActor;
    D_55a8_party[0] = oldParty;
    D_a9ce = sound;
    D_538e = oldAdvance;
    D_5398_currentCharset = oldCharset;
    D_539a_currentTextWindow = oldWindow;
    memcpy(D_535e_textWindows, oldWindows, sizeof(oldWindows));

    CombatEntity oldEntities[32];
    ActorFmt oldActors[32];
    memcpy(oldEntities, D_ba14, sizeof(oldEntities));
    memcpy(oldActors, D_5c5a, sizeof(oldActors));
    memset(D_ba14, 0, sizeof(oldEntities));
    memset(D_5c5a, 0, sizeof(oldActors));
    D_5893_map_id = 128;
    int first = ULTIMA_6506(40002, 0, 2, 2, 0);
    int second = ULTIMA_6506(40003, 0, 3, 2, 0);
    assert(first >= 6 && second >= 6 && first != second);
    assert(D_ba14[first].entityIdx == 40002 && D_ba14[first].hp == 30);
    assert(D_ba14[second].entityIdx == 40003 && D_ba14[second].hp == 90);
    assert(ACTORS_Instance(D_ba14[first].actorIdx) == 40002);
    byte savedCombatMap[352]; memcpy(savedCombatMap, D_ad14, sizeof(savedCombatMap));
    memset(D_ad14, 0x44, sizeof(savedCombatMap));
    D_538e = 1;
    for (int i = 0; i < 4; i++) D_535e_textWindows[i] = (TextWindow){0,0,39,24,0,0,15,0};
    D_5398_currentCharset = charset; D_539a_currentTextWindow = &D_535e_textWindows[2];
    D_a9ce = 0;
    D_ba14[first].x = D_5c5a[D_ba14[first].actorIdx]._2_x = 5;
    D_ba14[first].y = D_5c5a[D_ba14[first].actorIdx]._3_y = 5;
    COMBAT_1574(first, 1);
    int divided = 0;
    for (int i = 6; i < 32; i++) if (i != second && D_ba14[i].entityIdx == 40003 && D_ba14[i].flags) divided++;
    assert(divided > 0);
    byte oldNegation = D_587a; D_587a = 'A';
    ULTIMA_207e_srand(1234);
    int summoned = 0;
    for (int attempt = 0; attempt < 256 && !summoned; attempt++) summoned = COMSUBS_00f4(first);
    assert(summoned);
    int customChildren = 0;
    for (int i = 6; i < 32; i++) if (i != second && D_ba14[i].entityIdx == 40003 && D_ba14[i].flags) customChildren++;
    assert(customChildren > divided);
    D_587a = oldNegation;
    memcpy(D_ad14, savedCombatMap, sizeof(savedCombatMap));
    D_538e = oldAdvance; D_5398_currentCharset = oldCharset; D_539a_currentTextWindow = oldWindow;
    memcpy(D_535e_textWindows, oldWindows, sizeof(oldWindows)); D_a9ce = sound;

    TYPES_Put32(defs + 32 + 2 * recordSize + 24, 40003);
    assert(ACTORS_Transform(first));
    assert(D_ba14[first].entityIdx == 40003 && D_ba14[first].hp == 90);
    D_5893_map_id = 0;
    TypeCell oldWorldSlot = {4, 0, 0, 15, 0, 40002, 0};
    assert(TYPES_Set(oldWorldSlot, 40002));
    D_5c5a[15]._0_tile = 0;
    ULTIMA_3a74(144, 144, 20, 20, 0, 0, 15);
    assert(TYPES_At(oldWorldSlot, 0) == 0);
    memcpy(D_ba14, oldEntities, sizeof(oldEntities));
    memcpy(D_5c5a, oldActors, sizeof(oldActors));
    D_5893_map_id = 13;
    byte stale = 0;
    assert(TILESET_Select(TILESET_AMIGA));
    TILESET_Register(&stale, 1);
    int staleColor = GRAP_BUF_TilePixel(40000, 2, 2);
    TILESET_Record(&stale, staleColor);
    stale = staleColor;
    TYPES_Mount(NULL, 0, NULL, 0);
    assert(TILESET_BufferColor(&stale, 0, 0, ega) == ega[0]);
    TILESET_Unregister(&stale);
    assert(TILESET_Select(TILESET_DOS));
    free(defs);
    memcpy(D_6608, base, 1024);
    D_5893_map_id = oldMap;
    D_5895_map_level = oldFloor;
    D_5896_map_x = oldX;
    D_5897_map_y = oldY;
    puts("PASS custom terrain native rules, door transitions, 32px art, opaque black/alpha and "
         "native save/load");
}

static void multiTileActors(void) {
    const unsigned localBytes = 32 * 32 * 4, worldBytes = 16 * 16 * 4;
    const unsigned recordSize = 256 + 4 + localBytes + 4 + worldBytes, size = 32 + recordSize;
    unsigned char *defs = calloc(1, size);
    assert(defs);
    memcpy(defs, "ITYPES01", 8);
    defs[8] = 23;
    TYPES_Put32(defs + 24, 1);
    TypeDef d;
    TYPES_Vanilla(1, 144, &d);
    d.id = 60000;
    d.kind = 2;
    d.width = 2;
    d.height = 2;
    d.frames = 2;
    d.render = TYPES_SPRITE;
    d.stats[5] = 100;
    d.stats[6] = 1;
    unsigned char *r = defs + 32;
    TYPES_Encode(r, &d);
    TYPES_Put32(r, recordSize);
    r[256] = 0;
    r[257] = 16;
    r[258] = 100;
    for (unsigned pixel = 0; pixel < 32 * 32; ++pixel) {
        r[260 + pixel * 4] = 255;
        r[263 + pixel * 4] = 255;
    }
    unsigned off = 260 + localBytes;
    r[off] = 6;
    r[off + 1] = 16;
    r[off + 2] = 100;
    for (unsigned pixel = 0; pixel < 256; ++pixel) {
        r[off + 6 + pixel * 4] = 255;
        r[off + 7 + pixel * 4] = 255;
    }
    assert(TYPES_Validate(defs, size));
    byte oldMap = D_5893_map_id, oldFloor = D_5895_map_level, oldX = D_5896_map_x,
         oldY = D_5897_map_y;
    ActorFmt oldActors[32];
    CombatEntity oldEntities[32];
    NpcFmt oldNpcs[32];
    byte oldTerrain[1024], oldCombat[352], oldViewport[352];
    byte oldLight = D_58a5;
    D_58a5 = 99;
    memcpy(oldViewport, D_ab02, sizeof(oldViewport));
    memset(D_ab02, 0, sizeof(oldViewport));
    memcpy(oldActors, D_5c5a, sizeof(oldActors));
    memcpy(oldEntities, D_ba14, sizeof(oldEntities));
    memcpy(oldNpcs, D_5f5e, sizeof(oldNpcs));
    memcpy(oldTerrain, D_6608, sizeof(oldTerrain));
    memcpy(oldCombat, D_ad14, sizeof(oldCombat));
    memset(D_5c5a, 0, sizeof(oldActors));
    memset(D_ba14, 0, sizeof(oldEntities));
    memset(D_5f5e, 0, sizeof(oldNpcs));
    memset(D_6608, 5, sizeof(oldTerrain));
    memset(D_ad14, 5, sizeof(oldCombat));
    D_5893_map_id = 13;
    D_5895_map_level = 0;
    D_5896_map_x = D_5897_map_y = 20;
    TYPES_Mount(defs, size, NULL, 0);
    TypeCell npc = {3, 13, 0, 1, 0, 60000, 0};
    assert(TYPES_Set(npc, 60000));
    D_5f5e[1]._0 = 1;
    D_5f5e[1]._c = 12;
    D_5c5a[12] = (ActorFmt){0};
    D_5c5a[12]._0_tile = D_5c5a[12]._1_animTile = 144;
    D_5c5a[12]._2_x = D_5c5a[12]._3_y = 3;
    unsigned w, h;
    ACTORS_Size(12, &w, &h);
    assert(w == 2 && h == 2);
    assert(ACTORS_Contains(12, 4, 4) && !ACTORS_Contains(12, 5, 4));
    assert(ULTIMA_3702(4, 4, 0) == 144);
    assert(ACTORS_CanMove(12, 4, 3)); /* Own overlapping cells do not block movement. */
    assert(!ACTORS_CanMove(12, 31, 3));
    GetMap(5, 4) = 0x4f;
    assert(!ACTORS_CanMove(12, 4, 3));
    GetMap(5, 4) = 5;
    assert(TILESET_Select(TILESET_DOS));
    int color = GRAP_BUF_SpritePixel(60000, 31, 31);
    assert(color >= 0 && TILESET_LastSpriteColor(color, 0, 0, ega, true) == 0xffff0000u);
    assert(GRAP_BUF_SpritePixel(60000, 32, 31) == -1);
    GRAP_BUF_DrawSprite(g_linearEgaBuffer0, 320, 16, 16, 60000, 8, 8, 184, 184, 0, 0);
    assert(TILESET_BufferColor(g_linearEgaBuffer0 + 47 * 320 + 47, 0, 0, ega) == 0xffff0000u);
    D_5893_map_id = 0;
    TypeCell world = {4, 0, 0, 12, 0, 60000, 0};
    assert(TYPES_Set(world, 60000));
    ACTORS_Size(12, &w, &h);
    assert(w == 1 && h == 1);
    assert(!ACTORS_Contains(12, 4, 4));
    color = GRAP_BUF_SpritePixel(60000, 15, 15);
    assert(color >= 0 && TILESET_LastSpriteColor(color, 0, 0, ega, true) == 0xff0000ffu);
    assert(GRAP_BUF_SpritePixel(60000, 16, 15) == -1);
    D_5893_map_id = 128;
    D_ba14[6].flags = COMBAT_FLAGS_MONSTER;
    D_ba14[6].entityIdx = 60000;
    D_ba14[6].actorIdx = 12;
    D_ba14[6].x = D_ba14[6].y = 3;
    D_ba14[7].flags = COMBAT_FLAGS_PLAYER;
    D_ba14[7].actorIdx = 13;
    D_ba14[7].x = 5;
    D_ba14[7].y = 4;
    D_5c5a[13]._0_tile = D_5c5a[13]._1_animTile = 0x1c;
    D_5c5a[13]._2_x = 5;
    D_5c5a[13]._3_y = 4;
    assert(COMSUBS_0748(4, 4) == 6 && COMSUBS_04d4(6, 7) == 1);
    assert(!ACTORS_CanMove(12, 4, 3) && ACTORS_CanMove(12, 2, 3));
    int ax, ay, bx, by;
    ACTORS_AttackCells(6, 7, &ax, &ay, &bx, &by);
    assert(ax == 4 && ay == 4 && bx == 5 && by == 4);
    assert(!ACTORS_CanPlace(60000, 10, 10, 32));
    int spawned = ULTIMA_6506(60000, 0, 10, 10, 0);
    assert(spawned >= 6 && D_ba14[spawned].x <= 9 && D_ba14[spawned].y <= 9);
    TYPES_Mount(NULL, 0, NULL, 0);
    free(defs);
    memcpy(D_5c5a, oldActors, sizeof(oldActors));
    memcpy(D_ba14, oldEntities, sizeof(oldEntities));
    memcpy(D_5f5e, oldNpcs, sizeof(oldNpcs));
    memcpy(D_6608, oldTerrain, sizeof(oldTerrain));
    memcpy(D_ad14, oldCombat, sizeof(oldCombat));
    memcpy(D_ab02, oldViewport, sizeof(oldViewport));
    D_58a5 = oldLight;
    D_5893_map_id = oldMap;
    D_5895_map_level = oldFloor;
    D_5896_map_x = oldX;
    D_5897_map_y = oldY;
    puts("PASS multi-tile occupancy, own-overlap movement, obstacles, targeting, range, large "
         "rendering and single-tile world art");
}
