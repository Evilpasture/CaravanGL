// --- START OF FILE caravangl.c ---

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Buffer Object Implementation (Heap Type)
// -----------------------------------------------------------------------------

static int Buffer_init(PyCaravanBuffer *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"size", "data", "target", "usage", nullptr};
    Py_ssize_t size = 0;
    Py_buffer view = {0};
    GLuint target = GL_ARRAY_BUFFER;
    GLuint usage = GL_STATIC_DRAW;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "n|y*II", kwlist, &size, &view, &target, &usage)) {
        return -1;
    }

    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        gl.GenBuffers(1, &self->buf.id);
        gl.BindBuffer(target, self->buf.id);
        
        const void *ptr = (view.obj) ? view.buf : nullptr;
        gl.BufferData(target, (GLsizeiptr)size, ptr, usage);
        
        self->buf.target = target;
        self->buf.size = (GLsizeiptr)size;
        self->buf.usage = usage;
        self->buf.is_immutable = false;
    }

    if (view.obj) PyBuffer_Release(&view);
    return 0;
}

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

static PyObject* Buffer_write(PyCaravanBuffer *self, PyObject *args) {
    Py_buffer view;
    Py_ssize_t offset = 0;

    if (!PyArg_ParseTuple(args, "y*|n", &view, &offset)) return nullptr;

    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        if (offset + view.len > self->buf.size) {
            PyErr_SetString(PyExc_ValueError, "Data exceeds buffer size");
            PyBuffer_Release(&view);
            return nullptr;
        }
        gl.BindBuffer(self->buf.target, self->buf.id);
        gl.BufferSubData(self->buf.target, (GLintptr)offset, (GLsizeiptr)view.len, view.buf);
    }

    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

static PyObject* Buffer_bind_base(PyCaravanBuffer *self, PyObject *arg) {
    GLuint index = (GLuint)PyLong_AsUnsignedLong(arg);
    if (PyErr_Occurred()) return nullptr;

    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        gl.BindBufferBase(self->buf.target, index, self->buf.id);
    }
    Py_RETURN_NONE;
}

static PyMethodDef Buffer_methods[] = {
    {"write", (PyCFunction)Buffer_write, METH_VARARGS, "Write data to buffer at optional offset."},
    {"bind_base", (PyCFunction)Buffer_bind_base, METH_O, "Bind as an indexed resource (UBO/SSBO)."},
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
// Module Functions
// -----------------------------------------------------------------------------

static void query_capabilities(PyObject *m) {
    WithCaravanGL(m, gl) {
        if (gl.GetIntegerv) {
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

static PyObject *caravan_init(PyObject *m, PyObject *const *args, Py_ssize_t nargs) {
    if (nargs < 1) {
        PyErr_SetString(PyExc_TypeError, "init() requires a loader argument");
        return nullptr;
    }
    
    WithCaravanGL(m, gl) {
        if (load_gl(state, args[0]) < 0) {
            return nullptr; 
        }
        query_capabilities(m);
    }
    Py_RETURN_NONE;
}

static PyObject *caravan_context(PyObject *m, PyObject *const *args, Py_ssize_t nargs) {
    WithCaravanGL(m, gl) {
        if (!gl.GetString) {
            PyErr_SetString(PyExc_RuntimeError, "OpenGL not loaded. Call init() first.");
            return nullptr;
        }

        PyObject *caps = PyDict_New();
        PyDict_SetItemString(caps, "max_texture_size", PyLong_FromLong(state->ctx.caps.max_texture_size));
        PyDict_SetItemString(caps, "max_samples", PyLong_FromLong(state->ctx.caps.max_samples));
        PyDict_SetItemString(caps, "support_compute", PyBool_FromLong(state->ctx.caps.support_compute));
        PyDict_SetItemString(caps, "support_bindless", PyBool_FromLong(state->ctx.caps.support_bindless));

        PyObject *info = PyDict_New();
        const char* vendor = (const char*)gl.GetString(GL_VENDOR);
        const char* renderer = (const char*)gl.GetString(GL_RENDERER);
        PyDict_SetItemString(info, "vendor", PyUnicode_FromString(vendor ? vendor : "Unknown"));
        PyDict_SetItemString(info, "renderer", PyUnicode_FromString(renderer ? renderer : "Unknown"));

        PyObject *result = PyDict_New();
        PyDict_SetItemString(result, "caps", caps);
        PyDict_SetItemString(result, "info", info);
        
        Py_DECREF(caps);
        Py_DECREF(info);
        return result;
    }
    
    PyErr_SetString(PyExc_RuntimeError, "Failed to get CaravanGL State");
    return nullptr;
}

static PyObject *caravan_test_render(PyObject *m, PyObject *const *args, Py_ssize_t nargs) {
    WithCaravanGL(m, gl) {
        if (!gl.ClearBufferfv) {
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
    {"init", (PyCFunction)caravan_init, METH_FASTCALL, "Initialize loader"},
    {"context", (PyCFunction)caravan_context, METH_FASTCALL, "Get capabilities"},
    {"test_render", (PyCFunction)caravan_test_render, METH_FASTCALL, "Test render"},
    {nullptr, nullptr, 0, nullptr}
};

// -----------------------------------------------------------------------------
// Module Lifecycle Slots
// -----------------------------------------------------------------------------

static int caravan_exec(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (!state) return -1;
    
    memset(state, 0, sizeof(CaravanState));
    state->ctx.last_error = GL_NO_ERROR;

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
    if (state) Py_CLEAR(state->BufferType);
    return 0;
}

static PyModuleDef_Slot caravan_slots[] = {
    {Py_mod_exec, caravan_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED}, // Boom. GIL disabled.
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