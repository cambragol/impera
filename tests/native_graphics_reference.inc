/* Frozen from df25c19: independent pre-extraction raster/scroll oracle. */
void GRAP_BUF_PrintChar(byte* ptr, int offset, byte fgColor, byte bgColor, int penX, int penY)
{
    byte* p = &ptr[offset];
    for (int y = 0; y < 8; y++)
    {
        byte b = p[y];
        for (int x = 0; x < 8; x++)
        {
            byte col = b & s_bitMask[x] ? s_colorTable[fgColor] : s_colorTable[bgColor];

            GrPutPixel(D_52ba_vdp._52d8_page, penX * 8 + x, penY * 8 + y + s_textPixelOffset, col);
        }
    }

    s_dirty = true;
}

// 0x27
void GRAP_BUF_ScrollWindow(int left, int top, int right, int bottom, int amount)
{
    if (amount < 0)
    {
        amount = -amount;
        for (int y = top; y <= bottom - amount; y++)
        {
            TILESET_Copy(&g_linearEgaBuffer0[y * loresWidth + left], &g_linearEgaBuffer0[(y + amount) * loresWidth + left], right - left + 1);
            memcpy(&g_linearEgaBuffer0[y * loresWidth + left], &g_linearEgaBuffer0[(y + amount) * loresWidth + left], right - left + 1);
        }

        for (int y = bottom - amount + 1; y <= bottom; y++)
        {
            TILESET_Copy(&g_linearEgaBuffer0[y * loresWidth + left], NULL, right-left+1);
            memset(&g_linearEgaBuffer0[y * loresWidth + left], 0, right - left + 1);
        }
    }
    else
    {
        for (int y = bottom; y >= top + amount; y--)
        {
            TILESET_Copy(&g_linearEgaBuffer0[y * loresWidth + left], &g_linearEgaBuffer0[(y - amount) * loresWidth + left], right-left+1);
            memcpy(&g_linearEgaBuffer0[y * loresWidth + left], &g_linearEgaBuffer0[(y - amount) * loresWidth + left], right - left + 1);
        }

        for (int y = top; y < top + amount; y++)
        {
            TILESET_Copy(&g_linearEgaBuffer0[y * loresWidth + left], NULL, right-left+1);
            memset(&g_linearEgaBuffer0[y * loresWidth + left], 0, right - left + 1);
        }
    }

    s_dirty = true;
}


