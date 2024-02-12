#include "./tetris.h"

int tmindex(int x, int y, int width) {
    return x + y * width;
}

void tetris_clear_map(TetrisMap *tm) {
    for (int i = 0; i < tm->size.x; i++) 
        for (int j = 0; j < tm->size.y; j++) 
            tm->tc[tmindex(i, j, tm->size.x)] = Empty;
}

bool tetris_test_shape(TetrisMap* tm) {
    int x, y;
    for (int i = 0; i < 4; i++) {
        x = tm->x + shapes[tm->shape][tm->rotation][i][0];
        y = tm->y + shapes[tm->shape][tm->rotation][i][1];
        if (x >= tm->size.x ||
            y >= tm->size.y ||
            tm->tc[tmindex(x, y, tm->size.x)] != Empty ||
            x < 0 ||
            y < 0) {
            return true;
        }
    }
    return false;
}

void tetris_clear_shape(TetrisMap* tm) {
    for (int i = 0; i < 4; i++) {
        tm->tc[tmindex(tm->x + shapes[tm->shape][tm->rotation][i][0],
                tm->y + shapes[tm->shape][tm->rotation][i][1], tm->size.x)] = Empty;
    }
}

void tetris_draw_shape(TetrisMap* tm) {
    for (int i = 0; i < 4; i++) {
        tm->tc[tmindex(tm->x + shapes[tm->shape][tm->rotation][i][0],
                tm->y + shapes[tm->shape][tm->rotation][i][1], tm->size.x)] = tm->shape;
    }
}

bool tetris_move_down(TetrisMap* tm) {
    tm->y++;
    if (tetris_test_shape(tm)) {
        tm->y--;
        return false;
    }
    return true;
    
}

void tetris_move_left(TetrisMap* tm) {
    tm->x--;
    if (tetris_test_shape(tm)) {
        tm->x++;
    }
}

void tetris_move_right(TetrisMap* tm) {
    tm->x++;
    if (tetris_test_shape(tm)) {
        tm->x--;
    }
}

uint32_t tetris_insta_drop(TetrisMap* tm) {
    int sy = tm->y;
    while (!tetris_test_shape(tm)) {
        tm->y++;
    }
    tm->y--;
    return 2 * (tm->y - sy);
}

void tetris_rotate_clockwise(TetrisMap* tm) {
    tm->rotation = (tm->rotation + 1) % 4;
    if (tetris_test_shape(tm))
        tm->rotation = (tm->rotation + 3) % 4;
}

void tetris_rotate_countercw(TetrisMap* tm) {
    tm->rotation = (tm->rotation + 3) % 4;
    if (tetris_test_shape(tm))
        tm->rotation = (tm->rotation + 1) % 4;
    
}


void tetris_create_map(TetrisMap* tm, Vec2f pos, Vec2i size, unsigned int sidelength, unsigned int spacing) {
    tm->r = malloc(sizeof(XRectangle) * (size.x + 2) * (size.y + 2));
    if (!tm->r) {
        printf("couldn't allocate memory for XRectangles\n");
        exit(1);
    }
    tm->tc = malloc(sizeof(TetrisColor) * size.x * size.y);
    if (!tm->r) {
        printf("couldn't allocate memory for TetrisColor\n");
        exit(1);
    }
    tm->rotation = 0;
    tm->spacing = spacing;
    tm->sidelength = sidelength;
    tm->position = pos;
    tm->size = size;
    tm->x = tm->y = 0;
    unsigned int tmp = (tm->sidelength + tm->spacing);
    tm->pxsize = vec2f(
        ((size.x + 2) * tmp) - tm->spacing,
        ((size.y + 2) * tmp) - tm->spacing);
    for (int i = 0; i < tm->size.x + 2; i++) {
        for (int j = 0; j < tm->size.y + 2; j++) {
            int t = tmindex(i, j, tm->size.x + 2);
            tm->r[t].width = tm->r[t].height = tm->sidelength;
            tm->r[t].x = pos.x + tmp * i;
            tm->r[t].y = pos.y + tmp * j;
        }
    }
    tetris_clear_map(tm);
}
