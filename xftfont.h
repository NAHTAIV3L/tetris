#ifndef XFTFONT_H
#define XFTFONT_H
#include <stdint.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>

typedef uint_least32_t Rune;

typedef struct {
    XRenderColor rc;
    XftColor xfc;
} XftFontColor;

typedef struct {
    XftFont* font;
    unsigned int fontwidth;
    XGlyphInfo extents;
} XftFontFont;

typedef struct{
    XftDraw* d;
    Display* dpy;
    Colormap cmap;
    Visual* vis;
    XftFontColor* fc;
    XftFontFont* font;
    int scr, width, height;
} XftFontDraw;

XftFontDraw* xftfont_draw_create(Display* dpy, Drawable buf, Visual* vis, Colormap cmap, int screen, int w, int h);
void xftfont_draw_free(XftFontDraw* f);
void xftfont_draw_text(XftFontDraw* f, int x, int y, const char* fmt, ...);
void xftfont_draw_clear(XftFontDraw* f);

XftFontFont* xftfont_font_create(XftFontDraw *f, const char *fontname);
void xftfont_font_free(XftFontFont* font);
void xftfont_font_set(XftFontDraw* f, XftFontFont* font);

XftFontColor* xftfont_color_create(XftFontDraw *f, uint16_t r, uint16_t g, uint16_t b, uint16_t a);
void xftfont_color_free(XftFontDraw *f, XftFontColor *fc);
void xftfont_color_set(XftFontDraw *f, XftFontColor *fc);
#endif
