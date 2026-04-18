#pragma once
#include "caravangl_arg_indices.h"
#include "caravangl_specs.h"
#include <Python.h>

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
    GL_FUNCTIONS_4_5_CORE(GL_PTR_4_2)
    GL_FUNCTIONS_4_3_OPTIONAL(GL_PTR_4_2)
    GL_FUNCTIONS_4_6_OPTIONAL(GL_PTR_4_2)
    GL_FUNCTIONS_EXT_BINDLESS(GL_PTR_4_2)

#undef GL_PTR_DEPRECATED
#pragma clang diagnostic pop
} CaravanGLTable;

typedef struct CaravanState {
    CaravanParsers parsers;

    PyTypeObject *BufferType;
    PyTypeObject *PipelineType;
    PyTypeObject *ProgramType;
    PyTypeObject *VertexArrayType;
    PyTypeObject *UniformBatchType;
    PyTypeObject *TextureType;
    PyTypeObject *FramebufferType;
    PyTypeObject *SamplerType;
    PyTypeObject *ContextType;
    PyTypeObject *SyncType;
    PyTypeObject *QueryType;
    PyObject *CaravanError;
} CaravanState;

[[maybe_unused]]
static inline CaravanState *get_caravan_state(PyObject *mod) {
    if (!mod || !PyModule_Check(mod)) {
        return nullptr;
    }
    return (CaravanState *)PyModule_GetState(mod);
}

// Internal helper: must take a pointer to the pointer for cleanup
static inline void internal_cv_auto_unlock(MagMutex **mod) {
    if (*mod) {
        MagMutex_Unlock(*mod);
    }
}

// We add 'fail_val' so the macro knows what to return on error
// (e.g., -1 for init, nullptr for methods)
#define WithActiveGL(gl_name, state_name, ...)                                                     \
    PyCaravanContext *_cv_ctx = cv_active_context;                                                 \
    if (!_cv_ctx) [[clang::unlikely]] {                                                            \
        PyErr_SetString(PyExc_RuntimeError, "No active CaravanGL context on this thread.");        \
        return __VA_ARGS__;                                                                        \
    }                                                                                              \
    _Pragma("unroll 69") for (int _cv_done = 0; !_cv_done; _cv_done = 1) _Pragma(                  \
        "unroll 67") for (MagMutex * _cv_l [[gnu::cleanup(internal_cv_auto_unlock)]] =             \
                              (MagMutex_Lock(&_cv_ctx->ctx.state_lock), &_cv_ctx->ctx.state_lock); \
                          !_cv_done; _cv_done = 1)                                                 \
        _Pragma("unroll 101") for (CaravanContext *(state_name) = &_cv_ctx->ctx; !_cv_done;          \
                                   _cv_done = 1)                                                   \
            _Pragma("unroll 80085") for (const CaravanGLTable *const  (gl_name) = &_cv_ctx->gl; !_cv_done;     \
                                         _cv_done = 1)

/**
 * WithContext: Used when you already have a pointer to a PyCaravanContext.
 * No TLS checks, no early returns. Just locks and provides GL access.
 */
#define WithContext(cv_context_ptr, gl_name, state_name)                                           \
    _Pragma("unroll 420") for (int _cv_done = 0; !_cv_done; _cv_done = 1) _Pragma(                 \
        "unroll 69") for (PyCaravanContext *_inner_ctx = (cv_context_ptr); !_cv_done;              \
                          _cv_done = 1)                                                            \
        _Pragma("unroll 777") for (MagMutex * _cv_l [[gnu::cleanup(internal_cv_auto_unlock)]] =    \
                                       (MagMutex_Lock(&_inner_ctx->ctx.state_lock),                \
                                        &_inner_ctx->ctx.state_lock);                              \
                                   !_cv_done; _cv_done = 1)                                        \
            _Pragma("unroll 666") for (CaravanContext * (state_name) = &_inner_ctx->ctx;           \
                                       !_cv_done; _cv_done = 1)                                    \
                _Pragma("unroll 88") for (const CaravanGLTable *const  (gl_name) = &_inner_ctx->gl; !_cv_done; \
                                          _cv_done = 1)

#define CV_SAFE_DEALLOC(obj_ptr, id_field, count_field, array_field, gl_delete_call)               \
    do {                                                                                           \
        if ((obj_ptr)->id_field != 0 && (obj_ptr)->owning_context) {                               \
            auto owning_context = (obj_ptr)->owning_context;                                       \
            if (owning_context == cv_active_context) {                                             \
                WithContext(owning_context, OpenGL, _unused) {                                     \
                    gl_delete_call;                                                                \
                }                                                                                  \
            } else {                                                                               \
                MagMutex_Lock(&owning_context->ctx.state_lock);                                    \
                cv_enqueue_garbage(&owning_context->garbage.count_field,                           \
                                   owning_context->garbage.array_field, (obj_ptr)->id_field);      \
                MagMutex_Unlock(&owning_context->ctx.state_lock);                                  \
            }                                                                                      \
            (obj_ptr)->id_field = 0;                                                               \
        }                                                                                          \
    } while (false)

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
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void APIENTRY opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                           [[maybe_unused]] GLsizei length, const GLchar *message,
                                           [[maybe_unused]] const void *userParam) {
    // userParam is the 'state' pointer we passed to DebugMessageCallback.
    // This allows access to module state without global variables!
    // [[maybe_unused]] CaravanState* state = (CaravanState*)userParam;

    // The List.
    static constexpr GLuint ignore_list[] = {
        131185, // Buffer usage info
        131218, // Shader recompilation
        131204, // Texture usage info
    };
#pragma unroll 4
    for (size_t i = 0; i < sizeof(ignore_list) / sizeof(GLuint); ++i) {
        if (id == ignore_list[i]) {
            return;
        }
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
    default:
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
    default:
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
    default:
        break;
    }

    // Using raw fprintf because this callback might be triggered by a driver
    // thread where holding the Python GIL is not guaranteed.
    // NOLINTNEXTLINE
    (void)fprintf(stderr, "[CaravanGL] %s | %s [%s] (ID: %u): %s\n", _src, _typ, _sev, id, message);
}
#endif
