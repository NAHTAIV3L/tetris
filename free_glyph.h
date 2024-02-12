#ifndef FREE_GLYPH_H_
#define FREE_GLYPH_H_

#include <GL/glew.h>
#include <GL/glxew.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "./render.h"

typedef struct {
    float ax;
    float ay;

    float bw;
    float bh;
    
    float bl;
    float bt;

    float tx;
} Metric;

#define GLYPH_METRIC_CAP 128
#define BUFFER_LEN 128

typedef struct {
    FT_UInt width;
    FT_UInt height;
    GLuint glyph_texture;
    FT_Int mby;
    Metric metrics[GLYPH_METRIC_CAP];
} Glyph_Atlas;

void glyph_atlas_init(Glyph_Atlas *atlas, FT_Face face);
void free_glyph_atlas_render_line_sized(Glyph_Atlas *atlas, Render *glr, const char *text, size_t text_size, Vec2f pos, Vec4f color);
void free_glyph_draw_text(Glyph_Atlas* atlas, Render* glr, Vec2f pos, Vec4f color, const char* fmt, ...);
#endif // FREE_GLYPH_H_

