#include "common/common.h"
#include "funcs.h"
#include "vars.h"
#include "text_input.h"
static void print(void *host, uint16_t ch) {
    (void)host;
    ULTIMA_16ba_PrintChar(ch);
}
static void backspace(void *host, int n) {
    (void)host;
    ULTIMA_1fa0_Backspace(n);
}
static int poll(void *host) {
    (void)host;
    return ULTIMA_1d5e_PollKey();
}
static int readChar(void *host) {
    (void)host;
    return ULTIMA_266c_GetChar();
}
static void waitTicks(void *host, int n) {
    (void)host;
    ULTIMA_20fa_WaitTicks(n);
}
static void frame(void *host) {
    (void)host;
    ULTIMA_5910_UpdateFrame();
}
ImperaInput IMPERA_EngineInput(void) {
    ImperaInput s = {0};
    s.advance = &D_538e;
    s.cycle = &D_540c;
    s.numpad = &D_538a;
    s.buffering = &D_538c;
    s.cursorBase = D_5390;
    s.cursorCount = D_5392;
    s.map = D_5893_map_id;
    s.driver = D_52c8;
    s.animate = &D_5891;
    s.print = print;
    s.backspace = backspace;
    s.poll = poll;
    s.waitTicks = waitTicks;
    s.frame = frame;
    s.readChar = readChar;
    return s;
}
