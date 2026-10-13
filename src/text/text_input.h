#ifndef IMPERA_TEXT_INPUT_H
#define IMPERA_TEXT_INPUT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ImperaInput {
    uint16_t *advance, *cycle, *numpad, *buffering;
    uint16_t cursorBase, cursorCount;
    int map, driver;
    uint8_t *animate;
    void *host;
    int (*cancelled)(void *);
    void (*print)(void *, uint16_t);
    void (*backspace)(void *, int);
    int (*poll)(void *);
    int (*readChar)(void *);
    void (*waitTicks)(void *, int);
    void (*frame)(void *);
} ImperaInput;
int IMPERA_InputCursor(ImperaInput *);
int IMPERA_InputChar(ImperaInput *);
void IMPERA_InputString(ImperaInput *, char *, int);
ImperaInput IMPERA_EngineInput(void);
#ifdef __cplusplus
}
#endif
#endif
