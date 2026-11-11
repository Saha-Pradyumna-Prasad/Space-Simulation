#include "render/gl_shader.h"

#include "render/gl_api.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace spacesim {

namespace {

std::string shader_info_log(GLuint shader) {
    GLint length = 0;
    glapi::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) return std::string();
    std::string log(static_cast<size_t>(length), '\0');
    GLsizei written = 0;
    glapi::GetShaderInfoLog(shader, length, &written, &log[0]);
    if (written > 0) log.resize(static_cast<size_t>(written));
    while (!log.empty() && (log.back() == '\0' || log.back() == '\n')) {
        log.pop_back();
    }
    return log;
}

std::string program_info_log(GLuint program) {
    GLint length = 0;
    glapi::GetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) return std::string();
    std::string log(static_cast<size_t>(length), '\0');
    GLsizei written = 0;
    glapi::GetProgramInfoLog(program, length, &written, &log[0]);
    if (written > 0) log.resize(static_cast<size_t>(written));
    while (!log.empty() && (log.back() == '\0' || log.back() == '\n')) {
        log.pop_back();
    }
    return log;
}

GLuint compile(GLenum type, const char *source, std::string &last_log) {
    GLuint shader = glapi::CreateShader(type);
    glapi::ShaderSource(shader, 1, &source, nullptr);
    glapi::CompileShader(shader);
    GLint ok = GL_FALSE;
    glapi::GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        const char *stage = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        last_log += std::string("Shader compile failed (") + stage + "):\n" +
                    shader_info_log(shader);
        std::fprintf(stderr, "Shader compile failed (%s):\n%s\n", stage,
                     shader_info_log(shader).c_str());
        glapi::DeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

GlShader::GlShader() : program_(0) {}

GlShader::~GlShader() {
    if (program_ != 0) glapi::DeleteProgram(program_);
}

bool GlShader::build(const char *vertex_source, const char *fragment_source) {
    last_log_.clear();

    GLuint vertex = compile(GL_VERTEX_SHADER, vertex_source, last_log_);
    GLuint fragment = compile(GL_FRAGMENT_SHADER, fragment_source, last_log_);
    if (vertex == 0 || fragment == 0) {
        if (vertex != 0) glapi::DeleteShader(vertex);
        if (fragment != 0) glapi::DeleteShader(fragment);
        return false;
    }

    program_ = glapi::CreateProgram();
    glapi::AttachShader(program_, vertex);
    glapi::AttachShader(program_, fragment);
    glapi::LinkProgram(program_);
    glapi::DeleteShader(vertex);
    glapi::DeleteShader(fragment);

    GLint ok = GL_FALSE;
    glapi::GetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        last_log_ += "Program link failed:\n" + program_info_log(program_);
        std::fprintf(stderr, "Shader link failed:\n%s\n",
                     program_info_log(program_).c_str());
        glapi::DeleteProgram(program_);
        program_ = 0;
        return false;
    }

    if (last_log_.empty()) last_log_ = "OK";
    return true;
}

void GlShader::use() const {
    if (program_ != 0) glapi::UseProgram(program_);
}

GLint GlShader::location(const char *name) const {
    if (program_ == 0) return -1;
    return glapi::GetUniformLocation(program_, name);
}

void GlShader::set_float(const char *name, float value) const {
    if (program_ == 0) return;
    glapi::Uniform1f(location(name), value);
}

void GlShader::set_int(const char *name, int value) const {
    if (program_ == 0) return;
    glapi::Uniform1i(location(name), value);
}

void GlShader::set_vec2(const char *name, float x, float y) const {
    if (program_ == 0) return;
    glapi::Uniform2f(location(name), x, y);
}

void GlShader::set_vec3(const char *name, float x, float y, float z) const {
    if (program_ == 0) return;
    glapi::Uniform3f(location(name), x, y, z);
}

void GlShader::set_vec4(const char *name, float x, float y, float z, float w) const {
    if (program_ == 0) return;
    glapi::Uniform4f(location(name), x, y, z, w);
}

void GlShader::set_mat4(const char *name, const float *matrix) const {
    if (program_ == 0) return;
    glapi::UniformMatrix4fv(location(name), 1, GL_FALSE, matrix);
}

} // namespace spacesim
