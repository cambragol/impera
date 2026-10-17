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
