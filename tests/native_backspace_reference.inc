/* Frozen from df25c19. */
void ULTIMA_1fa0_Backspace(int char_count)
{
    u16 local_4;
    TextWindow* local_6;
    int local_8;
    int local_a;

    if ((ULTIMA_1f12_GetCurrentTextX() != 0 || ULTIMA_1cee_GetCurrentTextY() != 0) && char_count != 0)
    {
        local_4 = D_538e;
        local_6 = &D_535e_textWindows[D_5386_current_text_window_idx];
        D_538e = 0;

        for (local_8 = 0; local_8 < char_count; local_8++)
        {
            local_a = ULTIMA_1f12_GetCurrentTextX();
            ULTIMA_16ba_PrintChar(' ');
            if (local_a != 0)
            {
                ULTIMA_1bf2_SetTextPosition(local_a - 1, ULTIMA_1cee_GetCurrentTextY());
            }
            else
            {
                ULTIMA_1bf2_SetTextPosition(local_6->right - local_6->left, ULTIMA_1cee_GetCurrentTextY() - 1);
            }
        }

        D_538e = local_4;
    }
}

