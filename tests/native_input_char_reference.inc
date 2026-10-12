int ULTIMA_266c_GetChar(void)
{
    int local_4;

    if (D_52c8 == 2)
    {
        D_5891 = 0xff;
    }

    do
    {
        local_4 = ULTIMA_2032_ToUpper(ULTIMA_1b38_PollKeyWithCursor());
        if ((local_4 == 0) && (D_5893_map_id < 0x21 || 0x7f < D_5893_map_id))
        {
            ULTIMA_5910_UpdateFrame();
        }
    } while (local_4 == 0);

    if (D_538a != 0)
    {
        // numpad
        switch (local_4)
        {
        case 0x31:
            local_4 = U5_KEY_END;
            break;
        case 0x32:
            local_4 = U5_KEY_DOWN;
            break;
        case 0x33:
            local_4 = U5_KEY_PGDN;
            break;
        case 0x34:
            local_4 = U5_KEY_LEFT;
            break;
        case 0x36:
            local_4 = U5_KEY_RIGHT;
            break;
        case 0x37:
            local_4 = U5_KEY_HOME;
            break;
        case 0x38:
            local_4 = U5_KEY_UP;
            break;
        case 0x39:
            local_4 = U5_KEY_PGUP;
        }
    }
    else
    {
        if (local_4 < 5)
            local_4 += 0xfa;
    }

    return local_4;
}


