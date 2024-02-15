#include "./render.h"
#include <assert.h>

void gl_render_init(Render* glr, Display* dpy, Window win, XVisualInfo* vi,
    const char* vertex_file, const char* block_fragment_file, const char* text_fragment_file) {

    glr->glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glr->glc);

    if (glewInit() != GLEW_OK) {
        perror("failed to initalize glew");
        exit(1);
    }
    glEnable(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &glr->vao);
    glBindVertexArray(glr->vao);
    
    glGenBuffers(1, &glr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, glr->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glr->buffer), glr->buffer, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, pos));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, color));

    glr->program[SHADER_BLOCK] = CreateShader(vertex_file, block_fragment_file, __FILE__, __LINE__);
    glr->program[SHADER_TEXT] = CreateShader(vertex_file, text_fragment_file, __FILE__, __LINE__);
}

void gl_use_shader(Render* glr, Shader s) {
    glUseProgram(glr->program[s]);
    glr->resolution_uniform = glGetUniformLocation(glr->program[s], "res");
    glr->time_uniform = glGetUniformLocation(glr->program[s], "time");
    glr->current_shader = s;
}

void gl_render_vertex(Render* glr, Vec2f p, Vec2f uv, Vec4f c) {
    assert(glr->buffer_count < BUFFER_SIZE);
    glr->buffer[glr->buffer_count++] = (Vertex) {
        .pos = p,
        .uv = uv,
        .color = c,
    };
}

void gl_render_triangle(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3,
    Vec2f uv1, Vec2f uv2, Vec2f uv3,
    Vec4f c1, Vec4f c2, Vec4f c3) {
    gl_render_vertex(glr, p1, uv1, c1);
    gl_render_vertex(glr, p2, uv2, c2);
    gl_render_vertex(glr, p3, uv3, c3);
}

void gl_render_quad(Render* glr,
    Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4,
    Vec2f uv1, Vec2f uv2, Vec2f uv3, Vec2f uv4,
    Vec4f c1, Vec4f c2, Vec4f c3, Vec4f c4) {
    gl_render_triangle(glr, p1, p2, p3, uv1, uv2, uv3, c1, c2, c3);
    gl_render_triangle(glr, p2, p3, p4, uv2, uv3, uv4, c2, c3, c4);
}

void gl_render_rec(Render* glr, Vec2f p, Vec2f s, Vec4f c) {
    Vec2f uv = vec2fs(0);
    gl_render_quad(
        glr,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        uv, uv, uv, uv,
        c, c, c, c);
}

void gl_render_img(Render* glr, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c) {
    gl_render_quad(
        glr,
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs),
        c, c, c, c);
}

void gl_clear(Render* glr) {
    glr->buffer_count = 0;
}

void gl_sync(Render* glr) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * glr->buffer_count, glr->buffer);
}

void gl_draw_tetris(Render* glr, TetrisMap* tm) {
    gl_render_rec(glr, tm->position, tm->pxsize, gl_colors[Wall]);
    for (int i = 0; i < tm->size.x + 2; i++) {
        for (int j = 0; j < tm->size.y + 2; j++) {
            int tmp = tmindex(i, j, tm->size.x + 2);
            Vec2f p = vec2f(tm->r[tmp].x, tm->r[tmp].y);
            Vec2f s = vec2f(tm->r[tmp].width, tm->r[tmp].height);
            if (i == 0 || j == 0 || i > tm->size.x || j > tm->size.y) {
                gl_render_rec(glr, p, s, gl_colors[Wall]);
            }
            else {
                gl_render_rec(glr, p, s, gl_colors[tm->tc[tmindex(i - 1, j - 1, tm->size.x)]]);
            }
        }
    }
}
