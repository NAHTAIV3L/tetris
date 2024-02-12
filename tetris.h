#ifndef TETRIS_H_
#define TETRIS_H_

#include "./la.h"
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
    Yellow,
    LBlue,
    Purple,
    Orange,
    DBlue,
    Green,
    Red,
    Wall,
    Empty
} TetrisColor;

static const int shapes[7][4][4][2] = {
    //TO
    {{{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }},
     {{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }},
     {{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }},
     {{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }}},
    //TI
    {{{ 0,  0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }},
     {{ 1, -1 }, { 1, 0 }, { 1, 1 }, { 1, 2 }},
     {{ 0,  0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }},
     {{ 1, -1 }, { 1, 0 }, { 1, 1 }, { 1, 2 }}},
    //TT
    {{{ 1, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 }},
     {{ 1, 0 }, { 1, 1 }, { 2, 1 }, { 1, 2 }},
     {{ 0, 1 }, { 1, 1 }, { 2, 1 }, { 1, 2 }},
     {{ 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }}},
    //TL
    {{{ 0,  0 }, { 1,  0 }, { 2, 0 }, { 0, 1 }},
     {{ 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }},
     {{ 2, -1 }, { 0,  0 }, { 1, 0 }, { 2, 0 }},
     {{ 1, -1 }, { 1,  0 }, { 1, 1 }, { 2, 1 }}},
    //TJ
    {{{ 0,  0 }, { 1,  0 }, { 2, 0 }, { 2, 1 }},
     {{ 1, -1 }, { 1,  0 }, { 1, 1 }, { 0, 1 }},
     {{ 0, -1 }, { 0,  0 }, { 1, 0 }, { 2, 0 }},
     {{ 1, -1 }, { 2, -1 }, { 1, 0 }, { 1, 1 }}},
    //TS
    {{{ 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }},
     {{ 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }},
     {{ 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }},
     {{ 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }}},
    //TZ
    {{{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 }},
     {{ 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }},
     {{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 }},
     {{ 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }}}};

static const int shapewidths[7] = { 2, 4, 3, 3, 3, 3, 3 };

typedef struct {
    Vec2f position;
    Vec2f pxsize;
    Vec2i size;
    uint32_t spacing, sidelength;
    uint32_t shape;
    uint32_t x, y;
    XRectangle *r;
    TetrisColor *tc;
    unsigned int rotation;
} TetrisMap;

int tmindex(int x, int y, int width);
void tetris_clear_map(TetrisMap* tm);
bool tetris_test_shape(TetrisMap* tm);
void tetris_clear_shape(TetrisMap* tm);
void tetris_draw_shape(TetrisMap* tm);

bool tetris_move_down(TetrisMap* tm);
void tetris_move_left(TetrisMap* tm);
void tetris_move_right(TetrisMap* tm);
uint32_t tetris_insta_drop(TetrisMap* tm);
void tetris_rotate_clockwise(TetrisMap* tm);
void tetris_rotate_countercw(TetrisMap* tm);

void tetris_create_map(TetrisMap* tm, Vec2f pos, Vec2i size, uint32_t sidelength, uint32_t spacing);
#endif // TETRIS_H_
