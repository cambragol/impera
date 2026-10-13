void ULTIMA_3b1c_GetString(char* param_1, int param_2)
{
    int local_8;
    int local_6;
    int local_4;

    local_4 = 0; // di
    local_8 = D_538c;
    D_538c = 0;
    do
    {
        local_6 = ULTIMA_266c_GetChar();
        if ((local_6 == U5_KEY_BACKSPACE || local_6 == U5_KEY_LEFT) && local_4 != 0)
        {
            // 3b43
            ULTIMA_1fa0_Backspace(1);
            local_4--;
        }
        else if ((local_6 == U5_KEY_ESC) && (local_4 != 0))
        {
            ULTIMA_1fa0_Backspace(local_4);
            local_4 = 0;
        }
        else if (0x1f < local_6 && local_6 < 0x80 && local_4 < param_2)
        {
            param_1[local_4++] = (char)local_6;
            ULTIMA_16ba_PrintChar(local_6);
        }
    } while (local_6 != U5_KEY_ENTER);
    param_1[local_4] = 0;
    D_538c = local_8;
}


