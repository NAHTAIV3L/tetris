#ifndef XFTFONT_H
#define XFTFONT_H
#include <stdint.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>

typedef uint_least32_t Rune;

typedef struct{
    XftDraw* d;
    XGlyphInfo extents;
    XftFont* font;
    Display* dpy;
    Colormap cmap;
    Visual* vis;
    XftGlyphFontSpec* gfs;
    Rune* r;
    int scr, width, height;
    unsigned int fontwidth;
} XftFontDraw;

XftFontDraw* xftfont_create(Display* dpy, Drawable buf, Visual* vis, Colormap cmap, int screen, int w, int h);
void xftfont_free(XftFontDraw* f);
void xftfont_draw_text(XftFontDraw* f, const char* str, int x, int y);
void xftfont_clear(XftFontDraw* f);
bool xftfont_load_font(XftFontDraw *f, const char *fontname);
#endif
