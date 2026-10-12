// draw frame
void ULTIMA_637e_DrawFrame(void)
{
    int local_4; // unused
 
    // black
    ULTIMA_0a70_GRAP_2d_SetPenColor(0);
    ULTIMA_0aa6_GRAP_3f_FillRect(0, 0, 319, 199);

    ULTIMA_0a70_GRAP_2d_SetPenColor(D_13b2_blueColor);
    ULTIMA_0aa6_GRAP_3f_FillRect(0, 0, 319, 6);
    ULTIMA_0aa6_GRAP_3f_FillRect(0, 185, 191, 191);
    ULTIMA_0aa6_GRAP_3f_FillRect(0, 0, 6, 191);
    ULTIMA_0aa6_GRAP_3f_FillRect(185, 0, 191, 191);
    ULTIMA_0aa6_GRAP_3f_FillRect(313, 0, 319, 87);
    ULTIMA_0aa6_GRAP_3f_FillRect(192, 80, 312, 87);
    ULTIMA_0aa6_GRAP_3f_FillRect(192, 0x39, 312, 63);

    ULTIMA_1cca_SetTextForegroundColor(D_13b2_blueColor);
    ULTIMA_1bf2_SetTextPosition(0, 0);
    ULTIMA_16ba_PrintChar(0x7b);
    ULTIMA_1bf2_SetTextPosition(39, 0);
    ULTIMA_16ba_PrintChar(0x7c);
    ULTIMA_1bf2_SetTextPosition(0, 23);
    ULTIMA_16ba_PrintChar(0x7d);

    ULTIMA_0a70_GRAP_2d_SetPenColor(D_13b0_brightWhiteColor);
    ULTIMA_0b10_GRAP_Line(7, 7, 7, 184);
    ULTIMA_0f90_GRAP_Pen(184, 184);
    ULTIMA_0f90_GRAP_Pen(184, 7);
    ULTIMA_0f90_GRAP_Pen(7, 7);
    ULTIMA_0b10_GRAP_Line(191, 191, 191, 87);
    ULTIMA_0f90_GRAP_Pen(319, 87);
    ULTIMA_0b10_GRAP_Line(191, 7, 312, 7);
    ULTIMA_0f90_GRAP_Pen(312, 56);
    ULTIMA_0f90_GRAP_Pen(191, 56);
    ULTIMA_0f90_GRAP_Pen(191, 7);
    ULTIMA_0b10_GRAP_Line(191, 63, 312, 63);
    ULTIMA_0f90_GRAP_Pen(312, 80);
    ULTIMA_0f90_GRAP_Pen(191, 80);
    ULTIMA_0f90_GRAP_Pen(191, 63);

    ULTIMA_1cca_SetTextForegroundColor(D_13b0_brightWhiteColor);
}

