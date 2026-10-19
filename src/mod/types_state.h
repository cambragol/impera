#ifndef IMPERA_TYPES_STATE_H
#define IMPERA_TYPES_STATE_H
#include "types.h"
/* Internal atomic operation: validates both cells and reserves state capacity
 * before either identity changes. Both targets belong to the current surface. */
int TYPES_SwapCells(TypeCell a, TypeCell b, uint32_t fallbackA, uint32_t fallbackB);
#endif
