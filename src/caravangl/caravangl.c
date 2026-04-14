/**
 * ============================================================================
 * CaravanGL - Final Implementation
 * ============================================================================
 * Architecture: C23 + Python 3.14t (Free-Threaded)
 * Features: Isolated State, Fast-Path Parsing, Register-Speed Build, No-GIL.
 * ============================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <string.h>
#include "pycaravangl.h"
#include "caravangl_arg_indices.h"
#include "fast_build.h"

// -----------------------------------------------------------------------------
// Internal Helpers
// -----------------------------------------------------------------------------

/**
 * Queries GPU hardware limits and populates the state context.
 * Called once during caravan.init().
 */
static void query_capabilities(PyObject *m) {
    WithCaravanGL(m, gl) {
        if (gl.GetIntegerv != nullptr) {
            gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &state->ctx.caps.max_texture_size);
            gl.GetIntegerv(GL_MAX_SAMPLES, &state->ctx.caps.max_samples);
            gl.GetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &state->ctx.caps.max_uniform_block_size);
            
            GLint v[4] = {0};
            gl.GetIntegerv(GL_VIEWPORT, v);
            state->ctx.viewport = (CaravanRect){v[0], v[1], v[2], v[3]};
        }
        state->ctx.caps.support_compute = (gl.DispatchCompute != nullptr);
        state->ctx.caps.support_bindless = (gl.GetTextureHandleARB != nullptr);
    }
}

// -----------------------------------------------------------------------------
// Buffer Object Implementation (Heap Type)
// -----------------------------------------------------------------------------

/**
 * Buffer Deallocator: Cleans up GPU resources before the Python object is freed.
 */
static void Buffer_dealloc(PyCaravanBuffer *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    
    WithCaravanGL(m, gl) {
        if (self->buf.id != 0) {
            gl.DeleteBuffers(1, &self->buf.id);
            self->buf.id = 0;
        }
    }

    if (self->weakreflist != nullptr) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }

    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

/**
 * Buffer Init: Handles creation and initial data upload.
 * Signature: (self, args, kwds) -> Standard tp_init
 */
static int Buffer_init(PyCaravanBuffer *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        int size = 0;
        PyObject *data = nullptr;
        uint32_t target = GL_ARRAY_BUFFER;
        uint32_t usage = GL_STATIC_DRAW;

        void *targets[BufInit_COUNT] = {
            [IDX_BUF_SIZE]   = &size,
            [IDX_BUF_DATA]   = &data,
            [IDX_BUF_TARGET] = &target,
            [IDX_BUF_USAGE]  = &usage
        };

        // tp_init call: (args, kwds, kwnames=nullptr, parser, targets)
        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.BufInitParser, targets)) {
            return -1;
        }

        gl.GenBuffers(1, &self->buf.id);
        gl.BindBuffer(target, self->buf.id);

        const void *ptr = nullptr;
        Py_buffer view;
        if (data && data != Py_None) {
            // Python 3.14t signature: (view, obj, buf, len, readonly, flags)
            if (PyBuffer_FillInfo(&view, data, nullptr, 0, 0, PyBUF_SIMPLE) == 0) {
                ptr = view.buf;
            }
        }

        gl.BufferData(target, (GLsizeiptr)size, ptr, usage);
        if (ptr) PyBuffer_Release(&view);

        self->buf.target = target;
        self->buf.size = (GLsizeiptr)size;
        self->buf.usage = usage;
    }
    return 0;
}

/**
 * Buffer Write: Updates a region of the buffer.
 * Signature: (self, args, nargs, kwnames) -> FastCall
 */
static PyObject* Buffer_write(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        PyObject *data = nullptr;
        int offset = 0;
        void *targets[BufWrite_COUNT] = {
            [IDX_BUF_WRITE_DATA]   = &data,
            [IDX_BUF_WRITE_OFFSET] = &offset
        };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufWriteParser, targets)) {
            return nullptr;
        }

        Py_buffer view;
        if (data == nullptr || PyBuffer_FillInfo(&view, data, nullptr, 0, 0, PyBUF_SIMPLE) != 0) {
            return nullptr;
        }

        if (offset + view.len > self->buf.size) {
            PyErr_SetString(PyExc_ValueError, "Data exceeds buffer size");
            PyBuffer_Release(&view);
            return nullptr;
        }

        gl.BindBuffer(self->buf.target, self->buf.id);
        gl.BufferSubData(self->buf.target, (GLintptr)offset, (GLsizeiptr)view.len, view.buf);
        PyBuffer_Release(&view);
    }
    Py_RETURN_NONE;
}

/**
 * Buffer Bind Base: Binds buffer to an indexed target (UBO/SSBO).
 * Signature: (self, args, nargs, kwnames) -> FastCall
 */
static PyObject* Buffer_bind_base(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        uint32_t index = 0;
        void *targets[BufBind_COUNT] = { [IDX_BUF_BIND_IDX] = &index };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufBindParser, targets)) {
            return nullptr;
        }
        gl.BindBufferBase(self->buf.target, index, self->buf.id);
    }
    Py_RETURN_NONE;
}

static PyMethodDef Buffer_methods[] = {
    {"write", (PyCFunction)(void(*)(void))Buffer_write, METH_FASTCALL | METH_KEYWORDS, "Write data to buffer."},
    {"bind_base", (PyCFunction)(void(*)(void))Buffer_bind_base, METH_FASTCALL | METH_KEYWORDS, "Bind as indexed resource."},
    {nullptr}
};

static PyType_Slot Buffer_slots[] = {
    {Py_tp_init, Buffer_init},
    {Py_tp_dealloc, Buffer_dealloc},
    {Py_tp_methods, Buffer_methods},
    {Py_tp_doc, "CaravanGL Buffer Object (VBO, IBO, UBO)"},
    {0, nullptr}
};

static PyType_Spec Buffer_spec = {
    .name = "caravangl.Buffer",
    .basicsize = sizeof(PyCaravanBuffer),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Buffer_slots,
};

// -----------------------------------------------------------------------------
// Module-Level API
// -----------------------------------------------------------------------------

/**
 * caravan.init(loader)
 * Loads the function table and discovers GPU capabilities.
 */
static PyObject *caravan_init(PyObject *m, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    WithCaravanGL(m, gl) {
        PyObject *loader = nullptr;
        void *targets[Init_COUNT] = { [IDX_INIT_LOADER] = &loader };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.InitParser, targets)) {
            return nullptr;
        }

        if (load_gl(state, loader) < 0) return nullptr;
        query_capabilities(m);
    }
    Py_RETURN_NONE;
}

/**
 * caravangl.context() -> dict
 * Returns a snapshot of capabilities and driver info using FastBuild.
 */
static PyObject *caravan_context(PyObject *m, [[maybe_unused]] PyObject *const *args, [[maybe_unused]] Py_ssize_t nargs, [[maybe_unused]] PyObject *kwnames) {
    WithCaravanGL(m, gl) {
        if (gl.GetString == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "OpenGL not loaded. Call init() first.");
            return nullptr;
        }

        return FastBuild_Dict(
            "caps", FastBuild_Dict(
                "max_texture_size", state->ctx.caps.max_texture_size,
                "max_samples",      state->ctx.caps.max_samples,
                "support_compute",  state->ctx.caps.support_compute,
                "support_bindless", state->ctx.caps.support_bindless
            ),
            "info", FastBuild_Dict(
                "vendor",   (const char*)gl.GetString(GL_VENDOR),
                "renderer", (const char*)gl.GetString(GL_RENDERER),
                "version",  (const char*)gl.GetString(GL_VERSION)
            ),
            "viewport", FastBuild_Tuple(
                state->ctx.viewport.x, state->ctx.viewport.y,
                state->ctx.viewport.w, state->ctx.viewport.h
            )
        );
    }
    return nullptr;
}

static PyObject *caravan_test_render(PyObject *m, [[maybe_unused]] PyObject *const *args, [[maybe_unused]] Py_ssize_t nargs, [[maybe_unused]] PyObject *kwnames) {
    WithCaravanGL(m, gl) {
        if (gl.ClearBufferfv == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "GL not initialized");
            return nullptr;
        }
        const GLfloat clear_color[] = {0.1f, 0.2f, 0.3f, 1.0f};
        gl.ClearBufferfv(GL_COLOR, 0, clear_color);
        Py_RETURN_NONE;
    }
    return nullptr;
}

static PyMethodDef caravan_methods[] = {
    {"init", (PyCFunction)(void(*)(void))caravan_init, METH_FASTCALL | METH_KEYWORDS, "Initialize loader"},
    {"context", (PyCFunction)(void(*)(void))caravan_context, METH_FASTCALL | METH_KEYWORDS, "Get capabilities"},
    {"test_render", (PyCFunction)(void(*)(void))caravan_test_render, METH_FASTCALL | METH_KEYWORDS, "Test render"},
    {nullptr, nullptr, 0, nullptr}
};

// -----------------------------------------------------------------------------
// Module Lifecycle
// -----------------------------------------------------------------------------

static int caravan_exec(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state == nullptr) return -1;
    
    memset(state, 0, sizeof(CaravanState));
    
    // Initialize the FastParse registry
    caravan_init_parsers(&state->parsers);

    // Register the Buffer heap type
    state->BufferType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Buffer_spec, nullptr);
    if (!state->BufferType) return -1;
    if (PyModule_AddObjectRef(m, "Buffer", (PyObject *)state->BufferType) < 0) return -1;

    return 0;
}

static int caravan_traverse(PyObject *m, visitproc visit, void *arg) {
    CaravanState *state = get_caravan_state(m);
    if (state && state->BufferType) Py_VISIT(state->BufferType);
    return 0;
}

static int caravan_clear(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        caravan_free_parsers(&state->parsers);
        Py_CLEAR(state->BufferType);
    }
    PyErr_Occurred();
    return 0;
}

static PyModuleDef_Slot caravan_slots[] = {
    {Py_mod_exec, caravan_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, nullptr}
};

static struct PyModuleDef caravan_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "caravangl",
    .m_doc = "CaravanGL: Modern Isolated OpenGL Context",
    .m_size = sizeof(CaravanState),
    .m_methods = caravan_methods,
    .m_slots = caravan_slots,
    .m_traverse = caravan_traverse,
    .m_clear = caravan_clear,
    .m_free = nullptr,
};

PyMODINIT_FUNC PyInit_caravangl(void) {
    return PyModuleDef_Init(&caravan_module);
}