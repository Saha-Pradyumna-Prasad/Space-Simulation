#include "render/gl_api.h"

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <GL/glx.h>
#include <dlfcn.h>
#endif

namespace glapi {

PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC GenBuffers = nullptr;
PFNGLBINDBUFFERPROC BindBuffer = nullptr;
PFNGLBUFFERDATAPROC BufferData = nullptr;
PFNGLBUFFERSUBDATAPROC BufferSubData = nullptr;
PFNGLDELETEBUFFERSPROC DeleteBuffers = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer = nullptr;
PFNGLVERTEXATTRIBDIVISORPROC VertexAttribDivisor = nullptr;
PFNGLCREATESHADERPROC CreateShader = nullptr;
PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
PFNGLCOMPILESHADERPROC CompileShader = nullptr;
PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC DeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
PFNGLATTACHSHADERPROC AttachShader = nullptr;
PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC UseProgram = nullptr;
PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC Uniform1i = nullptr;
PFNGLUNIFORM1FPROC Uniform1f = nullptr;
PFNGLUNIFORM2FPROC Uniform2f = nullptr;
PFNGLUNIFORM3FPROC Uniform3f = nullptr;
PFNGLUNIFORM4FPROC Uniform4f = nullptr;
PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv = nullptr;
PFNGLDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced = nullptr;
PFNGLDRAWARRAYSINSTANCEDPROC DrawArraysInstanced = nullptr;
PFNGLACTIVETEXTUREPROC ActiveTexture = nullptr;

namespace {

template <typename T>
bool load_one(T &pointer, const char *name) {
#ifdef _WIN32
    pointer = reinterpret_cast<T>(wglGetProcAddress(name));
    if (pointer == nullptr) {
        static HMODULE module = LoadLibraryA("opengl32.dll");
        if (module != nullptr) {
            pointer = reinterpret_cast<T>(GetProcAddress(module, name));
        }
    }
#else
    static void *handle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
    pointer = reinterpret_cast<T>(
        glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(name)));
    if (pointer == nullptr && handle != nullptr) {
        pointer = reinterpret_cast<T>(dlsym(handle, name));
    }
#endif
    if (pointer == nullptr) {
        std::fprintf(stderr, "OpenGL function not available: %s\n", name);
        return false;
    }
    return true;
}

} // namespace

bool load() {
    bool ok = true;
    ok = load_one(GenVertexArrays, "glGenVertexArrays") && ok;
    ok = load_one(BindVertexArray, "glBindVertexArray") && ok;
    ok = load_one(DeleteVertexArrays, "glDeleteVertexArrays") && ok;
    ok = load_one(GenBuffers, "glGenBuffers") && ok;
    ok = load_one(BindBuffer, "glBindBuffer") && ok;
    ok = load_one(BufferData, "glBufferData") && ok;
    ok = load_one(BufferSubData, "glBufferSubData") && ok;
    ok = load_one(DeleteBuffers, "glDeleteBuffers") && ok;
    ok = load_one(EnableVertexAttribArray, "glEnableVertexAttribArray") && ok;
    ok = load_one(DisableVertexAttribArray, "glDisableVertexAttribArray") && ok;
    ok = load_one(VertexAttribPointer, "glVertexAttribPointer") && ok;
    ok = load_one(VertexAttribDivisor, "glVertexAttribDivisor") && ok;
    ok = load_one(CreateShader, "glCreateShader") && ok;
    ok = load_one(ShaderSource, "glShaderSource") && ok;
    ok = load_one(CompileShader, "glCompileShader") && ok;
    ok = load_one(GetShaderiv, "glGetShaderiv") && ok;
    ok = load_one(GetShaderInfoLog, "glGetShaderInfoLog") && ok;
    ok = load_one(DeleteShader, "glDeleteShader") && ok;
    ok = load_one(CreateProgram, "glCreateProgram") && ok;
    ok = load_one(AttachShader, "glAttachShader") && ok;
    ok = load_one(LinkProgram, "glLinkProgram") && ok;
    ok = load_one(GetProgramiv, "glGetProgramiv") && ok;
    ok = load_one(GetProgramInfoLog, "glGetProgramInfoLog") && ok;
    ok = load_one(UseProgram, "glUseProgram") && ok;
    ok = load_one(DeleteProgram, "glDeleteProgram") && ok;
    ok = load_one(GetUniformLocation, "glGetUniformLocation") && ok;
    ok = load_one(Uniform1i, "glUniform1i") && ok;
    ok = load_one(Uniform1f, "glUniform1f") && ok;
    ok = load_one(Uniform2f, "glUniform2f") && ok;
    ok = load_one(Uniform3f, "glUniform3f") && ok;
    ok = load_one(Uniform4f, "glUniform4f") && ok;
    ok = load_one(UniformMatrix4fv, "glUniformMatrix4fv") && ok;
    ok = load_one(DrawElementsInstanced, "glDrawElementsInstanced") && ok;
    ok = load_one(DrawArraysInstanced, "glDrawArraysInstanced") && ok;
    ok = load_one(ActiveTexture, "glActiveTexture") && ok;
    return ok;
}

} // namespace glapi
