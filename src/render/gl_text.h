#ifndef SPACESIM_RENDER_GL_TEXT_H
#define SPACESIM_RENDER_GL_TEXT_H

#include "render/gl_shader.h"

#include <GL/gl.h>

namespace spacesim {

class GlText {
public:
    GlText();
    ~GlText();

    bool init();
    void begin(int width, int height);
    void end();

    void draw_text(float x, float y, const char *text, float scale,
                   float r, float g, float b, float a = 1.0f);
    void draw_rect(float x, float y, float width, float height,
                   float r, float g, float b, float a);
    float text_width(const char *text, float scale) const;

private:
    struct Vertex {
        float x, y, u, v;
    };

    void draw_vertices(const Vertex *vertices, int count);

    GlShader shader_;
    GLuint vao_;
    GLuint vbo_;
    GLuint atlas_texture_;
    GLuint white_texture_;
    int width_;
    int height_;
};

} // namespace spacesim

#endif
