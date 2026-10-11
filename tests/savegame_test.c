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

    puts("Settings, save/load, and NPC town-entry tests passed.");
    return 0;
}
