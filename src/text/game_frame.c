#include "game_frame.h"
void IMPERA_DrawGameFrame(const ImperaGameFrame *s) {
    s->fill(s->host, 0, 0, 0, 319, 199);
    s->fill(s->host, s->blue, 0, 0, 319, 6);
    s->fill(s->host, s->blue, 0, 185, 191, 191);
    s->fill(s->host, s->blue, 0, 0, 6, 191);
    s->fill(s->host, s->blue, 185, 0, 191, 191);
    s->fill(s->host, s->blue, 313, 0, 319, 87);
    s->fill(s->host, s->blue, 192, 80, 312, 87);
    s->fill(s->host, s->blue, 192, 57, 312, 63);
    s->corner(s->host, s->blue, 0, 0, 0x7b);
    s->corner(s->host, s->blue, 39, 0, 0x7c);
    s->corner(s->host, s->blue, 0, 23, 0x7d);
    s->line(s->host, s->white, 7, 7, 7, 184);
    s->line(s->host, s->white, 7, 184, 184, 184);
    s->line(s->host, s->white, 184, 184, 184, 7);
    s->line(s->host, s->white, 184, 7, 7, 7);
    s->line(s->host, s->white, 191, 191, 191, 87);
    s->line(s->host, s->white, 191, 87, 319, 87);
    s->line(s->host, s->white, 191, 7, 312, 7);
    s->line(s->host, s->white, 312, 7, 312, 56);
    s->line(s->host, s->white, 312, 56, 191, 56);
    s->line(s->host, s->white, 191, 56, 191, 7);
    s->line(s->host, s->white, 191, 63, 312, 63);
    s->line(s->host, s->white, 312, 63, 312, 80);
    s->line(s->host, s->white, 312, 80, 191, 80);
    s->line(s->host, s->white, 191, 80, 191, 63);
}
