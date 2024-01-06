#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xft/Xft.h>

#include "./xftfont.h"

#define HEIGHT 20
#define WIDTH 10
#define NSIDE 4


#define TryMove(try, iffail) \
    (try);                   \
    if (TestShape())         \
        (iffail)

Atom done = false;
Atom paused = false;
Atom thread_running = false;

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

typedef struct {
    Display *dpy;
    Colormap cmap;
    Window win;
    XSetWindowAttributes swa;
    Screen* screen;
    Visual* vis;
    int scr;
    int x, y;
    unsigned int width, height, depth;
} XWindow;

typedef struct {
    int x, y;
    unsigned int width, height, spacing, sidelength;
    XRectangle r[WIDTH + 2][HEIGHT + 2];
    TetrisColor tc[WIDTH][HEIGHT];
} TetrisMap;

typedef struct {
    int x, y;
    unsigned int width, height, spacing, sidelength;
    XRectangle r[6][6];
    TetrisColor tc[4][4];
} TetrisNext;

const int shapewidths[7] = { 2, 4, 3, 3, 3, 3, 3 };

const int shapes[7][4][4][2] = {
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

// Globals
XWindow xw;
TetrisMap tm;
TetrisNext tn;
int root;
Drawable buf;
GC gc;
XGCValues xgc;
Window r;
unsigned int bw;
pthread_t updatethread;
int curshape, nextshape, rotation, xpos, ypos;
unsigned int lines = 0, score = 0, highscore = 0;
unsigned char level = 1;
double speed = 1.0;
XftFontDraw* xftfontdraw;
XftFontColor* xftfontcolor;
XftFontFont* xftfontfont;

unsigned long colors[] = {
    0xFFFF00,
    0x00EDFF,
    0xBF00FF,
    0xFF9000,
    0x0000FF,
    0x00FF00,
    0xFF0000,
    0x5b5b5b,
    0x000000,
};

double pow(double base, double exp) {
    if (exp <= 0) return 1;
    return base * pow(base, exp - 1);
}

void xinit() {
    XWindowAttributes wa;
    XVisualInfo vis;

    xw.dpy = XOpenDisplay(NULL); if (!xw.dpy) {
        perror("Could not open display");
        exit(1);
    }
    root = DefaultRootWindow(xw.dpy);
    xw.scr = DefaultScreen(xw.dpy);
    xw.screen = ScreenOfDisplay(xw.dpy, xw.scr);

    XGetWindowAttributes(xw.dpy, root, &wa);
    xw.depth = wa.depth;

    XMatchVisualInfo(xw.dpy, xw.scr, xw.depth, TrueColor, &vis);
    xw.vis = vis.visual;

    xw.cmap = XCreateColormap(xw.dpy, root, xw.vis, None);

    xw.swa.bit_gravity = NorthWestGravity;
    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask | FocusChangeMask;

    xw.width = 400;
    xw.height = 400;
    xw.x = xw.y = 0;


    xw.win = XCreateWindow(xw.dpy, root, xw.x, xw.y, xw.width, xw.height,
        0, xw.depth, InputOutput, xw.vis,
        CWBitGravity | CWColormap | CWEventMask, &xw.swa);

    XStoreName(xw.dpy, xw.win, "Tetris");
}

void xsetupgc() {
    while (1) {
        XEvent ev;
        XNextEvent(xw.dpy, &ev);
        if (ev.type == MapNotify)
          break;
    }

    XGetGeometry(xw.dpy, xw.win, &r, &xw.x, &xw.y, &xw.width, &xw.height, &bw,
                 &xw.depth);

    buf = XCreatePixmap(xw.dpy, xw.win, xw.width, xw.height, xw.depth);

    XSetWindowBackground(xw.dpy, xw.win, 0x000000);
    XClearWindow(xw.dpy, xw.win);

    xgc.background = 0x00000000;
    xgc.foreground = 0xFFFFFFFF;
    xgc.fill_style = FillSolid;
    xgc.graphics_exposures = False;
    gc = XCreateGC(xw.dpy, buf, GCFillStyle | GCBackground | GCForeground | GCGraphicsExposures, &xgc);

    XSetForeground(xw.dpy, gc, colors[Empty]);
    XFillRectangle(xw.dpy, buf, gc, 0, 0, xw.width, xw.height);
}

void ClearTetrisMap() {
    for (int i = 0; i < WIDTH; i++) 
        for (int j = 0; j < HEIGHT; j++) 
            tm.tc[i][j] = Empty;
}

void ClearTetrisNext() {
    for (int i = 0; i < NSIDE; i++)
        for (int j = 0; j < NSIDE; j++)
            tn.tc[i][j] = Empty;
}

void CreateTetrisMap(int x, int y, unsigned int sidelength, unsigned int spacing) {
    tm.spacing = spacing;
    tm.x = x;
    tm.y = y;
    tm.sidelength = sidelength;
    unsigned int tmp = (tm.sidelength + tm.spacing);
    tm.width = ((WIDTH + 2) * tmp) - tm.spacing;
    tm.height = ((HEIGHT + 2) * tmp) - tm.spacing;

    for (int i = 0; i < WIDTH + 2; i++) {
        for (int j = 0; j < HEIGHT + 2; j++) {
            tm.r[i][j].width = tm.r[i][j].height = tm.sidelength;
            tm.r[i][j].x = x + tmp * i;
            tm.r[i][j].y = y + tmp * j;
        }
    }

    ClearTetrisMap();
}

void CreateTetrisNext(int x, int y, unsigned int sidelength, unsigned int spacing) {
    tn.spacing = spacing;
    tn.x = x;
    tn.y = y;
    tn.sidelength = sidelength;
    unsigned int tmp = (tn.sidelength + tn.spacing);
    tn.width = ((NSIDE + 2) * tmp) - tn.spacing;
    tn.height = ((NSIDE + 2) * tmp) - tn.spacing;

    for (int i = 0; i < NSIDE + 2; i++) {
        for (int j = 0; j < NSIDE + 2; j++) {
            tn.r[i][j].width = tn.r[i][j].height = tn.sidelength;
            tn.r[i][j].x = x + tmp * i;
            tn.r[i][j].y = y + tmp * j;
        }
    }

    ClearTetrisNext();
}


bool TestShape() {
    int x, y;
    for (int i = 0; i < 4; i++) {
        x = xpos + shapes[curshape][rotation][i][0];
        y = ypos + shapes[curshape][rotation][i][1];
        if (x >= WIDTH || y >= HEIGHT || tm.tc[x][y] != Empty || x < 0 || y < 0) {
            return true;
        }
    }
    return false;
}

void ClearShape() {
    for (int i = 0; i < 4; i++) {
        tm.tc[xpos + shapes[curshape][rotation][i][0]]
            [ypos + shapes[curshape][rotation][i][1]] = Empty;
    }
}

void DrawShape() {
    for (int i = 0; i < 4; i++) {
        tm.tc[xpos + shapes[curshape][rotation][i][0]]
            [ypos + shapes[curshape][rotation][i][1]] = curshape;
    }
}


void DrawNextShape() {
    for (int i = 0; i < 4; i++) {
        tn.tc[shapes[nextshape][0][i][0]]
            [shapes[nextshape][0][i][1]] = nextshape;
    }
}

void MoveDown() {
    TryMove(ypos++, ypos--);
    score++;
}

void InstaDrop() {
    int sy = ypos;
    while (!TestShape()) {
        ypos++;
    }
    ypos--;
    score += 2 * (ypos - sy);
}

void MoveLeft() {
    TryMove(xpos--, xpos++);
}

void RotateClockwise() {
    TryMove(rotation = (rotation + 1) % 4, rotation = (rotation + 3) % 4);
}

void RotateCounterClockwise() {
    TryMove(rotation = (rotation + 3) % 4, rotation = (rotation + 1) % 4);
}

void MoveRight() {
    TryMove(xpos++, xpos--);
}


bool FullRow(int y) {
    for (int i = 0; i < WIDTH; i++) {
        if (tm.tc[i][y] == Empty)
            return false;
    }
    lines++;
    level = 1 + lines / 10;
    speed = pow((0.8 - ((level - 1) * 0.007)), (level - 1));
    return true;
}

void ShiftDown() {
    unsigned int l = lines;
    for (int i = 0; i < HEIGHT; i++) {
        if (FullRow(i)) {
            for (int j = i; j != 1; j--) {
                for (int k = 0; k < WIDTH; k++) {
                    tm.tc[k][j] = tm.tc[k][j - 1];
                }
            }
        }
    }
    switch ((lines - l)) {
    case 1: score += 100*level; break;
    case 2: score += 300*level; break;
    case 3: score += 500*level; break;
    case 4: score += 800*level; break;
    }
}

void NewShape() {
    ClearTetrisNext();
    ShiftDown();
    curshape = nextshape;
    nextshape = rand() % 7;
    xpos = (WIDTH - shapewidths[curshape]) / 2;
    ypos = 0;
    rotation = 0;
    if (TestShape()) {
        done = true;
        return;
    }
    DrawShape();
    DrawNextShape();
}

void HandleTextDrawing() {
    xftfont_draw_clear(xftfontdraw);

    int height = xftfontdraw->font->font->height;
    int yoffset = tn.height + tn.y;
    int xoffset = tn.x;

    xftfont_color_set(xftfontdraw, xftfontcolor);
    
    xftfont_draw_text(xftfontdraw, xoffset, yoffset,             "Score: %u",      score);
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Level: %u",      (unsigned int)level);
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Lines: %u",      lines);
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "High Score: %u", highscore);
#ifdef TETRIS_DEBUG
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Speed %f",       speed);
#endif
}


void DisplayScreen() {

    HandleTextDrawing();
    
    XSetForeground(xw.dpy, gc, colors[Wall]);
    XFillRectangle(xw.dpy, buf, gc, tm.x, tm.y, tm.width, tm.height);
    XSetForeground(xw.dpy, gc, colors[Wall]);
    XFillRectangles(xw.dpy, buf, gc, (XRectangle*)tm.r, (WIDTH+2)*(HEIGHT+2));
    XFillRectangles(xw.dpy, buf, gc, (XRectangle*)tn.r, (NSIDE+2)*(NSIDE+2));


    XRectangle* tmp;
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            XSetForeground(xw.dpy, gc, colors[tm.tc[i][j]]);
            tmp = &tm.r[i+1][j+1];
            XFillRectangle(xw.dpy, buf, gc, tmp->x, tmp->y, tmp->width, tmp->height);

            if (i < NSIDE && j < NSIDE) {
                XSetForeground(xw.dpy, gc, colors[tn.tc[i][j]]);
                tmp = &tn.r[i+1][j+1];
                XFillRectangle(xw.dpy, buf, gc, tmp->x, tmp->y, tmp->width, tmp->height);
            }
        } 
    }

    XCopyArea(xw.dpy, buf, xw.win, gc, 0, 0, xw.width, xw.height, 0, 0);
    XFlush(xw.dpy);
}

void ResetGame() {
    done = false;
    if (score > highscore) highscore = score;
    speed = 1.0;
    score = 0;
    level = 1;
    lines = 0;
    ClearTetrisMap();
    nextshape = rand() % 7;
    NewShape();
}

void *UpdateGame(void *arg) {
    thread_running = true;
    while (!done) {
        usleep(1000000 * speed);
        ClearShape();
        ypos++;
        bool hit = TestShape();
        if (hit)
            ypos--;

        DrawShape();
        if (hit)
            NewShape();

        DisplayScreen();
    }
    thread_running = false;
    return NULL;
}

int main() {

    srand(time(NULL));

    xinit();

    XMapWindow(xw.dpy, xw.win);
    XSync(xw.dpy, False);

    xsetupgc();
    xftfontdraw = xftfont_draw_create(xw.dpy, buf, xw.vis, xw.cmap, xw.scr, xw.width, xw.height);
    xftfontfont = xftfont_font_create(xftfontdraw, "Monospace-10");
    if (!xftfontfont) {
        perror("Could not find font");
        return 1;
    }
    xftfont_font_set(xftfontdraw, xftfontfont);
    xftfontcolor = xftfont_color_create(xftfontdraw, 0xffff, 0xffff, 0xffff, 0xffff);

    CreateTetrisMap(20, 20, 30, 1);
    CreateTetrisNext(tm.width + tm.x + 30, 20, 30, 1);

    nextshape = rand() % 7;

    NewShape();

    DisplayScreen();

    pthread_create(&updatethread, NULL, UpdateGame, NULL);

    XEvent ev;
    while (!done) {
        XNextEvent(xw.dpy, &ev);
        if (ev.type == KeyPress) {
            XKeyEvent *e = &ev.xkey;
           KeySym ksym = NoSymbol;
            char buf[64];
            XLookupString(e, buf, sizeof buf, &ksym, NULL);
            if (ksym == XK_q) {
                done = true;
                continue;
            }
            else if (ksym == XK_Escape) {
                paused = false;
                if (!thread_running)
                    pthread_create(&updatethread, NULL, UpdateGame, NULL);
            }
            else if (ksym == XK_F1) {
                paused = true;
                if (thread_running) {
                    pthread_cancel(updatethread);
                    thread_running = false;
                }
            }
            else if (ksym == XK_r) {
                ResetGame();
                if (!thread_running)
                    pthread_create(&updatethread, NULL, UpdateGame, NULL);
            }
            if (!paused) {
                ClearShape();
                if (ksym == XK_Down)
                    MoveDown();
                else if (ksym == XK_Left)
                    MoveLeft();
                else if (ksym == XK_Right)
                    MoveRight();
                else if (ksym == XK_Up || ksym == XK_x)
                    RotateClockwise();
                else if (ksym == XK_z || ksym == XK_Control_L || ksym == XK_Control_R)
                    RotateCounterClockwise();
                else if (ksym == XK_space)
                    InstaDrop();
                DrawShape();
            }
        } else if (ev.type == Expose) {
            XGetGeometry(xw.dpy, xw.win, &r, &xw.x, &xw.y, &xw.width,
                            &xw.height, &bw, &xw.depth);
        } else if (ev.type == FocusIn) {
            paused = false;
            if (!thread_running)
                pthread_create(&updatethread, NULL, UpdateGame, NULL);
        } else if (ev.type == FocusOut) {
            paused = true;
            if (thread_running) {
                pthread_cancel(updatethread);
                thread_running = false;
            }
        }
        DisplayScreen();
    }
    pthread_cancel(updatethread);


    XFreePixmap(xw.dpy, buf);
    XFreeGC(xw.dpy, gc);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}
