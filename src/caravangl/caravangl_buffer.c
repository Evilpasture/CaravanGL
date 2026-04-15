#include "caravangl_buffer.h"
#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Buffer Object Implementation (Heap Type)
// -----------------------------------------------------------------------------

/**
 * Buffer Deallocator: Cleans up GPU resources before the Python object is freed.
 */
PyCaravanGL_Slot Buffer_dealloc(PyCaravanBuffer *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);

    WithCaravanGL(m, gl)
    {
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
PyCaravanGL_Status Buffer_init(PyCaravanBuffer *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));

    WithCaravanGL(m, gl)
    {
        int size = 0;
        PyObject *data = nullptr;
        uint32_t target = GL_ARRAY_BUFFER;
        uint32_t usage = GL_STATIC_DRAW;

        void *targets[BufInit_COUNT] = {[IDX_BUF_SIZE] = &size,
                                        [IDX_BUF_DATA] = &data,
                                        [IDX_BUF_TARGET] = &target,
                                        [IDX_BUF_USAGE] = &usage};

        // tp_init call: (args, kwds, kwnames=nullptr, parser, targets)
        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.BufInitParser, targets)) {
            return -1;
        }

        gl.GenBuffers(1, &self->buf.id);
        gl.BindBuffer(target, self->buf.id);

        const void *ptr = nullptr;
        Py_buffer view;
        if (data && data != Py_None) {
            if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) == 0) {
                ptr = view.buf;
            } else {
                return -1; // Fail initialization if buffer extraction fails
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
PyCaravanGL_API Buffer_write(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs,
                              PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));

    WithCaravanGL(m, gl)
    {
        PyObject *data = nullptr;
        int offset = 0;
        void *targets[BufWrite_COUNT] = {[IDX_BUF_WRITE_DATA] = &data,
                                         [IDX_BUF_WRITE_OFFSET] = &offset};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufWriteParser, targets)) {
            return nullptr;
        }

        Py_buffer view;
        if (data == nullptr || PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) != 0) {
            PyErr_SetString(PyExc_TypeError, "data must support the buffer protocol");
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
PyCaravanGL_API Buffer_bind_base(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs,
                                  PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));

    WithCaravanGL(m, gl)
    {
        uint32_t index = 0;
        void *targets[BufBind_COUNT] = {[IDX_BUF_BIND_IDX] = &index};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufBindParser, targets)) {
            return nullptr;
        }
        gl.BindBufferBase(self->buf.target, index, self->buf.id);
    }
    Py_RETURN_NONE;
}

PyMethodDef Buffer_methods[] = {{"write", (PyCFunction)(void (*)(void))Buffer_write,
                                        METH_FASTCALL | METH_KEYWORDS, "Write data to buffer."},
                                       {"bind_base", (PyCFunction)(void (*)(void))Buffer_bind_base,
                                        METH_FASTCALL | METH_KEYWORDS, "Bind as indexed resource."},
                                       {nullptr}};

PyType_Slot Buffer_slots[] = {{Py_tp_init, Buffer_init},
                                     {Py_tp_dealloc, Buffer_dealloc},
                                     {Py_tp_methods, Buffer_methods},
                                     {Py_tp_doc, "CaravanGL Buffer Object (VBO, IBO, UBO)"},
                                     {0, nullptr}};

PyType_Spec Buffer_spec = {
    .name = "caravangl.Buffer",
    .basicsize = sizeof(PyCaravanBuffer),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Buffer_slots,
};