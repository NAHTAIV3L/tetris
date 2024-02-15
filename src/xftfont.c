#include <stdbool.h>
#include <stdarg.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "./xftfont.h"

#define BUFFER_SIZE 255

static char ascii_printable[] =
	" !\"#$%&'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmnopqrstuvwxyz{|}~";


XftFontDraw* xftfont_draw_create(Display* dpy, Drawable buf, Visual* vis, Colormap cmap, int screen, int w, int h) {
    XftFontDraw* f = malloc(sizeof(XftFontDraw));
    f->d = XftDrawCreate(dpy, buf, vis, cmap);
    f->dpy = dpy;
    f->vis = vis;
    f->cmap = cmap;
    f->scr = screen;
    f->width = w;
    f->height = h;
    
    return f;
}

void xftfont_draw_free(XftFontDraw* f) {
    XftDrawDestroy(f->d);
    free(f);
}

void xftfont_make_glyph_spec(XftFontDraw* f, Rune* r, XftGlyphFontSpec* gfs, int len, int x, int y) {
    float  xp, yp;
    Rune rune;
    FT_UInt glyphidx;
    int numspecs = 0;
    
    xp = x;
    yp = y + f->font->font->ascent;
    
    for (int i = 0; i < len; i++) {
        rune = r[i];

        glyphidx = XftCharIndex(f->dpy, f->font->font, rune);
        if (glyphidx) {
            gfs[numspecs].font = f->font->font;
            gfs[numspecs].glyph = glyphidx;
            gfs[numspecs].x = (short)xp;
            gfs[numspecs].y = (short)yp;
            xp += f->font->fontwidth;
            numspecs++;
            continue;
        }
    }
}

void xftfont_draw_clear(XftFontDraw* f) {
    XRenderColor gb = {
         .alpha = 0xffff,
         .red = 0x0000,
         .blue = 0x0000,
         .green = 0x0000
    };
    XftColor bg;
    XftColorAllocValue(f->dpy, f->vis, f->cmap, &gb, &bg);
    XftDrawRect(f->d, &bg, 0, 0, f->width, f->height);
    XftColorFree(f->dpy, f->vis, f->cmap, &bg);
}

void xftfont_draw_glyph_spec(XftFontDraw* f, XftGlyphFontSpec* gfs, int len) {
    XRectangle r = {0};

    r.x = 0;
    r.y = 0;
    r.height = f->height;
    r.width = f->width;
    XftDrawSetClipRectangles(f->d, 0, 0, &r, 1);
    
    XftDrawGlyphFontSpec(f->d, &f->fc->xfc, gfs, len);
}

void xftfont_draw_text(XftFontDraw* f, int x, int y, const char* fmt, ...) {
    char buf[BUFFER_SIZE] = {0};
    Rune runes[BUFFER_SIZE] = {0};
    XftGlyphFontSpec gfs[BUFFER_SIZE] = {0};

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, BUFFER_SIZE, fmt, args);
    va_end(args);
    
    int len = strlen(buf);
    for (int i = 0; i < len; i++) {
        runes[i] = buf[i];
    }

    xftfont_make_glyph_spec(f, runes, gfs, len, x, y);
    xftfont_draw_glyph_spec(f, gfs, len);
}

XftFontFont* xftfont_font_create(XftFontDraw *f, const char *fontname) {
    double fontsize;
    FcPattern* pattern = FcNameParse((const FcChar8*)fontname);
    FcPattern* match;
    FcResult result;
    XftFontFont* font = malloc(sizeof(XftFontFont));

    if (!font) {
        perror("Could not allocate font");
        exit(1);
    }
    
    if (!pattern) return NULL;

    if (FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &fontsize)
        == FcResultMatch) {}
    else if (FcPatternGetDouble(pattern, FC_SIZE, 0, &fontsize)
        == FcResultMatch) {}
    else {
        FcPatternAddDouble(pattern, FC_PIXEL_SIZE, 12);
        fontsize = 12;
    }

    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    XftDefaultSubstitute(f->dpy, f->scr, pattern);

    match = FcFontMatch(NULL, pattern, &result);
    if (!match) {
        FcPatternDestroy(pattern);
        return NULL;
    }    

    if (!(font->font = XftFontOpenPattern(f->dpy, match))) {
        FcPatternDestroy(pattern);
        FcPatternDestroy(match);
        return NULL;
    }

    XftTextExtentsUtf8(f->dpy, font->font, (const FcChar8 *)ascii_printable,
        sizeof(ascii_printable), &font->extents);
    font->fontwidth = (((font->extents.xOff) + ((strlen(ascii_printable)) - 1)) / (strlen(ascii_printable)));
    return font;
}

void xftfont_font_free(XftFontFont* font) {
    free(font);
}

void xftfont_font_set(XftFontDraw* f, XftFontFont* font) {
    f->font = font;
}

XftFontColor* xftfont_color_create(XftFontDraw *f, uint16_t r, uint16_t g, uint16_t b, uint16_t a) {
    XftFontColor* fc = malloc(sizeof(XftFontColor));
    if (!fc) {
        perror("Failed to create XftFontColor");
        exit(1);
    }

    fc->rc.alpha = a;
    fc->rc.red   = r;
    fc->rc.green = g;
    fc->rc.blue  = b;

    XftColorAllocValue(f->dpy, f->vis, f->cmap, &fc->rc, &fc->xfc);
    return fc;
}

void xftfont_color_free(XftFontDraw *f, XftFontColor *fc) {
    XftColorFree(f->dpy, f->vis, f->cmap, &fc->xfc);
    free(fc);
}

void xftfont_color_set(XftFontDraw *f, XftFontColor *fc) {
    f->fc = fc;
}
