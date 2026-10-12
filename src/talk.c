#include "common/common.h"
#include "funcs.h"
#include "vars.h"
#include "macros.h"
#include "tiles.h"

#include <string.h>

#include "npc.h"
#include "shoppes.h"
#include "shoppes2.h"
#include "shoppes3.h"
#include "talk.h"
#include "text/text_window.h"
#include "text/dialogue_runtime.h"
#include "town.h"

#define TALK_KEY_NAME 0
#define TALK_KEY_DESC 1
#define TALK_KEY_2 2
#define TALK_KEY_3 3
#define TALK_KEY_4 4

static int TALK_0f32_ProcessChar(byte param_1);
static void TALK_127e_StartTalk(int a);

// CHECKED
static int TALK_0000_CompareStringNoCase(char* param_1, char* param_2)
{
    while (1)
    {
        if (ULTIMA_2032_ToUpper((*(byte*)param_1++) & 0x7f) != ULTIMA_2032_ToUpper((*(byte*)param_2++) & 0x7f))
        {
            return 0;
        }

        if ((*param_1 == 0) && (*param_2 == 0))
        {
            return 1;
        }
    }
}

static int TALK_0054(int param_1, int param_2)
{
    switch (*ULTIMA_4402_GetTileAddr(param_1, param_2))
    {
    case TILE_MAP_29:
    case TILE_MAP_TABLE_94:
    case TILE_MAP_TABLE_95:
    case TILE_MAP_TABLE_96:
    case TILE_MAP_97:
    case TILE_MAP_98:
    case TILE_MAP_99:
    case TILE_MAP_TABLE_9A:
    case TILE_MAP_TABLE_9B:
    case TILE_MAP_TABLE_9C:
    case TILE_MAP_DESK:
    case TILE_MAP_AE:
    case TILE_MAP_DOOR_BA:
    case TILE_MAP_DOOR_BB:
    case TILE_MAP_TABLE_BE:
    case TILE_MAP_CA:
    case TILE_MAP_CB:
        return 1;
    }

    return 0;
}

/* Shared keyboard/mouse target resolution, including talking across furniture. */
int TALK_Target(int dx,int dy,int* x,int* y)
{
    *x=D_5896_map_x+dx;*y=D_5897_map_y+dy;
    if(D_5893_map_id && (*x<0 || *y<0 || *x>=32 || *y>=32)) return 0;
    int actor=ULTIMA_368e_FindActorTileAtPos(*x,*y,D_5895_map_level);
    if(!actor && TALK_0054(*x,*y)) {
        *x+=dx;*y+=dy;
        if(D_5893_map_id && (*x<0 || *y<0 || *x>=32 || *y>=32)) return 0;
        actor=ULTIMA_368e_FindActorTileAtPos(*x,*y,D_5895_map_level);
    }
    return actor;
}

// ask pay
static int TALK_00ac_AskPay(void)
{
    char ch;

    ULTIMA_1850_PrintString(_TEXT(0x9052, "\n\nDost thou pay?\n\n:"));

    do
    {
        ch = ULTIMA_2032_ToUpper(ULTIMA_266c_GetChar());
        if (ch == 'Y')
        {
            ULTIMA_1850_PrintString(_TEXT(0x9066, "Yes\n"));
            return 0;
        }
    } while (ch != 'N');

    ULTIMA_1850_PrintString(_TEXT(0x906c, "No!\n"));
    return 1;
}

// CHECKED
// talk to merchant (shoppe)
static void TALK_00e6_TalkToMerchant(int param_1)
{
    int local_4;

    if ((D_587c_partyTile & 0xfe) == TILE_ACTOR_RIDING_HORSE && param_1 != 0x83)
    {
        ULTIMA_1850_PrintString(_TEXT(0x9072, "A merchant says:\n\"GET THAT HORSE OUT OF HERE!\"\n"));
    }
    else
    {
        if (D_587b != 0xff)
        {
            local_4 = D_587b;
        }
        else
        {
            ULTIMA_39fc_GetFirstActivePartyMember();
            local_4 = D_5876;
        }

        D_b116 = param_1 - 0x81;

        for (D_b114 = 0; D_23ca[D_b116][D_b114] != D_5893_map_id && D_b114 < 0x10; D_b114++)
        {
        }

        D_aafc = D_21ca[D_b116][D_b114];
        D_aafe = D_22ca[D_b116][D_b114];

        switch (D_b116)
        {
        case 0:
            SHOPPES_12b2(local_4);
            break;
        case 1:
            SHOPPES2_066c(local_4);
            break;
        case 2:
            SHOPPES_07be(local_4);
            break;
        case 3:
            SHOPPES2_0abc(local_4);
            break;
        case 4:
            SHOPPES_075e(local_4);
            break;
        case 5:
            SHOPPES_04a2(local_4);
            break;
        case 6:
            SHOPPES_14f8(local_4);
            break;
        case 7:
            SHOPPES3_08b4(local_4);
        }
    }
}

// CHECKED
static int TALK_01e2(void)
{
    int local_14;
    int local_16;
    char local_12[0xf];

    if (D_5893_map_id != 0x12)
    {
        // 01f3
        if (D_5893_map_id == 5)
        {
            ULTIMA_16ba_PrintChar('\"');
            ULTIMA_1850_PrintString(_TEXT(0x90a2, "Thou wilt give\nhalf thy gold to\ncharity!"));
            ULTIMA_16ba_PrintChar('\"');

            if (TALK_00ac_AskPay() != 0)
            {
                // 0216
                return 1;
            }

            // 021c
            D_57aa /= 2;

            // 0228
            ULTIMA_2900_UpdateVitalsDisplay();
            // 022b
            return 0;
        }
        else
        {
            // 0230
            local_16 = 0;

            // 0243 (NOT MATCHING: loop structure)
            for (local_14 = 0; local_14 < D_585b; local_14++)
            {
                // 025a
                if (D_55a8_party[local_14].status != STATUS_DEAD)
                {
                    local_16 += 10;
                }
            }

            // 026c
            ULTIMA_1850_PrintString(_TEXT(0x90cc, "A guard demands\na "));
            ULTIMA_1a3e_PrintNumber(local_16, 2, 0x20);
            ULTIMA_1850_PrintString(_TEXT(0x90e0, " gp tribute\nto Blackthorn!"));

            if (TALK_00ac_AskPay() != 0)
            {
                // 028d -> 0216
                return 1;
            }

            // 028f
            if (D_57aa >= local_16)
            {
                // 029a
                D_57aa -= local_16;

                ULTIMA_2900_UpdateVitalsDisplay();
                return 0;
            }

            // 0297 -> 0216
            return 1;
        }
    }
    else
    {
        // 02a4
        if (D_587a == 0x1d)
        {
            // 02ae
            ULTIMA_16ba_PrintChar('\"');
            ULTIMA_1850_PrintString(_TEXT(0x90fc, "Give now the\npassword, bearer\nof the Badge!"));
            ULTIMA_16ba_PrintChar('\"');
            ULTIMA_1850_PrintString(_TEXT(0x9128, "\n\nYour response?\n"));
            ULTIMA_3b1c_GetString(local_12, 0xe);
            ULTIMA_16ba_PrintChar('\n');
            local_12[4] = 0;

            if (TALK_0000_CompareStringNoCase(_TEXT(0x4a9a, "IMPE"), local_12) != 0) // TODO: D_4a9a = "IMPE"?
            {
                // 02f2
                ULTIMA_16ba_PrintChar('\n');
                ULTIMA_16ba_PrintChar('\"');
                ULTIMA_1850_PrintString(_TEXT(0x913a, "Pass, friend!"));
                ULTIMA_16ba_PrintChar('\"');
                ULTIMA_16ba_PrintChar('\n');
                // 0315 -> 022b
                return 0;
            }
        }

        // 02ef -> 0216
        return 1;
    }
}

// CHECKED
// talk to npc
int TALK_031e_TalkToNpc(int param_1)
{
    int local_4;
    NpcFmt* local_6;
    NpcScheduleFmt* local_8;

    ULTIMA_16ba_PrintChar('\n');

    D_bcdc = param_1;
    local_6 = &D_5f5e[param_1];
    local_8 = &D_5d5e[param_1];

    if (local_8->type[local_6->_e] == 4)
    {
        local_8->type[local_6->_e] = 1;
    }
    // c2ea NOT MATCHING
    else if (D_5c5a[local_6->_c]._0_tile == TILE_ACTOR_GUARD && ((local_6->_e & 1) == 0 || (local_6->_a) == 0))
    {
        ULTIMA_1850_PrintString(_TEXT(0x9148, "The guard offers\nno response!\n"));
        return 0;
    }

    local_4 = local_6->_a;
    if (local_4 == 0)
    {
        ULTIMA_1850_PrintString(_TEXT(0x9168, "No response!\n"));
        return 0;
    }
    else
    {
        // c316
        if (local_4 < 0x80)
        {
            TALK_127e_StartTalk(local_4);
            return 0;
        }

        if (local_4 == 0xfd)
        {
            ULTIMA_16ba_PrintChar('\"');
            ULTIMA_1850_PrintString(_TEXT(0x9176, "Don't hurt me!\nPlease go away!"));
            ULTIMA_16ba_PrintChar('\"');
            ULTIMA_16ba_PrintChar('\n');
            return 0;
        }

        // c34c
        if (local_4 == 0xfe)
        {
#if !defined(MATCHING_BUILD)
            TOWN_10da(param_1); // TODO: validate param (reference apple?)
#else
            TOWN_10da(); // BUG: TOWN_10da(unaff_SI)
#endif
            return 0;
        }

        if (local_4 == 0xff)
        {
            return TALK_01e2();
        }

        if ((local_6->_e & 1) != 0 && (NPC_12e0(D_bcdc, D_587f) & 1) != 0)
        {
            TALK_00e6_TalkToMerchant(local_4);
            return 0;
        }
        else
        {
            ULTIMA_1850_PrintString(_TEXT(0x9196, "A merchant says:\n\"Come see me at\nmy shoppe, "));
            ULTIMA_1850_PrintString(_TEXT(0x91c4, "when\nit's open!\"\n"));
            return 0;
        }
    }
}

int TALK_041c_TalkCmd(void)
{
    int local_a;
    int local_8;
    int local_c;
    int local_4;
    int local_6;

    if (ULTIMA_SelectTalkDirection() == 0)
    {
        return 0;
    }

    local_8 = D_5876;
    local_c = D_5878;
    if (TALK_Target(local_8,local_c,&local_6,&local_a) == 0)
    {
        ULTIMA_1850_PrintString(_TEXT(0x91d6, "\nNobody's here!\n"));
    }
    else
    {
        local_4 = TOWN_011e(D_5876);
        switch (*ULTIMA_4402_GetTileAddr(local_6, local_a))
        {
        case TILE_MAP_BED:
            ULTIMA_1850_PrintString(_TEXT(0x91e8, "\n\"Zzzzzz...\"\n"));
            break;
        case TILE_MAP_MIRROR:
            ULTIMA_1850_PrintString(_TEXT(0x91f6, "\nNo response!\n"));
            break;
        default:
            return TALK_031e_TalkToNpc(local_4);
        }
    }

    return 0;
}

static ImperaTalk nativeTalk(void);
static int TALK_0f32_ProcessChar(byte ch) {
    ImperaTalk s = nativeTalk();
    return IMPERA_TalkCharacter(&s, ch);
}
static void TALK_04da_PrintQuoteMark(void) { TALK_0f32_ProcessChar(0xa2); }
static void TALK_04d2_PrintCr(void) { TALK_0f32_ProcessChar(0x8d); }
static void TALK_04e2_PrintWord(void) {
    ImperaTalk s = nativeTalk();
    IMPERA_TalkFlush(&s);
}
static void TALK_0574_BuildOrPrintWord(byte ch) {
    ImperaTalk s = nativeTalk();
    IMPERA_TalkAppend(&s, ch);
}
static void TALK_075a_SelectNthTalkString(int entry) {
    ImperaTalk s = nativeTalk();
    IMPERA_TalkSelect(&s, entry);
}
static int TALK_111c(void) {
    ImperaTalk s = nativeTalk();
    return IMPERA_TalkDescription(&s);
}
static int TALK_0b04(void) {
    ImperaTalk s = nativeTalk();
    return IMPERA_TalkTopics(&s);
}
static int TALK_0a3c(void) {
    ImperaTalk s = nativeTalk();
    return IMPERA_TalkFarewell(&s);
}
static void TALK_0682(byte param_1)
{
    if (param_1 < 0x40)
    {
        ULTIMA_3ef0_IncreaseByte(&D_57c0[param_1], 1, 99);
        return;
    }

    switch (param_1)
    {
    case 0x41:
        ULTIMA_3f14_IncreaseInt(&D_57a8, 1, 9999);
        ULTIMA_2900_UpdateVitalsDisplay();
        break;
    case 0x42:
        ULTIMA_3f14_IncreaseInt(&D_57aa, 1, 9999);
        ULTIMA_2900_UpdateVitalsDisplay();
        break;
    case 0x43:
        ULTIMA_3ef0_IncreaseByte(&D_57ac, 1, 99);
        break;
    case 0x44:
        ULTIMA_3ef0_IncreaseByte(&D_57ad, 1, 99);
        break;
    case 0x45:
        ULTIMA_3ef0_IncreaseByte(&D_57ae, 1, 99);
        break;
    case 0x46:
        ULTIMA_3ef0_IncreaseByte(&D_57af, 1, 99);
        break;
    case 0x47:
        ULTIMA_3ef0_IncreaseByte(&D_57b0, 1, 99);
        break;
    case 0x48:
        D_57bc = 0xff;
        break;
    case 0x49:
        D_57ba = 0xff;
        break;
    case 0x4a:
        D_57be = 0xff;
        break;
    case 0x4b:
        ULTIMA_3ef0_IncreaseByte(&D_57b1, 1, 99);
        break;
    }
}
static int TALK_080a_Join(void)
{
    int local_30; // 30..2f
    byte* local_2e;   // 2e..2d
    char local_2c[4]; // 2c..29
    S_55a8 local_28;  // 28..09 (size: 0x20)
    char local_8[4];  // 08..05
    int local_4;      // 04..03

    local_2e = D_bcde;
    local_4 = 0xf;

    if (D_585b == 6)
    {
        ULTIMA_266c_GetChar();
        ULTIMA_1850_PrintString(_TEXT(0x9348, "\"Thou hast no room for me in thy party! "));
        ULTIMA_1850_PrintString(_TEXT(0x9372, "Seek me again if one of thy members doth leave\nthee."));

        return 0;
    }

    TALK_075a_SelectNthTalkString(0);

    local_30 = 0;

    // 3 bytes
    memcpy(local_8, D_bcde, 3);
    local_30 += 3;
    D_bcde += 3;

    local_8[3] = 0;
    local_2c[3] = 0;

    // c7e6
    while (1)
    {
        // 3 bytes
        memcpy(local_2c, D_55a8_party[local_4].name, 3);

        // c805
        if (TALK_0000_CompareStringNoCase(local_8, local_2c) != 0)
        {
            // local_30 = DI;
            break;
        }

        if (--local_4 == 0)
        {
            // c824
            TALK_0574_BuildOrPrintWord(0x22);
            TALK_0574_BuildOrPrintWord(10);
            ULTIMA_1850_PrintString(_TEXT(0x93a8, "\nSystem Error -\nNo Match!"));
            D_bcde = local_2e;
            // local_30 = DI;

            // -> c8b3
            return 1;
        }
    }

    // c848
    D_55a8_party[local_4].mapId = 0;

    local_28 = D_55a8_party[local_4];
    D_55a8_party[local_4] = D_55a8_party[D_585b];
    D_55a8_party[D_585b] = local_28;

    D_585b++;

    TOWN_0052_SetNpcKilled(D_bcdc);
    TOWN_00b0_DespawnNpc(D_bcdc);

    ULTIMA_2900_UpdateVitalsDisplay();
    TALK_04e2_PrintWord();

    D_bcde = local_2e;

    return 1;
}
static void talkPrint(void *host, uint16_t ch) {
    (void)host;
    ULTIMA_16ba_PrintChar(ch);
}
static void talkString(void *host, const char *str) {
    (void)host;
    ULTIMA_1850_PrintString((char *)str);
}
static const char *talkText(void *host, uint16_t address, const char *str) {
    (void)host;
    (void)address;
    return _TEXT(address, str);
}
static int talkColumn(void *host) {
    (void)host;
    return ULTIMA_1f12_GetCurrentTextX();
}
static void talkFont(void *host, int index) {
    (void)host;
    ULTIMA_1c9e_SelectCharset(index);
}
static int talkChar(void *host) {
    (void)host;
    return ULTIMA_266c_GetChar();
}
static void talkInput(void *host, char *str, int size) {
    (void)host;
    ULTIMA_3b1c_GetString(str, size);
}
static int talkPoll(void *host) {
    (void)host;
    return ULTIMA_1d5e_PollKey();
}
static void talkWait(void *host, int ticks) {
    (void)host;
    ULTIMA_20fa_WaitTicks(ticks);
}
static void talkFrame(void *host) {
    (void)host;
    ULTIMA_5910_UpdateFrame();
}
static void talkClear(void *host) {
    (void)host;
    ULTIMA_1b16_ClearKbdBuffer();
}
static int talkIntroduced(void *host) {
    (void)host;
    uint32_t flags;
    memcpy(&flags, &D_5bda[(D_5893_map_id - 1) * 4], 4);
    return (flags & ((uint32_t)1 << D_bcdc)) != 0;
}
static void talkIntroduce(void *host) {
    (void)host;
    uint32_t flags;
    byte *p = &D_5bda[(D_5893_map_id - 1) * 4];
    memcpy(&flags, p, 4);
    flags |= (uint32_t)1 << D_bcdc;
    memcpy(p, &flags, 4);
}
static int talkParty(void *host) {
    (void)host;
    return D_585b;
}
static const uint8_t *talkName(void *host, int index) {
    (void)host;
    return D_55a8_party[index].name;
}
static int talkKarma(void *host) {
    (void)host;
    return D_5888;
}
static void talkChangeKarma(void *host, int amount) {
    (void)host;
    if (amount > 0)
        ULTIMA_3ef0_IncreaseByte(&D_5888, amount, 99);
    else
        ULTIMA_3f36_DecreaseByte(&D_5888, -amount);
}
static int talkPay(void *host, int amount) {
    (void)host;
    if (D_57aa < amount)
        return 1;
    D_57aa -= amount;
    ULTIMA_2900_UpdateVitalsDisplay();
    if ((D_5c5a[D_5f5e[D_bcdc]._c]._0_tile & 0xfc) == TILE_ACTOR_BEGGAR && D_588b >= 100) {
        D_588b = 0;
        ULTIMA_3ef0_IncreaseByte(&D_5888, 1, 99);
        if (!D_57aa)
            ULTIMA_3ef0_IncreaseByte(&D_5888, 2, 99);
    }
    return 0;
}
static void talkGive(void *host, uint8_t code) {
    (void)host;
    TALK_0682(code);
}
static int talkJoin(void *host) {
    (void)host;
    return TALK_080a_Join();
}
static void talkGuards(void *host) {
    (void)host;
    TOWN_0958();
}
static int talkGreeting(void *host) {
    (void)host;
    ULTIMA_207e_srand(ULTIMA_2056_GetTime());
    return ULTIMA_2092_RandomRange(0, 1);
}
/* Observe source bytes separately from rendered game text. */
static void talkObserve(void *host, const uint8_t *cursor) {
    (void)host;
    (void)cursor;
#if defined(IMPERA_REFERENCE_OBSERVER)
    REFERENCE_TraceOpcode(cursor);
#endif
}
static ImperaTalk nativeTalk(void) {
    ImperaTalk s = {0};
    s.data = D_b21e;
    s.size = 1024;
    s.cursor = &D_bcde;
    s.operands = D_bce0;
    s.pending = &D_4aee;
    s.operandCount = &D_4aef;
    s.mask = &D_4af0;
    s.length = &D_4af1;
    s.questionMode = &D_4af2;
    s.lastColumn = &D_4af3;
    s.lastLiteral = &D_4af4;
    s.dictionaryBoundary = &D_4af5;
    s.label = &D_bcf4;
    s.topic = &D_bcf6;
    s.word = D_bce4;
    s.answer = D_bcf8;
    s.dictionary = (const char *const *)D_24ea;
    s.keywords = (const char *const *)D_4aa8;
    s.text = talkText;
#if defined(MATCHING_BUILD)
    s.matching = 1;
#endif
    s.print = talkPrint;
    s.string = talkString;
    s.column = talkColumn;
    s.font = talkFont;
    s.getChar = talkChar;
    s.getString = talkInput;
    s.poll = talkPoll;
    s.waitTicks = talkWait;
    s.frame = talkFrame;
    s.clearKeys = talkClear;
    s.introduced = talkIntroduced;
    s.introduce = talkIntroduce;
    s.partySize = talkParty;
    s.name = talkName;
    s.karma = talkKarma;
    s.changeKarma = talkChangeKarma;
    s.pay = talkPay;
    s.give = talkGive;
    s.join = talkJoin;
    s.guards = talkGuards;
    s.greetingRoll = talkGreeting;
    s.observe = talkObserve;
    return s;
}

static void TALK_1180(void)
{
    int local_4;

    if (D_5958 == 0)
    {
        ULTIMA_1850_PrintString(_TEXT(0x94dc, "\nSomething was stolen!\n"));
        ULTIMA_43ae_AudioSweepTone(800, 2000, 1, 0x32);
        ULTIMA_207e_srand(ULTIMA_2056_GetTime());

        if ((D_57ac | D_57ad | D_57ae) != 0)
        {
            do
            {
                while (1)
                {
                    switch (ULTIMA_2092_RandomRange(0, 2))
                    {
                    case 0:
                        if (D_57ac != 0)
                        {
                            ULTIMA_3f36_DecreaseByte(&D_57ac, 1);
                            return;
                        }
                        break;

                    case 1:
                        if (D_57ad != 0)
                        {
                            ULTIMA_3f36_DecreaseByte(&D_57ad, 1);
                            return;
                        }
                        break;

                    case 2:
                        if (D_57ae != 0)
                        {
                            ULTIMA_3f36_DecreaseByte(&D_57ae, 1);
                            return;
                        }
                        break;
                    default:
                        return;
                    }
                }
            } while (D_57ad == 0);

            ULTIMA_3f36_DecreaseByte(&D_57ad, 1); // BUG?
        }
        else
        {
            for (local_4 = 0x2f; local_4 >= 0; local_4--)
            {
                if (D_57c0[local_4] != 0)
                {
                    ULTIMA_3f36_DecreaseByte(&D_57c0[local_4], 1);
                    return;
                }
            }

            for (local_4 = 7; local_4 >= 0; local_4--)
            {
                if (D_5828[local_4] != 0)
                {
                    ULTIMA_3f36_DecreaseByte(&D_5828[local_4], 1);
                    return;
                }
            }

            for (local_4 = 7; local_4 >= 0; local_4--)
            {
                if (D_5820[local_4] != 0)
                {
                    ULTIMA_3f36_DecreaseByte(&D_5820[local_4], 1);
                    return;
                }
            }

            ULTIMA_3f54_DecreaseInt(&D_57aa, ULTIMA_2092_RandomRange(1, 0xf));
            ULTIMA_2900_UpdateVitalsDisplay();
        }
    }
}

static void TALK_127e_StartTalk(int param_1)
{
    uint local_4;
    s16* local_6;
    s16 local_8;
    s16 local_a;
    s16 local_c;

    local_4 = (D_5893_map_id - 1) >> 3;
    ULTIMA_256e_ReadFileFromDisk(D_4aa0[local_4], D_b21e, 0x200, 0);

    // 12a9
    local_6 = (s16*)D_b21e;
    local_c = *local_6++;

    // 12ad
    for (local_a = 1; *local_6++ != param_1 && local_a <= local_c; local_6++, local_a++)
    {
    }

    // 12d6
    local_8 = *local_6;
    ULTIMA_256e_ReadFileFromDisk(D_4aa0[local_4], D_b21e, 0x400, local_8);

    ImperaTalk session = nativeTalk();
    IMPERA_TalkRun(&session);

    TALK_1180();
}
