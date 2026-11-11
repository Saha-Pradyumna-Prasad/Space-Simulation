#ifndef SPACESIM_RENDER_GL_SHADER_H
#define SPACESIM_RENDER_GL_SHADER_H

#include <GL/gl.h>

#include <string>

namespace spacesim {

class GlShader {
public:
    GlShader();
    ~GlShader();

    bool build(const char *vertex_source, const char *fragment_source);
    void use() const;
    GLuint id() const { return program_; }
    bool valid() const { return program_ != 0; }
    const std::string &last_log() const { return last_log_; }
    GLint location(const char *name) const;

    void set_float(const char *name, float value) const;
    void set_int(const char *name, int value) const;
    void set_vec2(const char *name, float x, float y) const;
    void set_vec3(const char *name, float x, float y, float z) const;
    void set_vec4(const char *name, float x, float y, float z, float w) const;
    void set_mat4(const char *name, const float *matrix) const;

private:
    GLuint program_;
    std::string last_log_;
};

} // namespace spacesim

#endif
