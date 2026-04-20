#pragma once
#include "caravangl_specs.h"
#include "caravangl_uniform_upload.h"
#include <Python.h>

#define PyCaravanGL_API [[nodiscard]] static PyObject *
#define PyCaravanGL_Status [[nodiscard]] static int
#define PyCaravanGL_Slot static void
// NOLINTNEXTLINE
#define IntToPtr(x) ((void *)(uintptr_t)(x))

#define CARAVAN_CAST(x) (PyCFunction)(void (*)(void))(x)
#define CARAVAN_STR(x) #x
#define CARAVAN_TOSTR(x) CARAVAN_STR(x)
#define CARAVAN_GLUE(a, b) a##b
#define CARAVAN_JOIN(a, b) CARAVAN_GLUE(a, b)

#define FOR_ALL_CARAVAN_TYPES(DO, state)                                                           \
    DO(state->BufferType)                                                                          \
    DO(state->PipelineType)                                                                        \
    DO(state->ProgramType)                                                                         \
    DO(state->VertexArrayType)                                                                     \
    DO(state->UniformBatchType)                                                                    \
    DO(state->TextureType)

typedef struct {
    [[gnu::aligned(16)]]
    visitproc visit;
    void *arg;
} TraverseContext;

/**
 * Agnostic Visit: Manual implementation of Py_VISIT logic for the dispatcher.
 */
static inline int op_visit_member(PyObject **member, void *context) {
    auto ctx = (TraverseContext *)context;
    PyObject *obj = *member;
    if (obj != nullptr) {
        int result = ctx->visit(obj, ctx->arg);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

/**
 * Agnostic Clear: Uses standard Py_CLEAR logic.
 */
static inline int op_clear_member(PyObject **member, [[maybe_unused]] void *context) {
    PyObject *tmp = *member;
    if (tmp != nullptr) {
        *member = nullptr;
        Py_DECREF(tmp);
    }
    return 0;
}

/**
 * Unified operation signature for any Python object reference.
 */
typedef int (*CaravanMemberOp)(PyObject **member, void *context);

[[gnu::always_inline]]
static inline int caravan_dispatch_members(PyObject **members[], size_t count,
                                           CaravanMemberOp operation, void *context) {
#pragma unroll 4
    for (size_t i = 0; i < count; ++i) {
        int result = operation(members[i], context);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

typedef struct PyCaravanContext {
    PyObject_HEAD

        CaravanGLTable gl;  // Function pointers per-context
    CaravanContext ctx;     // The Shadow State & Mutex
    CaravanGarbage garbage; // Deferred deletion queue

    // Optional callback to trigger the OS-level glfwMakeContextCurrent
    PyObject *os_make_current_cb;
    PyObject *os_release_cb;
} PyCaravanContext;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    CaravanBuffer buf;
    PyObject *weakreflist;
    Py_ssize_t map_shape[1];
} PyCaravanBuffer;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    CaravanTexture tex;
} PyCaravanTexture;

typedef struct PyCaravanPipeline {
    PyObject_HEAD

        PyCaravanContext *owning_context;

    PyObject *program_ref;
    PyObject *vao_ref;

    // Core GPU Objects
    GLuint program;
    GLuint vao;

    // Topology and Indexing
    GLenum topology;   // GL_TRIANGLES, GL_LINES, etc.
    GLenum index_type; // GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, or GL_NONE (0)

    // State Requirements
    CaravanRenderState render_state;

    // Fast-mutation draw data
    CaravanDrawParams params;

    Py_ssize_t params_shape[1];
    Py_ssize_t params_strides[1];
} PyCaravanPipeline;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    GLuint id;
} PyCaravanProgram;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    GLuint id;
} PyCaravanVertexArray;

typedef struct {
    PyObject_HEAD CaravanUniformHeader *header;
    char *payload;

    uint32_t max_bindings;
    uint32_t max_payload_bytes;
    uint32_t current_payload_offset;
} PyCaravanUniformBatch;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    CaravanFramebuffer fbo;
} PyCaravanFramebuffer;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    GLuint id;
} PyCaravanSampler;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    GLsync sync_obj;
} PyCaravanSync;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    GLuint id;
    GLenum target;
} PyCaravanQuery;

typedef struct {
    PyObject_HEAD PyCaravanContext *owning_context;
    GLuint id;
} PyCaravanComputePipeline;