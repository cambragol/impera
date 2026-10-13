int ULTIMA_6f1e(byte* param_1, char* param_2)
{
    byte bVar1;
    int iVar2;
    int iVar3;
    int local_18;
    char local_14[10];

    local_18 = -1;
    memset(local_14, 0, 10);

    for (iVar2 = 0; iVar2 < 9 && (iVar3 = iVar2, param_1[iVar2] != 0); iVar2++)
    {
        bVar1 = param_1[iVar2] & 0x7f;
        if ((DAT_000738d8[bVar1] & 2) != 0)
        {
            bVar1 = (param_1[iVar2] & 0x7fU) - 0x20;
        }
        local_14[iVar2] = (char)bVar1;
    }

    iVar2 = strncmp(local_14, param_2, iVar3);
    if (iVar2 == 0)
    {
        local_18 = 0;
    }

    return local_18;
}


