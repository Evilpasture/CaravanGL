#include "caravangl_context.h"
#include "pycaravangl.h"

/**
 * Buffer Deallocator: Cleans up GPU resources using the deferred garbage queue.
 */
PyCaravanGL_Slot Buffer_dealloc(PyCaravanBuffer *self) {
    CV_SAFE_DEALLOC(self, buf.id, buffer_count, buffers, OpenGL->DeleteBuffers(1, &self->buf.id));

    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Buffer_traverse(PyCaravanBuffer *self, visitproc visit, void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

static int Buffer_clear(PyCaravanBuffer *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_Status Buffer_init(PyCaravanBuffer *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);
    Py_ssize_t size = 0;
    PyObject *data = nullptr;
    uint32_t target = GL_ARRAY_BUFFER;
    uint32_t usage = GL_STATIC_DRAW;

    void *targets[BufInit_COUNT] = {[IDX_BUF_SIZE] = &size,
                                    [IDX_BUF_DATA] = (void *)&data,
                                    [IDX_BUF_TARGET] = &target,
                                    [IDX_BUF_USAGE] = &usage};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.BufInitParser, targets)) {
        return -1;
    }

    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);

        OpenGL->GenBuffers(1, &self->buf.id);
        OpenGL->BindBuffer(target, self->buf.id);

        // 1. Resolve Data Pointer and Size
        const void *ptr = nullptr;
        Py_buffer view;
        if (data && data != Py_None) {
            if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) == 0) {
                ptr = view.buf;
                // Auto-infer size if not provided
                if (size <= 0) {
                    size = (int)view.len;
                }
            } else {
                return -1;
            }
        }

        bool success = false;

        // 2. Try High-Performance Path (OpenGL 4.4+ / Windows & Linux)
#ifndef __APPLE__
        if (OpenGL->BufferStorage != nullptr) {
            // flags: allow glBufferSubData + Persistent Mapping + Coherent (no CPU flush needed)
            GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
                               GL_MAP_COHERENT_BIT;

            OpenGL->BufferStorage(target, (GLsizeiptr)size, ptr, flags);
            self->buf.is_immutable = true;
            self->buf.is_persistent = true;
            success = true;
        }
#endif

        // 3. Fallback Path (OpenGL 3.3 / macOS / Legacy Hardware)
        if (!success) {
            OpenGL->BufferData(target, (GLsizeiptr)size, ptr, usage);
            self->buf.is_immutable = false;
            self->buf.is_persistent = false;
        }

        // 4. Cleanup and Metadata
        if (ptr) {
            PyBuffer_Release(&view);
        }

        self->buf.target = target;
        self->buf.size = (GLsizeiptr)size;
        self->buf.usage = usage;
    }
    return 0;
}

PyCaravanGL_API Buffer_map(PyCaravanBuffer *self, [[maybe_unused]] PyObject *args) {
    // macOS Safety Check
    if (!self->buf.is_persistent) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Persistent Mapping is not supported on this platform (macOS/Old GL). "
                        "Use .write() instead.");
        return nullptr;
    }

    if (!self->buf.is_immutable) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Only immutable buffers (Storage) can be persistently mapped.");
        return nullptr;
    }

    WithActiveGL(OpenGL, cv_state, nullptr) {
        OpenGL->BindBuffer(self->buf.target, self->buf.id);

        // Map for Write, Persistent, and Coherent (No manual flushing needed)
        void *ptr =
            OpenGL->MapBufferRange(self->buf.target, 0, self->buf.size,
                                   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        if (!ptr) {
            PyErr_SetString(PyExc_RuntimeError, "GPU rejected mapping request.");
            return nullptr;
        }

        Py_buffer view;
        view.buf = ptr;
        view.obj = (PyObject *)self;
        view.len = self->buf.size;
        view.readonly = 0;
        view.itemsize = 1;
        view.format = "B";
        view.ndim = 1;
        self->map_shape[0] = (Py_ssize_t)self->buf.size;
        view.shape = self->map_shape;
        view.strides = &view.itemsize;
        view.suboffsets = nullptr;
        view.internal = nullptr;

        return PyMemoryView_FromBuffer(&view);
    }
    return nullptr;
}

PyCaravanGL_API Buffer_write(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs,
                             PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    PyObject *data = nullptr;
    int offset = 0;
    void *targets[BufWrite_COUNT] = {[IDX_BUF_WRITE_DATA] = (void *)&data,
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

    // UPDATED: Use WithActiveGL
    WithActiveGL(OpenGL, cv_state, nullptr) {
        OpenGL->BindBuffer(self->buf.target, self->buf.id);
        OpenGL->BufferSubData(self->buf.target, (GLintptr)offset, (GLsizeiptr)view.len, view.buf);
        PyBuffer_Release(&view);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Buffer_bind_base(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs,
                                 PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    uint32_t index = 0;
    void *targets[BufBind_COUNT] = {[IDX_BUF_BIND_IDX] = &index};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufBindParser, targets)) {
        return nullptr;
    }

    // UPDATED: Use WithActiveGL
    WithActiveGL(OpenGL, cv_state, nullptr) {
        OpenGL->BindBufferBase(self->buf.target, index, self->buf.id);
    }
    Py_RETURN_NONE;
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Buffer_spec = {
    .name = "caravangl.Buffer",
    .basicsize = sizeof(PyCaravanBuffer),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, Buffer_init},
            {Py_tp_dealloc, Buffer_dealloc},
            {Py_tp_traverse, Buffer_traverse},
            {Py_tp_clear, Buffer_clear},
            {Py_tp_methods,
             (PyMethodDef[]){

                 {"write", CARAVAN_CAST(Buffer_write), METH_FASTCALL | METH_KEYWORDS,
                  "Write data to buffer."},
                 {"bind_base", CARAVAN_CAST(Buffer_bind_base), METH_FASTCALL | METH_KEYWORDS,
                  "Bind as indexed resource."},
                 {"map", (PyCFunction)Buffer_map, METH_NOARGS, "Map to buffer."},
                 {}}

            },
            {Py_tp_doc, "CaravanGL Buffer Object (VBO, IBO, UBO)"},
            {}

        },
};