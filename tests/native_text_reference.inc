/* Frozen from df25c19: independent pre-extraction text oracle. */
// ASM
void ULTIMA_16ba_PrintChar(uint ch)
{
    int iVar4;
    byte bVar6;
    int uVar7;
    TextWindow* text_window;

    bVar6 = ch;
    text_window = D_539a_currentTextWindow;
    if (ch > 0x7f)
    {
        if (ch == CTRL_CHAR_CLEAR_WINDOW) // clear text window
        {
            int ax, bx, cx, dx;

            text_window->currentX = 0;
            text_window->currentY = 0;
            DRV_2d((text_window->colors >> 4) & 0xf);
            ULTIMA_1f77_ConvertCharCoordToPixel(text_window, &ax, &bx, &cx, &dx);
            DRV_3f(ax, bx, cx, dx, 0);
            DRV_2d(D_52da_pen_color);
            return;
        }
        if (ch == CTRL_CHAR_TOGGLE_UNDERLINE) // toggle underline
        {
            D_53a4_underline ^= 1;
            text_window->flags ^= 1; // underline
            return;
        }
        if (ch == CTRL_CHAR_TOGGLE_INVERSION) // toggle inversion
        {
            D_53a8_inverse ^= 1;
            text_window->flags ^= 4; // inverse
            return;
        }
        if (ch == CTRL_CHAR_CENTER_TEXT) // center text
        {
            D_53a6 = 1;
            text_window->flags |= 2;
            return;
        }
        if (ch == CTRL_CHAR_UNCENTER_TEXT) // uncenter text
        {
            D_53a6 = 0;
            text_window->flags &= 0xfd;
            return;
        }
        ch &= 0x7f;
    }

    if (ch != 0xa)
    {
        if (ch == 0xd)
        {
            goto LAB_1000_1745;
        }

        // Render Character
        uVar7 = bVar6 * 8;
        if (D_52ba_vdp._52c8_videoDriverSelection == VDP_VIDEO_HERC)
        {
            uVar7 = bVar6 * 24;
        }
        ULTIMA_17f4_ApplyCharEffects(D_5398_currentCharset, uVar7);

        // es = 5398
        // di = uVar7
        // dl = [D_53aa_text_fg_color]
        // dh = [D_53ab_text_bg_color]
        // al = [text_window + current_x] + [text_window + left]
        // ah = 0
        // bl = [text_window + current_y] + [text_window + top]
        // bh = 0
        DRV_5d(D_5398_currentCharset, uVar7, D_53aa_text_fg_color, D_53ab_text_bg_color,
               text_window->currentX + text_window->left, text_window->currentY + text_window->top);

        iVar4 = 4;
        if (D_52ba_vdp._52c8_videoDriverSelection == VDP_VIDEO_HERC)
        {
            iVar4 = 12;
        }

        memcpy(D_5398_currentCharset + uVar7, D_53ea, iVar4 * 2);

        text_window = D_539a_currentTextWindow;
        if (D_538e == 0)
        {
            return;
        }

        text_window->currentX++;
        if ((char)(text_window->currentX + text_window->left) <= text_window->right)
        {
            return;
        }
    }

    text_window->currentY++;
#if defined(ENABLE_SPEED_FIX)
    TIME_SleepMs(10);
#endif

LAB_1000_1745:
    text_window->currentX = 0;
    if (text_window->bottom < text_window->currentY + text_window->top)
    {
        int ax, bx, cx, dx;
        ULTIMA_1f77_ConvertCharCoordToPixel(text_window, &ax, &bx, &cx, &dx);
        text_window->currentY--;
#if defined(ENABLE_SPEED_FIX)
        TIME_SleepMs(10);
#endif
        DRV_27(ax, bx, cx, dx, -8);
    }
}

// ASM
// ptr = (es:)di
static void ULTIMA_17f4_ApplyCharEffects(byte* es, int di)
{
    int size;
    byte* ptr = es + di;

    size = 4;
    if (D_52ba_vdp._52c8_videoDriverSelection == VDP_VIDEO_HERC)
    {
        size = 12;
    }

    memcpy(D_53ea, ptr, size * 2);

    if (D_53a4_underline != 0)
    {
        if (D_52ba_vdp._52c8_videoDriverSelection == VDP_VIDEO_HERC)
        {
            // herc
            *(u16*)&ptr[0x16] = 0xffff;
        }
        else
        {
            // ega/cga/t1k
            ptr[7] = 0xff;
        }
    }

    if (D_53a8_inverse != 0)
    {
        size = 4;
        if (D_52ba_vdp._52c8_videoDriverSelection == VDP_VIDEO_HERC)
        {
            // herc
            size = 12;
        }

        size *= 2;
        for (; size > 0; size--)
        {
            *ptr = ~*ptr;
            ptr++;
        }
    }
}


