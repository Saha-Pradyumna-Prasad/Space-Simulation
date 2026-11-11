#include "render/gl_text.h"

#include "render/font_atlas.h"
#include "render/gl_api.h"

#include <cstring>

namespace spacesim {

namespace {

const char *kVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
uniform vec2 uScreen;
void main() {
    vec2 ndc = vec2(aPos.x / uScreen.x * 2.0 - 1.0,
                    aPos.y / uScreen.y * 2.0 - 1.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
}
)GLSL";

const char *kFragmentShader = R"GLSL(
#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uTexture;
uniform vec4 uColor;
void main() {
    float alpha = texture(uTexture, vUV).r;
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)GLSL";

} // namespace

GlText::GlText() : vao_(0), vbo_(0), atlas_texture_(0), white_texture_(0),
                   width_(1), height_(1) {}

GlText::~GlText() {
    if (vbo_ != 0) glapi::DeleteBuffers(1, &vbo_);
    if (vao_ != 0) glapi::DeleteVertexArrays(1, &vao_);
    if (atlas_texture_ != 0) glDeleteTextures(1, &atlas_texture_);
    if (white_texture_ != 0) glDeleteTextures(1, &white_texture_);
}

bool GlText::init() {
    if (!shader_.build(kVertexShader, kFragmentShader)) return false;

    glapi::GenVertexArrays(1, &vao_);
    glapi::GenBuffers(1, &vbo_);
    glapi::BindVertexArray(vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, vbo_);
    glapi::EnableVertexAttribArray(0);
    glapi::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<void *>(0));
    glapi::EnableVertexAttribArray(1);
    glapi::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<void *>(sizeof(float) * 2));
    glapi::BufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 6 * 4096,
                      nullptr, GL_DYNAMIC_DRAW);
    glapi::BindVertexArray(0);

    glGenTextures(1, &atlas_texture_);
    glBindTexture(GL_TEXTURE_2D, atlas_texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONT_ATLAS_W, FONT_ATLAS_H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, FONT_ATLAS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    unsigned char white = 255;
    glGenTextures(1, &white_texture_);
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED,
                 GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return true;
}

void GlText::begin(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
    shader_.use();
    shader_.set_int("uTexture", 0);
    shader_.set_vec2("uScreen", static_cast<float>(width_), static_cast<float>(height_));
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glapi::ActiveTexture(GL_TEXTURE0);
    glapi::BindVertexArray(vao_);
}

void GlText::end() {
    glapi::BindVertexArray(0);
}

void GlText::draw_vertices(const Vertex *vertices, int count) {
    if (count <= 0) return;
    glapi::BindBuffer(GL_ARRAY_BUFFER, vbo_);
    glapi::BufferSubData(GL_ARRAY_BUFFER, 0,
                         static_cast<GLsizeiptr>(sizeof(Vertex) * count),
                         vertices);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void GlText::draw_text(float x, float y, const char *text, float scale,
                       float r, float g, float b, float a) {
    if (text == nullptr) return;
    shader_.use();
    shader_.set_int("uTexture", 0);
    shader_.set_vec4("uColor", r, g, b, a);
    glBindTexture(GL_TEXTURE_2D, atlas_texture_);

    Vertex vertices[6 * 512];
    int count = 0;
    float cursor = x;
    for (const char *p = text; *p != '\0'; ++p) {
        unsigned char ch = static_cast<unsigned char>(*p);
        if (ch < 32 || ch > 126) {
            cursor += FONT_CELL_W * scale;
            continue;
        }
        const FontGlyph &glyph = FONT_GLYPHS[ch - 32];
        float w = FONT_CELL_W * scale;
        float h = FONT_CELL_H * scale;
        float x0 = cursor;
        float y0 = y;
        float x1 = cursor + w;
        float y1 = y + h;
        float v_bottom = glyph.v1;
        float v_top = glyph.v0;

        vertices[count++] = {x0, y0, glyph.u0, v_bottom};
        vertices[count++] = {x1, y0, glyph.u1, v_bottom};
        vertices[count++] = {x1, y1, glyph.u1, v_top};
        vertices[count++] = {x0, y0, glyph.u0, v_bottom};
        vertices[count++] = {x1, y1, glyph.u1, v_top};
        vertices[count++] = {x0, y1, glyph.u0, v_top};

        cursor += static_cast<float>(glyph.advance) * scale;
        if (count >= 6 * 512 - 6) break;
    }
    draw_vertices(vertices, count);
}

void GlText::draw_rect(float x, float y, float width, float height,
                       float r, float g, float b, float a) {
    shader_.use();
    shader_.set_int("uTexture", 0);
    shader_.set_vec4("uColor", r, g, b, a);
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    Vertex vertices[6] = {
        {x, y, 0.0f, 0.0f},
        {x + width, y, 1.0f, 0.0f},
        {x + width, y + height, 1.0f, 1.0f},
        {x, y, 0.0f, 0.0f},
        {x + width, y + height, 1.0f, 1.0f},
        {x, y + height, 0.0f, 1.0f},
    };
    draw_vertices(vertices, 6);
}

float GlText::text_width(const char *text, float scale) const {
    if (text == nullptr) return 0.0f;
    float width = 0.0f;
    for (const char *p = text; *p != '\0'; ++p) {
        unsigned char ch = static_cast<unsigned char>(*p);
        if (ch < 32 || ch > 126) {
            width += FONT_CELL_W * scale;
        } else {
            width += static_cast<float>(FONT_GLYPHS[ch - 32].advance) * scale;
        }
    }
    return width;
}

} // namespace spacesim
