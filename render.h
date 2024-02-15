#ifndef RENDER_H_
#define RENDER_H_

#define BUFFER_SIZE 1024 * 10

#include "./tetris.h"
#include "./la.h"
#include "./shader.h"
#include <GL/glew.h>
#include <GL/glxew.h>

static const Vec4f gl_colors[] = {
    (Vec4f) { .x = 1.0f,  .y =  1.0f,  .z =  0.0f,  .w =  1.0f},
    (Vec4f) { .x = 0.0f,  .y =  0.9f,  .z =  1.0f,  .w =  1.0f},
    (Vec4f) { .x = 0.75f, .y =  0.0f,  .z =  1.0f,  .w =  1.0f},
    (Vec4f) { .x = 1.0f,  .y =  0.5f,  .z =  0.0f,  .w =  1.0f},
    (Vec4f) { .x = 0.0f,  .y =  0.0f,  .z =  1.0f,  .w =  1.0f},
    (Vec4f) { .x = 0.0f,  .y =  1.0f,  .z =  0.0f,  .w =  1.0f},
    (Vec4f) { .x = 1.0f,  .y =  0.0f,  .z =  0.0f,  .w =  1.0f},
    (Vec4f) { .x = 0.35f, .y =  0.35f, .z =  0.35f, .w =  1.0f},
    (Vec4f) { .x = 0.0f,  .y =  0.0f,  .z =  0.0f,  .w =  1.0f},
};

typedef enum {
    SHADER_BLOCK = 0,
    SHADER_TEXT,
    SHADER_SIZE,
} Shader;

typedef struct {
    Vec2f pos;
    Vec2f uv;
    Vec4f color;
} Vertex;

typedef struct {
    GLXContext glc;
    GLuint vao;
    GLuint vbo;

    Shader current_shader;
    GLuint program[SHADER_SIZE];

    GLint resolution_uniform;
    GLint time_uniform;

    size_t buffer_count;
    Vertex buffer[BUFFER_SIZE];
} Render;

void gl_render_init(Render* glr, Display* dpy, Window win, XVisualInfo* vi,
    const char* vertex_file, const char* block_fragment_file, const char* text_fragment_file);

void gl_use_shader(Render* glr, Shader s);

void gl_render_vertex(Render* glr, Vec2f p, Vec2f uv, Vec4f c);
void gl_render_triangle(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3,
    Vec2f uv1, Vec2f uv2, Vec2f uv3,
    Vec4f c1, Vec4f c2, Vec4f c3);

void gl_render_quad(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4,
    Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4,
    Vec4f c1, Vec4f c2, Vec4f c3, Vec4f c4);

void gl_render_rec(Render* glr, Vec2f p, Vec2f s, Vec4f c);
void gl_render_img(Render* glr, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);
    
void gl_clear(Render* glr);
void gl_sync(Render* glr);
void gl_draw_tetris(Render* glr, TetrisMap* tm);

#endif // RENDER_H_
