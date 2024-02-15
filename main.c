/* #define XLIB_RENDER */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xft/Xft.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <GL/glew.h>
#include <GL/glxew.h>

#include "./tetris.h"
#include "./shader.h"
#include "./la.h"
#include "./xftfont.h"
#include "./render.h"
#include "./free_glyph.h"

Atom done = false;
Atom paused = false;
Atom thread_running = false;

typedef struct {
    Display *dpy;
    Colormap cmap;
    Window win;
    XSetWindowAttributes swa;
    Screen* screen;
    Visual* vis;
    XVisualInfo* vi;
    int scr;
    int x, y;
    unsigned int width, height, depth;
} XWindow;

typedef enum {
    GAME_PAUSED = 0,
    GAME_RUNNING,
    GAME_ENDED,
} GameState;

// Globals
XWindow xw;
TetrisMap tm;
TetrisMap tn;
int root;
Drawable buf;
GC gc;
XGCValues xgc;
GameState state = GAME_RUNNING;
Window r;
unsigned int bw;
pthread_t updatethread;
unsigned int lines = 0, score = 0, highscore = 0;
unsigned char level = 1;
double speed = 1.0;
XftFontDraw* xftfontdraw;
XftFontColor* xftfontcolor;
XftFontFont* xftfontfont;

Render glr = {0};
Glyph_Atlas atlas = {0};

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

    XMapWindow(xw.dpy, xw.win);
    XSync(xw.dpy, False);
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


bool FullRow(int y) {
    for (int i = 0; i < tm.size.x; i++) {
        if (tm.tc[tmindex(i, y, tm.size.x)] == Empty)
            return false;
    }
    lines++;
    level = 1 + lines / 10;
    speed = pow((0.8 - ((level - 1) * 0.007)), (level - 1));
    return true;
}

void ShiftDown() {
    unsigned int l = lines;
    for (int i = 0; i < tm.size.y; i++) {
        if (FullRow(i)) {
            for (int j = i; j != 1; j--) {
                for (int k = 0; k < tm.size.x; k++) {
                    tm.tc[tmindex(k, j, tm.size.x)] = tm.tc[tmindex(k, j - 1, tm.size.x)];
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
    tetris_clear_map(&tn);
    ShiftDown();
    tm.shape = tn.shape;
    tn.shape = rand() % 7;
    tm.x = (tm.size.x - shapewidths[tm.shape]) / 2;
    tm.y = 0;
    tm.rotation = 0;
    if (tetris_test_shape(&tm)) {
        state = GAME_ENDED;
        return;
    }
    tetris_draw_shape(&tm);
    tetris_draw_shape(&tn);
}

void HandleTextDrawing() {
    xftfont_draw_clear(xftfontdraw);

    int height = xftfontdraw->font->font->height;
    int yoffset = tn.pxsize.y + tn.position.y;
    int xoffset = tn.position.x;

    xftfont_color_set(xftfontdraw, xftfontcolor);
    
    xftfont_draw_text(xftfontdraw, xoffset, yoffset,             "High Score: %u", highscore);
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Score:      %u", score);
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Level: %u",      (unsigned int)level);
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Lines: %u",      lines);
#ifdef TETRIS_DEBUG
    xftfont_draw_text(xftfontdraw, xoffset, (yoffset += height), "Speed %f",       speed);
#endif
}

void DisplayScreen() {
#ifdef XLIB_RENDER
    HandleTextDrawing();
    
    XSetForeground(xw.dpy, gc, colors[Wall]);
    XFillRectangle(xw.dpy, buf, gc, tm.position.x, tm.position.y, tm.size.x, tm.size.y);
    XFillRectangles(xw.dpy, buf, gc, (XRectangle*)tm.r, (tm.size.x + 2)*(tm.size.y + 2));
    XFillRectangles(xw.dpy, buf, gc, (XRectangle*)tn.r, (tn.size.x + 2)*(tn.size.y + 2));
    
    XRectangle* tmp;
    for (int i = 0; i < tm.size.x; i++) {
        for (int j = 0; j < tm.size.y; j++) {
            XSetForeground(xw.dpy, gc, colors[tm.tc[tmindex(i, j, tm.size.x)]]);
            tmp = &tm.r[tmindex(i + 1 , j + 1, tm.size.x + 2)];
            XFillRectangle(xw.dpy, buf, gc, tmp->x, tmp->y, tmp->width, tmp->height);

            if (i < tn.size.x && j < tn.size.y) {
                XSetForeground(xw.dpy, gc, colors[tn.tc[tmindex(i, j, tn.size.x)]]);
                tmp = &tn.r[tmindex(i+1, j+1, tn.size.x + 2)];
                XFillRectangle(xw.dpy, buf, gc, tmp->x, tmp->y, tmp->width, tmp->height);
            }
        }
    }

    XCopyArea(xw.dpy, buf, xw.win, gc, 0, 0, xw.width, xw.height, 0, 0);
    XFlush(xw.dpy);
#else //XLIB_RENDER
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = (double)tv.tv_usec/1000000.0f;
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //boxes
    {
        gl_use_shader(&glr, SHADER_BLOCK);
        glUniform2f(glr.resolution_uniform, xw.width, xw.height);
        glUniform1f(glr.time_uniform, time);
        gl_draw_tetris(&glr, &tm);
        gl_draw_tetris(&glr, &tn);
        gl_sync(&glr);
        glDrawArrays(GL_TRIANGLES, 0, glr.buffer_count);
        gl_clear(&glr);
    }

    //strings
    {
        float h = atlas.height;
        Vec2f pos = vec2f(tn.position.x, tn.pxsize.y + tn.position.y);
        Vec4f color = vec4f(0.99, 0.05, 0.97, 1.0);
        /* Vec4f color = vec4f(1.0, 1.0, 1.0, 1.0); */
        gl_use_shader(&glr, SHADER_TEXT);
        glUniform2f(glr.resolution_uniform, xw.width, xw.height);
        glUniform1f(glr.time_uniform, time);
        free_glyph_draw_text(&atlas, &glr, pos, color, "High Score: %u", highscore);
        pos.y += h;
        free_glyph_draw_text(&atlas, &glr, pos, color, "Score:      %u", score);
        pos.y += h;
        free_glyph_draw_text(&atlas, &glr, pos, color, "Level: %u", (unsigned int)level);
        pos.y += h;
        free_glyph_draw_text(&atlas, &glr, pos, color, "Lines: %u", lines);
#ifdef TETRIS_DEBUG
        pos.y += h;
        free_glyph_draw_text(&atlas, &glr, pos, color, "Speed: %f", speed);
#endif // TETRIS_DEBUG
        gl_sync(&glr);
        glDrawArrays(GL_TRIANGLES, 0, glr.buffer_count);
        gl_clear(&glr);
    }
    
    glXSwapBuffers(xw.dpy, xw.win);
#endif //XLIB_RENDER
}

void ResetGame() {
    state = GAME_RUNNING;
    if (score > highscore) highscore = score;
    speed = 1.0;
    score = 0;
    level = 1;
    lines = 0;
    tetris_clear_map(&tm);
    tn.shape = rand() % 7;
    NewShape();
}

void glxinit() {
    xw.dpy = XOpenDisplay(NULL);
    if (!xw.dpy) {
        perror("no display");
        exit(1);
    }
    root = DefaultRootWindow(xw.dpy);
    
    int att[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None,
    };

    XVisualInfo* vi = glXChooseVisual(xw.dpy, 0, att);
    xw.vi = vi;
    xw.vis = vi->visual;
    xw.depth = vi->depth;
    xw.cmap = XCreateColormap(xw.dpy, root, xw.vis, None);

    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask | FocusChangeMask;

    xw.width = 400;
    xw.height = 400;
    xw.x = xw.y = 0;

    xw.win = XCreateWindow(xw.dpy, root, xw.x, xw.y, xw.width, xw.height,
        0, xw.depth, InputOutput, xw.vis,
        CWEventMask | CWColormap, &xw.swa);
    
    XMapWindow(xw.dpy, xw.win);
    XStoreName(xw.dpy, xw.win, "Tetris");
}

int main() {

    srand(time(NULL));
#ifdef XLIB_RENDER
    xinit();
    xsetupgc();
    xftfontdraw = xftfont_draw_create(xw.dpy, buf, xw.vis, xw.cmap, xw.scr, xw.width, xw.height);
    xftfontfont = xftfont_font_create(xftfontdraw, "Monospace-10");
    if (!xftfontfont) {
        perror("Could not find font");
        return 1;
    }
    xftfont_font_set(xftfontdraw, xftfontfont);
    xftfontcolor = xftfont_color_create(xftfontdraw, 0xffff, 0xffff, 0xffff, 0xffff);
#else // XLIB_RENDER
    glxinit();

    gl_render_init(&glr, xw.dpy, xw.win, xw.vi, "main.vert", "boxes.frag", "text.frag");

    FT_Library library;
    FT_Face face;
    if (FT_Init_FreeType(&library)) {
        printf("FreeType failed to initalize\n");
        exit(1);
    }
    if (FT_New_Face(library, "/usr/share/fonts/dejavu/DejaVuSansMono.ttf", 0, &face)) {
        printf("FreeType font failed to inititalize\n");
        exit(1);
    }
    if (FT_Set_Pixel_Sizes(face, 0, 24)) {
        printf("Could not set pixel size\n");
        exit(1);
    }
#ifdef TETRIS_DEBUG
    int OpenGLVersion[2];
	glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);
    printf("OpenGL Verision %d.%d\n", OpenGLVersion[0], OpenGLVersion[1]);
#endif //TETRIS_DEBUG
    glyph_atlas_init(&atlas, face);
#endif // XLIB_RENDER

    tetris_create_map(&tm, vec2f(20, 20), vec2i(10, 20), 30, 1);
    tetris_create_map(&tn, vec2f(tm.pxsize.x + tm.position.x + 30, 20), vec2i(4, 4), 30, 1);

    tn.shape = rand() % 7;
    NewShape();
    DisplayScreen();

    struct timeval tvstart, tvmovestart;
    gettimeofday(&tvstart, NULL);
    gettimeofday(&tvmovestart, NULL);
    
    XEvent ev;
    while (!done) {
        tetris_clear_shape(&tm);
        if (XPending(xw.dpy)) {
            XNextEvent(xw.dpy, &ev);
            switch (ev.type) {
            case KeyPress: {
                XKeyEvent *e = &ev.xkey;
                KeySym ksym = NoSymbol;
                char buf[64];
                XLookupString(e, buf, sizeof buf, &ksym, NULL);
                if (ksym == XK_q) {
                    done = true;
                    continue;
                }
                else if (ksym == XK_r) 
                    ResetGame();
                switch (state) {
                case GAME_RUNNING: {
                    switch (ksym) {
                    case XK_F1: {
                        state = GAME_PAUSED;
                    } break;
                    case XK_Down:
                        score += tetris_move_down(&tm); break;
                    case XK_Left:
                        tetris_move_left(&tm); break;
                    case XK_Right:
                        tetris_move_right(&tm); break;
                    case XK_Up:
                    case XK_x:
                        tetris_rotate_clockwise(&tm); break;
                    case XK_z:
                    case XK_Control_L:
                    case XK_Control_R:
                        tetris_rotate_countercw(&tm); break;
                    case XK_space:
                        score += tetris_insta_drop(&tm); break;

                    }
                } break;
                case GAME_PAUSED:
                    if (ksym == XK_Escape) {
                        state = GAME_RUNNING;
                    }
                case GAME_ENDED:
                }
                

            } break;
            case Expose: {
                if (!ev.xexpose.count) {
#ifndef XLIB_RENDER
                    glViewport(ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
#endif // XLIB_RENDER
                    xw.width = ev.xexpose.width;
                    xw.height = ev.xexpose.height;
                }
            } break;
            case FocusIn: {
                if (state == GAME_PAUSED)
                    state = GAME_RUNNING;
            } break;
            case FocusOut: {
                if (state == GAME_RUNNING) 
                    state = GAME_PAUSED;
            } break;
            }
        }
        tetris_draw_shape(&tm);
        if (state == GAME_RUNNING) {
            struct timeval tvend, tvelapsed;
            gettimeofday(&tvend, NULL);
            timersub(&tvend, &tvmovestart, &tvelapsed);
            double elapsed = (double)tvelapsed.tv_sec + (double)tvelapsed.tv_usec/1000000.0f;
            if (elapsed >= speed) {
                gettimeofday(&tvmovestart, NULL);
                tetris_clear_shape(&tm);
                tm.y++;
                bool hit = tetris_test_shape(&tm);
                if (hit)
                    tm.y--;
                tetris_draw_shape(&tm);
                if (hit)
                    NewShape();
            }
        }
        {
            struct timeval tvend, tvelapsed;
            gettimeofday(&tvend, NULL);
            timersub(&tvend, &tvstart, &tvelapsed);
            double elapsed = (double)tvelapsed.tv_sec + (double)tvelapsed.tv_usec/1000000.0f;
            if (elapsed >= 1.0f / 60.0f) {
                gettimeofday(&tvstart, NULL);
                DisplayScreen();
            }
        }
    }

#ifdef XLIB_RENDER
    XFreePixmap(xw.dpy, buf);
    XFreeGC(xw.dpy, gc);
#else // XLIB_RENDER
    glXMakeCurrent(xw.dpy, 0, 0);
    glXDestroyContext(xw.dpy, glr.glc);
#endif // XLIB_RENDER
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}
