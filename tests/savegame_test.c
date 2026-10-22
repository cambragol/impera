#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "common/common.h"
#include "common/settings.h"
#include "funcs.h"
#include "mod/contents.h"
#include "mod/contents_game.h"
#include "mod/discoveries.h"
#include "npc.h"
#include "town.h"
#include "mod/actors.h"
#include "mod/types.h"
#include "text/dialogue_game.h"
#include <stdlib.h>
#include "savegame.h"
#include "vars.h"

int main(int argc, char** argv)
{
    char value[3];
    FILE* stream;
    int i;
    static const char* npcFiles[] = { "TOWNE.NPC", "DWELLING.NPC", "CASTLE.NPC", "KEEP.NPC" };
    static const int maps[] = { 1, 13, 17, 25 };

    if (argc > 1 && strcmp(argv[1], "--missing-required") == 0)
    {
        unsigned char buffer[32];
        DEBUG_Initialize();
        ULTIMA_256e_ReadFileFromDisk("missing-required.dat", buffer, sizeof(buffer), 0);
        return 0;
    }

    errno = ERANGE;
    assert(SETTINGS_GetInt("window", "width", 1280) == 1280);
    assert(SETTINGS_GetInt("window", "height", 960) == 960);
    SETTINGS_GetString("test", "key", "abcd", value, sizeof(value));
    assert(strcmp(value, "ab") == 0);

    D_55a6 = 1234;
    strcpy(D_55a8_party[0].name, "Test");
    D_55a8_party[0].hp = 100;
    D_55a8_party[0].maxHp = 100;
    assert(FILE_ReadSavegameFile("missing.gam") == -1);
    assert(D_55a6 == 1234 && D_55a8_party[0].hp == 100);

    stream = fopen("truncated.gam", "wb");
    assert(stream);
    for (i = 0; i < 0x105f; i++)
        assert(fputc(0, stream) != EOF);
    assert(fclose(stream) == 0);
    assert(FILE_ReadSavegameFile("truncated.gam") == -1);
    assert(D_55a6 == 1234 && D_55a8_party[0].hp == 100);
    assert(strcmp(D_55a8_party[0].name, "Test") == 0);
    remove("truncated.gam");

    assert(FILE_WriteSavegameFile("SAVED.GAM") == 0);
    stream = fopen("SAVEGAME/SAVED.GAM", "rb");
    assert(stream);
    assert(fseek(stream, 0, SEEK_END) == 0);
    assert(ftell(stream) == 0x1060);
    fclose(stream);
    D_55a6 = 0;
    memset(D_55a8_party, 0, sizeof(S_55a8) * 16);
    assert(FILE_ReadSavegameFile("sAvEd.gAm") == 0);
    assert(D_55a6 == 1234 && D_55a8_party[0].hp == 100);
    assert(strcmp(D_55a8_party[0].name, "Test") == 0);
    remove("SAVEGAME/SAVED.GAM");

    /* Contents grants retain quantities over inventory limits; no actor slots are used. */
    for (i = 0; i < 113; i++)
    {
        assert(U5_Discoveries[i][0] == D_3e78[i]);
        assert(U5_Discoveries[i][1] == D_3eea[i]);
        assert(U5_Discoveries[i][2] == D_3f5c[i]);
        assert(U5_Discoveries[i][3] == D_3fce[i]);
        assert(U5_Discoveries[i][4] == D_4040[i]);
        assert(U5_Discoveries[i][5] == D_40b2[i]);
    }
    D_57b1 = 97;
    ContentsItem item = {6, 0, 5};
    assert(CONTENTS_GAME_Grant(&item, 5) == 2 && D_57b1 == 99);
    assert(CONTENTS_GAME_Grant(&item, 3) == 0 && D_57b1 == 99);
    D_57aa = 9998;
    item = (ContentsItem){1, 0, 5};
    assert(CONTENTS_GAME_Grant(&item, 5) == 1 && D_57aa == 9999);
    D_57c0[2] = 98;
    item = (ContentsItem){7, 2, 5};
    assert(CONTENTS_GAME_Grant(&item, 5) == 1 && D_57c0[2] == 99);
    item = (ContentsItem){7, 48, 1};
    assert(CONTENTS_GAME_Grant(&item, 1) == 0);
    unsigned char contents[128] = {0};
    memcpy(contents, "ICONT001", 8);
    contents[8] = 1;
    CONTENTS_PutWord(contents + 24, 1);
    ContentsDef def = {0};
    def.id = 15;
    def.map = 5;
    def.x = 2;
    def.y = 2;
    def.slot = 255;
    def.original = 14;
    def.refill = 2;
    def.count = 1;
    def.items[0] = (ContentsItem){6, 0, 5};
    CONTENTS_EncodeRecord(contents + 32, &def);
    CONTENTS_Mount(contents, sizeof(contents));
    const ContentsDef *mounted = CONTENTS_Find(0, 5, 0, 2, 2);
    assert(mounted);
    ContentsState *state = CONTENTS_State(mounted);
    assert(CONTENTS_Reveal(mounted, state, 12345));
    state->remaining[0] = 3;
    state->flags |= 2 | 4;
    assert(FILE_WriteSavegameFile("SAVED.GAM") == 0);
    CONTENTS_Reset();
    D_55a6 = 0;
    assert(FILE_ReadSavegameFile("SAVED.GAM") == 0 && D_55a6 == 1234);
    assert(CONTENTS_State(mounted)->remaining[0] == 3 && CONTENTS_State(mounted)->day == 12345 &&
           CONTENTS_State(mounted)->flags == 15);
    stream = fopen("SAVEGAME/SAVED.GAM", "r+b");
    assert(stream);
    assert(fseek(stream, 0x1060 + 17, SEEK_SET) == 0);
    assert(fputc(255, stream) != EOF);
    fclose(stream);
    D_55a6 = 777;
    assert(FILE_ReadSavegameFile("SAVED.GAM") == -1 && D_55a6 == 777);
    assert(CONTENTS_State(mounted)->remaining[0] == 3);
    CONTENTS_Reset();
    assert(FILE_WriteSavegameFile("INIT.GAM") == 0);
    state = CONTENTS_State(mounted);
    state->remaining[0] = 2;
    assert(FILE_ReadSavegameFile("INIT.GAM") == 0 && CONTENTS_State(mounted)->remaining[0] == 0);
    remove("INIT.GAM");
    remove("SAVEGAME/SAVED.GAM");
    CONTENTS_Unmount();

    /* Entering a town reloads NPC schedules. Use uppercase-only original assets
     * to catch filename case errors, including Iolo's Hut (map 13). */
    for (i = 0; i < 4; i++)
    {
        unsigned char expected[0x240];
        int npc;
        int offset = ((maps[i] - 1) % 8) * 0x240;
        stream = fopen(npcFiles[i], "rb");
        assert(stream);
        assert(fseek(stream, offset, SEEK_SET) == 0);
        assert(fread(expected, 1, sizeof(expected), stream) == sizeof(expected));
        fclose(stream);
        D_5893_map_id = maps[i];
        NPC_0000_LoadNpcFile();
        assert(D_5893_map_id == maps[i]);
        assert(memcmp(D_5d5e, expected, 0x200) == 0);
        assert(memcmp(D_659e, expected + 0x200, 0x20) == 0);
        for (npc = 0; npc < 32; npc++)
            assert(D_5f5e[npc]._a == expected[0x220 + npc]);
    }

    /* Extra NPCs keep full IDs through entry, targeting, schedules and save/load. */
    const unsigned extra = 226, length = IMPERA_NPC_EXTRA_START + extra * 24;
    unsigned char *extended = calloc(1, length);
    assert(extended);
    memcpy(extended + 4608, "INPC0001", 8);
    TYPES_Put32(extended + 4616, extra);
    for (unsigned n = 0; n < extra; n++) {
        unsigned slot = 32 + n, off = IMPERA_NPC_EXTRA_START + n * 24;
        extended[off] = 4;
        extended[off + 2] = slot;
        extended[off + 3] = slot >> 8;
        for (unsigned j = 0; j < 3; j++) {
            extended[off + 4 + 3 + j] = slot == 257 ? 11 : 10;
            extended[off + 4 + 6 + j] = 10;
        }
        for (unsigned j = 0; j < 4; j++)
            extended[off + 4 + 12 + j] = j * 6;
        if (slot == 32 || slot == 257) {
            extended[off + 20] = 0x50;
            extended[off + 21] = 3;
        }
    }
    assert(NPCS_Validate(extended, length));
    extended[IMPERA_NPC_EXTRA_START + 2] = 33;
    assert(!NPCS_Validate(extended, length));
    extended[IMPERA_NPC_EXTRA_START + 2] = 32;
    stream = fopen("EXTRA.NPC", "wb");
    assert(stream);
    assert(fwrite(extended, 1, length, stream) == length);
    fclose(stream);
    free(extended);
    NPC_ResetExtra();
    D_5893_map_id = 13;
    D_5895_map_level = 0;
    D_587f = 12;
    memset(D_659e, 0, 32);
    memset(D_5c5a, 0, 32 * sizeof(*D_5c5a));
    NPC_LoadExtra("EXTRA.NPC", 4);
    assert(LOCATION_NPC_COUNT == 258 && D_5f5e[257]._a == 3);
    NPC_00d6(D_587f);
    TOWN_1694();
    assert(D_5f5e[32]._c == 32 && D_5f5e[257]._c == 257);
    assert(ULTIMA_368e_FindActorTileAtPos(11, 10, 0) == 0x50 && D_5876 == 257);
    assert(TOWN_011e(257) == 257 && ACTORS_Contains(257, 11, 10));
    /* The original pursuit guard still crashed on slot 32 even after entry and
     * targeting worked. Advance actual turns for both extended index bytes. */
    ActorFmt before32 = D_5c5a[32], before257 = D_5c5a[257], beforeParty = D_5c5a[0];
    NpcFmt npc32 = D_5f5e[32], npc257 = D_5f5e[257];
    NpcScheduleFmt schedule32 = D_5d5e[32], schedule257 = D_5d5e[257];
    memset(D_6608, 5, 1024);
    D_5896_map_x = D_5c5a[0]._2_x = 8;
    D_5897_map_y = D_5c5a[0]._3_y = 10;
    D_587f = 13;
    memset(D_5d5e[32].type, 5, 3);
    memset(D_5d5e[257].type, 5, 3);
    for (int turn = 0; turn < 4; turn++)
        NPC_0db4(D_587f);
    assert(D_5f5e[32]._c == 32 && D_5f5e[257]._c == 257);
    assert(D_5c5a[32]._2_x != before32._2_x || D_5c5a[32]._3_y != before32._3_y);
    D_5c5a[32] = before32;
    D_5c5a[257] = before257;
    D_5c5a[0] = beforeParty;
    D_5f5e[32] = npc32;
    D_5f5e[257] = npc257;
    D_5d5e[32] = schedule32;
    D_5d5e[257] = schedule257;
    /* Stay here is a schedule behavior, independent of monster artwork and
     * dialogue. Keep both extra NPC slots stationary over actual turns. */
    byte previousShape = D_659e[32];
    D_659e[32] = D_5c5a[32]._0_tile = 216; /* daemon appearance */
    memset(D_5d5e[32].type, 0, 3);
    memset(D_5d5e[257].type, 0, 3);
    D_5f5e[32]._e = D_5f5e[257]._e = 2;
    D_5f5e[32]._0 = D_5f5e[257]._0 = 1;
    D_5f5e[32]._a = 0;
    D_5f5e[257]._a = 3;
    for (int turn = 0; turn < 8; ++turn)
        NPC_0db4(13);
    assert(D_5c5a[32]._2_x == before32._2_x && D_5c5a[32]._3_y == before32._3_y);
    assert(D_5c5a[257]._2_x == before257._2_x && D_5c5a[257]._3_y == before257._3_y);
    D_659e[32] = previousShape;
    D_5c5a[32] = before32;
    D_5f5e[32] = npc32;
    D_5f5e[257] = npc257;
    D_5d5e[32] = schedule32;
    D_5d5e[257] = schedule257;
    D_587f = 12;
    D_65be = D_65bf = 0;
    assert(!TOWN_0000_CheckNpcKilled(257));
    TOWN_0052_SetNpcKilled(257);
    assert(TOWN_0000_CheckNpcKilled(257));
    assert(!TOWN_0000_CheckNpcKilled(32));
    D_659e[32] = 0x70;
    TOWN_0052_SetNpcKilled(32);
    assert(!TOWN_0000_CheckNpcKilled(32));
    D_659e[32] = 144;
    TOWN_0052_SetNpcKilled(32);
    assert(!TOWN_0000_CheckNpcKilled(32));
    D_659e[32] = 80;
    ImperaDialogueGame game = {0};
    game.map = &D_5893_map_id;
    game.npc = 257;
    game.npcCount = 258;
    game.met = D_5bda;
    game.killed = D_5b5a;
    game.extraMet = D_5bda + 128;
    game.extraKilled = D_5b5a + 128;
    assert(!IMPERA_GameMet(&game));
    IMPERA_GameIntroduce(&game);
    assert(IMPERA_GameMet(&game));
    D_65c2[257] = 300;
    D_655e[257] = -1;
    assert(FILE_WriteSavegameFile("SAVED.GAM") == 0);
    NPC_ResetExtra();
    assert(LOCATION_NPC_COUNT == 32);
    assert(FILE_ReadSavegameFile("SAVED.GAM") == 0);
    assert(LOCATION_NPC_COUNT == 258 && D_5f5e[257]._c == 257 && D_5c5a[257]._2_x == 11);
    assert(D_65c2[257] == 300 && D_655e[257] == -1 && IMPERA_GameMet(&game));
    assert(TOWN_0000_CheckNpcKilled(257) && !TOWN_0000_CheckNpcKilled(32));
    NPC_LoadExtra("EXTRA.NPC", 4);
    assert(!D_659e[257] && D_659e[32]);
    /* Corruption rejects the entire save before touching live state. */
    stream = fopen("SAVEGAME/SAVED.GAM", "r+b");
    assert(stream);
    assert(!fseek(stream, 0x1060 + 24, SEEK_SET));
    int savedByte = fgetc(stream);
    assert(!fseek(stream, -1, SEEK_CUR));
    fputc(savedByte ^ 1, stream);
    fclose(stream);
    D_55a6 = 4321;
    assert(FILE_ReadSavegameFile("SAVED.GAM") == -1 && D_55a6 == 4321);
    remove("SAVEGAME/SAVED.GAM");
    remove("EXTRA.NPC");
    NPC_ResetExtra();
    puts("PASS Extended NPC entry, IDs above 255, killed/met flags and transactional saves");
    puts("Settings, save/load, and NPC town-entry tests passed.");
    return 0;
}
