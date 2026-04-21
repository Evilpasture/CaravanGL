#pragma once
#include "caravangl_arg_indices.h"
#include "caravangl_state.h"
#include "mag_mutex.h"
#include <Python.h>

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
    PyTypeObject *ComputePipelineType;
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

/**
 * WithContext: Wraps a PyCaravanContext pointer and extracts the handle.
 */
#define WithContext(py_ctx_ptr, gl_name, state_name)                                               \
    WithHandle(&(py_ctx_ptr)->handle, gl_name, state_name)

/**
 * WithActiveGL: The TLS-aware version for top-level Python methods.
 */
#define WithActiveGL(gl_name, state_name, ...)                                                     \
    PyCaravanContext *_cv_ctx = cv_active_context;                                                 \
    if (!_cv_ctx) [[clang::unlikely]] {                                                            \
        PyErr_SetString(PyExc_RuntimeError, "No active CaravanGL context on this thread.");        \
        return __VA_ARGS__;                                                                        \
    }                                                                                              \
    WithHandle(&_cv_ctx->handle, gl_name, state_name)

#define CV_SAFE_DEALLOC(obj_ptr, id_field, count_field, array_field, gl_delete_call)               \
    do {                                                                                           \
        if ((obj_ptr)->id_field != 0 && (obj_ptr)->owning_context) {                               \
            PyCaravanContext *owning_context = (obj_ptr)->owning_context;                          \
            if (owning_context == cv_active_context) {                                             \
                /* Current thread owns this, delete immediately */                                 \
                WithContext(owning_context, OpenGL, _unused) {                                     \
                    gl_delete_call;                                                                \
                }                                                                                  \
            } else {                                                                               \
                /* Thread mismatch, enqueue for later */                                           \
                MagMutex_Lock(&owning_context->handle.ctx.state_lock);                             \
                cv_enqueue_garbage(&owning_context->handle.garbage.count_field,                    \
                                   owning_context->handle.garbage.array_field,                     \
                                   (obj_ptr)->id_field);                                           \
                MagMutex_Unlock(&owning_context->handle.ctx.state_lock);                           \
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
