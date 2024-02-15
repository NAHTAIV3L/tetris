#include "./free_glyph.h"


void glyph_atlas_init(Glyph_Atlas *atlas, FT_Face face) {
    FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF);
    /* FT_Int32 load_flags = FT_LOAD_RENDER; */
    for (int i = 32; i < 128; ++i) {
        if (FT_Load_Char(face, i, load_flags)) {
            fprintf(stderr, "ERROR: could not load glyph of a character with code %d\n", i);
            exit(1);
        }

        atlas->width += face->glyph->bitmap.width;
        if (atlas->height < face->glyph->bitmap.rows) {
            atlas->height = face->glyph->bitmap.rows;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &atlas->glyph_texture);
    glBindTexture(GL_TEXTURE_2D, atlas->glyph_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        (GLsizei) atlas->width,
        (GLsizei) atlas->height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        NULL);

    int x = 0;
    for (int i = 32; i < 128; ++i) {
        if (FT_Load_Char(face, i, load_flags)) {
            fprintf(stderr, "ERROR: could not load glyph of a character with code %d\n", i);
            exit(1);
        }

        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
            fprintf(stderr, "ERROR: could not render glyph of a character with code %d\n", i);
            exit(1);
        }

        atlas->metrics[i].ax = face->glyph->advance.x >> 6;
        atlas->metrics[i].ay = face->glyph->advance.y >> 6;
        atlas->metrics[i].bw = face->glyph->bitmap.width;
        atlas->metrics[i].bh = face->glyph->bitmap.rows;
        atlas->metrics[i].bl = face->glyph->bitmap_left;
        atlas->metrics[i].bt = face->glyph->bitmap_top;
        atlas->metrics[i].tx = (float) x / (float) atlas->width;
        if (atlas->mby < face->glyph->bitmap_top)
            atlas->mby = face->glyph->bitmap_top;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            x,
            0,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        x += face->glyph->bitmap.width;
    }
}

void free_glyph_atlas_render_line_sized(Glyph_Atlas *atlas, Render *glr, const char *text, size_t text_size, Vec2f pos, Vec4f color)
{
    for (size_t i = 0; i < text_size; ++i) {
        size_t glyph_index = text[i];
        // TODO: support for glyphs outside of ASCII range
        if (glyph_index >= GLYPH_METRIC_CAP) {
            glyph_index = '?';
        }
        Metric metric = atlas->metrics[glyph_index];
        float x2 = pos.x + metric.bl;
        float y2 = pos.y - metric.bt + atlas->mby;
        float w  = metric.bw;
        float h  = metric.bh;

        pos.x += metric.ax;
        pos.y += metric.ay;
        
        gl_render_img(
            glr,
            vec2f(x2, y2),
            vec2f(w, h),
            vec2f(metric.tx, 0.0f),
            vec2f(metric.bw / (float) atlas->width, metric.bh / (float) atlas->height),
            color);
    }
}

void free_glyph_draw_text(Glyph_Atlas * atlas, Render* glr, Vec2f pos, Vec4f color, const char* fmt, ...) {
    char buf[BUFFER_LEN] = {0};
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, BUFFER_LEN, fmt, args);
    va_end(args);

    free_glyph_atlas_render_line_sized(atlas, glr, buf, strlen(buf), pos, color);
}
