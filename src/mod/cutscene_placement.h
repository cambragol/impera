#ifndef IMPERA_CUTSCENE_PLACEMENT_H
#define IMPERA_CUTSCENE_PLACEMENT_H
typedef struct U5CutscenePlacement {
    int custom;
    int imageX, imageY;
    int left[2], right[2], top, bottom, x, y, secondY;
} U5CutscenePlacement;
static int U5_ValidCutscenePlacement(const U5CutscenePlacement *p) {
    if (p->custom != 0 && p->custom != 1)
        return 0;
    if (!p->custom)
        return 1;
    return p->imageX >= 0 && p->imageX < 320 && p->imageY >= 0 && p->imageY < 200 &&
           p->left[0] >= 0 && p->right[0] <= 320 && p->left[0] < p->right[0] && p->left[1] >= 0 &&
           p->right[1] <= 320 && p->left[1] < p->right[1] && p->top >= 0 && p->top <= p->bottom &&
           p->bottom <= 200 && p->x >= 0 && p->x < 320 && p->y >= 0 && p->y < 192 &&
           p->secondY >= 0 && p->secondY < 192;
}
#endif
