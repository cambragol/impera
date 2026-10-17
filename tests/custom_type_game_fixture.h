#include "funcs.h"
#include "mod/contents.h"
#include "mod/package.h"
#include "mod/types.h"
#include "mod/types_game.h"
#include "savegame.h"
#include "tiles.h"
static void customTypeGame(void) {
    const unsigned recordSize = 256 + 4 + 32 * 32 * 4, size = 32 + recordSize * 2;
    unsigned char *defs = calloc(1, size);
    assert(defs);
    memcpy(defs, "ITYPES01", 8);
    defs[8] = 17;
    TYPES_Put32(defs + 24, 2);
    for (unsigned i = 0; i < 2; i++) {
        TypeDef d;
        TYPES_Vanilla(0, i ? 0x44 : 0xb8, &d);
        d.id = 40000 + i;
        d.frames = 1;
        d.ground = 5;
        d.render = TYPES_SPRITE | TYPES_MASONRY;
        d.result[0] = 40001;
        d.result[1] = 40000;
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
