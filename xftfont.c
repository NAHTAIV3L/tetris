#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "./xftfont.h"

static char ascii_printable[] =
	" !\"#$%&'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmnopqrstuvwxyz{|}~";

XftFontDraw* xftfont_create(Display* dpy, Drawable buf, Visual* vis, Colormap cmap, int screen, int w, int h) {
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

void xftfont_free(XftFontDraw* f) {
    free(f);
}

bool xftfont_load_font(XftFontDraw *f, const char *fontname) {
    double fontsize;
    FcPattern* pattern = FcNameParse((const FcChar8*)fontname);
    FcPattern* match;
    FcResult result;

    if (!pattern) return false;

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
        return false;
    }    

    if (!(f->font = XftFontOpenPattern(f->dpy, match))) {
        FcPatternDestroy(pattern);
        FcPatternDestroy(match);
        return false;
    }

    XftTextExtentsUtf8(f->dpy, f->font, (const FcChar8 *)ascii_printable,
        sizeof(ascii_printable), &f->extents);
    f->fontwidth = (((f->extents.xOff) + ((strlen(ascii_printable)) - 1)) / (strlen(ascii_printable)));
    return true;
}

void xftfont_make_glyph_spec(XftFontDraw* f, int len, int x, int y) {
    float  xp, yp;
    Rune rune;
    FT_UInt glyphidx;
    int numspecs = 0;
    
    xp = x;
    yp = y + f->font->ascent;
    
    for (int i = 0; i < len; i++) {
        rune = f->r[i];

        glyphidx = XftCharIndex(f->dpy, f->font, rune);
        if (glyphidx) {
            f->gfs[numspecs].font = f->font;
            f->gfs[numspecs].glyph = glyphidx;
            f->gfs[numspecs].x = (short)xp;
            f->gfs[numspecs].y = (short)yp;
            xp += f->fontwidth;
            numspecs++;
            continue;
        }
    }
}

void xftfont_clear(XftFontDraw* f) {
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

void xftfont_draw_glyph_spec(XftFontDraw* f, int len) {
    XftColor fg;
    XRenderColor gf;
    XRectangle r;
    
    gf.alpha = 0xffff;
    gf.red = 0xffff;
    gf.blue = 0xffff;
    gf.green = 0xffff;
    XftColorAllocValue(f->dpy, f->vis, f->cmap, &gf, &fg);

    r.x = 0;
    r.y = 0;
    r.height = f->height;
    r.width = f->width;
    XftDrawSetClipRectangles(f->d, 0, 0, &r, 1);
    
    XftDrawGlyphFontSpec(f->d, &fg, f->gfs, len);
    XftColorFree(f->dpy, f->vis, f->cmap, &fg);
}

void xftfont_draw_text(XftFontDraw* f, const char* str, int x, int y) {
    int len = strlen(str);
    f->gfs = malloc(len*sizeof(XftGlyphFontSpec));
    f->r = malloc(len*sizeof(Rune));
    for (int i = 0; i < len; i++) {
        f->r[i] = str[i];
    }

    xftfont_make_glyph_spec(f, len, x, y);
    xftfont_draw_glyph_spec(f, len);
    free(f->gfs);
    free(f->r);
}
