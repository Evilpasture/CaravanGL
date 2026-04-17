#pragma once
#include "caravangl_specs.h"
#include <Python.h>

#include "caravangl_arg_indices.h"

typedef struct CaravanGLTable {
// 3.3 is safe on Mac
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#define GL_PTR_GEN(ret, name, ...) [[nodiscard]] ret(GL_API *name)(__VA_ARGS__);
    GL_FUNCTIONS_3_3_CORE(GL_PTR_GEN)
#undef GL_PTR_GEN

// 4.2+ gets the "Silly Apple" treatment
#ifndef __APPLE__
// Standard path for Linux/Windows
#define GL_PTR_4_2(ret, name, ...) [[nodiscard]] ret(GL_API *name)(__VA_ARGS__);
#else
// Mac path: Functions exist in the struct (to keep offsets same)
// but are poisoned to cause a compile error if used.
#define GL_PTR_4_2(ret, name, ...)                                                                 \
    __attribute__((unavailable("OpenGL 4.2+ is not supported on macOS"))) ret(GL_API *name)(       \
        __VA_ARGS__);
#endif

    GL_FUNCTIONS_4_2_CORE(GL_PTR_4_2)
    GL_FUNCTIONS_4_3_CORE(GL_PTR_4_2)
    GL_FUNCTIONS_4_4_CORE(GL_PTR_4_2)
    GL_FUNCTIONS_4_3_OPTIONAL(GL_PTR_4_2)
    GL_FUNCTIONS_4_6_OPTIONAL(GL_PTR_4_2)
    GL_FUNCTIONS_EXT_BINDLESS(GL_PTR_4_2)

#undef GL_PTR_DEPRECATED
#pragma clang diagnostic pop
} CaravanGLTable;

typedef struct CaravanState {
    CaravanParsers parsers;
    CaravanContext ctx;
    CaravanGLTable gl;

    PyTypeObject *BufferType;
    PyTypeObject *PipelineType;
    PyTypeObject *ProgramType;
    PyTypeObject *VertexArrayType;
    PyTypeObject *UniformBatchType;
    PyTypeObject *TextureType;
    PyTypeObject *FramebufferType;
    PyTypeObject *SamplerType;
    PyObject *CaravanError;
} CaravanState;

[[maybe_unused]]
static inline CaravanState *get_caravan_state(PyObject *mod) {
    if (!mod || !PyModule_Check(mod)) {
        return nullptr;
    }
    return (CaravanState *)PyModule_GetState(mod);
}

static inline CaravanGLTable gl_table(PyObject *mod) {
    return get_caravan_state(mod)->gl;
}

// Internal helper: must take a pointer to the pointer for cleanup
static inline void internal_cv_auto_unlock(MagMutex **mod) {
    if (*mod) {
        MagMutex_Unlock(*mod);
    }
}
// PREFERABLY USE THIS ONLY WHEN USING OPENGL CALLS. PUT AS MANY PYTHON CALLS AND C LOGIC OUTSIDE
// THE SCOPE!
#define WithCaravanGL(module_ptr, gl_name)                                                         \
    _Pragma("unroll 69") for (int _cv_done = 0; !_cv_done; _cv_done = 1) _Pragma(                  \
        "unroll 69") for (PyObject *_cv_m = (PyObject *)(module_ptr);                              \
                          !_cv_done && _cv_m != nullptr; _cv_done = 1)                             \
        _Pragma("unroll 69") for (CaravanState *state = get_caravan_state(_cv_m);                  \
                                  !_cv_done && state != nullptr; _cv_done = 1)                     \
            _Pragma("unroll 69") for (MagMutex * _cv_l [[gnu::cleanup(internal_cv_auto_unlock)]] = \
                                          (MagMutex_Lock(&state->ctx.state_lock),                  \
                                           &state->ctx.state_lock);                                \
                                      !_cv_done; _cv_done = 1)                                     \
                _Pragma("unroll 69") for (CaravanGLTable * (gl_name) = &state->gl; !_cv_done;      \
                                          _cv_done = 1)

#if defined(__has_attribute)
#if !__has_attribute(cleanup)
static_assert(false, "CaravanGL requires __attribute__((cleanup)) for thread safety. "
                     "Please use a compiler that supports GNU extensions (GCC/Clang).");
#endif
#else
#error "Compiler does not support __has_attribute. Cannot verify RAII safety."
#endif

#ifdef NDEBUG
/* --- RELEASE MODE: High Performance, No Stalls --- */

// Just execute the call. No error checking, no stalls.
#define CARAVAN_GL_CHECK(gl_ptr, call) (call)
#define CARAVAN_GL_CHECK_INT(gl_ptr, call) (call)

#else
/* --- DEBUG MODE: Strict Error Checking --- */

#define CARAVAN_GL_CHECK(gl_ptr, call)                                                             \
    do {                                                                                           \
        while ((gl_ptr).GetError() != GL_NO_ERROR)                                                 \
            ;                                                                                      \
        (call);                                                                                    \
        GLenum _cv_err = (gl_ptr).GetError();                                                      \
        if (_cv_err != GL_NO_ERROR) {                                                              \
            PyErr_Format(PyExc_RuntimeError, "OpenGL Error [0x%04X] in: %s", _cv_err, #call);      \
            return nullptr;                                                                        \
        }                                                                                          \
    } while (false)

#define CARAVAN_GL_CHECK_INT(gl_ptr, call)                                                         \
    do {                                                                                           \
        while ((gl_ptr).GetError() != GL_NO_ERROR)                                                 \
            ;                                                                                      \
        (call);                                                                                    \
        GLenum _cv_err = (gl_ptr).GetError();                                                      \
        if (_cv_err != GL_NO_ERROR) {                                                              \
            PyErr_Format(PyExc_RuntimeError, "OpenGL Error [0x%04X] in: %s", _cv_err, #call);      \
            return -1;                                                                             \
        }                                                                                          \
    } while (false)

#endif

#if !defined(__APPLE__)
static void APIENTRY opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                           GLsizei length, const GLchar *message,
                                           const void *userParam) {
    // userParam is the 'state' pointer we passed to DebugMessageCallback.
    // This allows access to module state without global variables!
    // [[maybe_unused]] CaravanState* state = (CaravanState*)userParam;

    // The List.
    static constexpr GLuint ignore_list[] = {
        131185, // Buffer usage info
        131218, // Shader recompilation
        131204, // Texture usage info
    };

    for (size_t i = 0; i < sizeof(ignore_list) / sizeof(GLuint); ++i) {
        if (id == ignore_list[i])
            return;
    }

    const char *_src = "Unknown";
    const char *_typ = "Unknown";
    const char *_sev = "Unknown";

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        _src = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        _src = "Window System";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        _src = "Shader Compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        _src = "Third Party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        _src = "Application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        _src = "Other";
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        _typ = "Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        _typ = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        _typ = "Undefined Behavior";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        _typ = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        _typ = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER:
        _typ = "Other";
        break;
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        _sev = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        _sev = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        _sev = "LOW";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        _sev = "INFO";
        break;
    }

    // Using raw fprintf because this callback might be triggered by a driver
    // thread where holding the Python GIL is not guaranteed.
    fprintf(stderr, "[CaravanGL] %s | %s [%s] (ID: %u): %s\n", _src, _typ, _sev, id, message);
}
#endif