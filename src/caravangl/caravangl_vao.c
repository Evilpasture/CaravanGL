#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// VertexArray Object
// -----------------------------------------------------------------------------

PyCaravanGL_Status VertexArray_init(PyCaravanVertexArray *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl)
    {
        gl.GenVertexArrays(1, &self->id);
    }
    return 0;
}

PyCaravanGL_Slot VertexArray_dealloc(PyCaravanVertexArray *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    WithCaravanGL(m, gl)
    {
        if (self->id) gl.DeleteVertexArrays(1, &self->id);
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

PyCaravanGL_API VertexArray_bind_attribute(PyCaravanVertexArray *self, PyObject *const *args,
                                           Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl)
    {
        uint32_t location = 0, type = GL_FLOAT;
        int size = 0, normalized = 0, stride = 0, offset = 0;
        PyObject *py_buffer = nullptr;

        void *targets[VaoAttr_COUNT] = {
            [IDX_VAO_ATTR_LOC] = &location,    [IDX_VAO_ATTR_BUF] = &py_buffer,
            [IDX_VAO_ATTR_SIZE] = &size,       [IDX_VAO_ATTR_TYPE] = &type,
            [IDX_VAO_ATTR_NORM] = &normalized, [IDX_VAO_ATTR_STRIDE] = &stride,
            [IDX_VAO_ATTR_OFFSET] = &offset};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.VaoAttrParser, targets))
            return nullptr;

        // Verify it's actually our Buffer object
        if (Py_TYPE(py_buffer) != state->BufferType) {
            PyErr_SetString(PyExc_TypeError, "buffer must be a caravangl.Buffer");
            return nullptr;
        }
        PyCaravanBuffer *buf = (PyCaravanBuffer *)py_buffer;

        // Bind VAO and VBO, then map the attribute
        gl.BindVertexArray(self->id);
        gl.BindBuffer(GL_ARRAY_BUFFER, buf->buf.id);

        gl.EnableVertexAttribArray(location);
        gl.VertexAttribPointer(location, size, type, normalized ? GL_TRUE : GL_FALSE, stride,
                               (void *)(uintptr_t)offset);

        // Unbind VAO to prevent accidental corruption
        gl.BindVertexArray(0);
    }
    Py_RETURN_NONE;
}

static PyMethodDef VertexArray_methods[] = {
    {"bind_attribute", (PyCFunction)(void (*)(void))VertexArray_bind_attribute,
     METH_FASTCALL | METH_KEYWORDS, "Map a VBO to a shader attribute"},
    {nullptr}};

static PyType_Slot VertexArray_slots[] = {{Py_tp_init, VertexArray_init},
                                          {Py_tp_dealloc, VertexArray_dealloc},
                                          {Py_tp_methods, VertexArray_methods},
                                          {0, nullptr}};

PyType_Spec VertexArray_spec = {
    .name = "caravangl.VertexArray",
    .basicsize = sizeof(PyCaravanVertexArray),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = VertexArray_slots,
};