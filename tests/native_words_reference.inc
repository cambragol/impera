/* Frozen from df25c19: independent pre-extraction word oracle. */
// CHECKED
static void TALK_04e2_PrintWord(void)
{
    byte local_6;
    int local_4;

    local_4 = 0;

    // c469
    if (D_4af1 != 0)
    {
        while (local_4 < D_4af1) // c4c9
        {
            // c4d5 -> c47c
            ULTIMA_1c9e_SelectCharset((D_bce4[local_4] & 0x80) == 0);
            local_6 = D_bce4[local_4] & 0x7f;
            local_4++;

            if (ULTIMA_1f12_GetCurrentTextX() != 0 || local_6 != 0x20)
            {
                // c49a
                if (D_4af3 < 0xf || local_6 != 10)
                {
                    // c4a7
                    if (local_6 != 10)
                    {
                        D_4af3 = ULTIMA_1f12_GetCurrentTextX();
                    }

                    // c4b3
                    ULTIMA_16ba_PrintChar(local_6);
                }
                else if (local_6 == 10) // c4be
                {
                    D_4af3 = 0;
                }
            }
        }

        // c4e0
        D_4af1 = 0;
        ULTIMA_1c9e_SelectCharset(0);
    }
}

static void TALK_0574_BuildOrPrintWord(byte param_1)
{
#if !defined(MATCHING_BUILD)
    /* Keep the incoming character when a long literal fills the buffer. */
    if (D_4af1 == sizeof(D_bce4)) TALK_04e2_PrintWord();
#endif
    if (D_4af1 != 0x10)
    {
        D_bce4[D_4af1++] = param_1;

        if (param_1 != 0x8a && param_1 != 0xa0) // 0xa / 0x20
        {
            return;
        }

        if (ULTIMA_1f12_GetCurrentTextX() + (uint)D_4af1 >= 0x12)
        {
            ULTIMA_16ba_PrintChar('\n');
        }
    }

    TALK_04e2_PrintWord();
}


