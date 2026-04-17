#include "caravangl_context.h"
#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// VertexArray Object
// -----------------------------------------------------------------------------

PyCaravanGL_Status VertexArray_init(PyCaravanVertexArray *self, [[maybe_unused]] PyObject *args) {
    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        OpenGL->GenVertexArrays(1, &self->id);
    }
    return 0;
}

PyCaravanGL_Slot VertexArray_dealloc(PyCaravanVertexArray *self) {
    CV_SAFE_DEALLOC(self, id, vao_count, vaos, OpenGL->DeleteVertexArrays(1, &self->id));
    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyCaravanGL_API VertexArray_bind_attribute(PyCaravanVertexArray *self, PyObject *const *args,
                                           Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *module = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(module);

    uint32_t location = 0;
    uint32_t type = GL_FLOAT;
    int size = 0;
    int normalized = 0;
    int stride = 0;
    uintptr_t offset = 0;
    PyObject *py_buffer = nullptr;
    uint32_t divisor = 0;

    void *targets[VaoAttr_COUNT] = {
        [IDX_VAO_ATTR_LOC] = &location,    [IDX_VAO_ATTR_BUF] = (void *)&py_buffer,
        [IDX_VAO_ATTR_SIZE] = &size,       [IDX_VAO_ATTR_TYPE] = &type,
        [IDX_VAO_ATTR_NORM] = &normalized, [IDX_VAO_ATTR_STRIDE] = &stride,
        [IDX_VAO_ATTR_OFFSET] = &offset,   [IDX_VAO_ATTR_DIV] = &divisor};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.VaoAttrParser, targets)) {
        return nullptr;
    }

    // 1. Strict Type Check for the Buffer
    if (!Py_IS_TYPE(py_buffer, state->BufferType)) {
        PyErr_Format(PyExc_TypeError, "buffer must be a caravangl.Buffer, not %.200s",
                     Py_TYPE(py_buffer)->tp_name);
        return nullptr;
    }
    PyCaravanBuffer *buf = (PyCaravanBuffer *)py_buffer;

    // 2. Context Safety
    WithActiveGL(OpenGL, cv_state, nullptr) {
        // IMPORTANT: VAOs are container objects that belong to a specific context.
        // They are NOT shared. If this context isn't the one that created the VAO, crash.
        if (_cv_ctx != self->owning_context) [[clang::unlikely]] {
            PyErr_SetString(PyExc_RuntimeError,
                            "VertexArray cannot be modified by a context other than its creator.");
            return nullptr;
        }

        OpenGL->BindVertexArray(self->id);

        // VBOs are shared across contexts, so buf->buf.id is safe to use here.
        OpenGL->BindBuffer(GL_ARRAY_BUFFER, buf->buf.id);

        OpenGL->EnableVertexAttribArray(location);
        OpenGL->VertexAttribPointer(location, size, type, normalized ? GL_TRUE : GL_FALSE, stride,
                                    IntToPtr(offset));

        OpenGL->VertexAttribDivisor(location, divisor);

        // Clean up binding state to prevent accidental state corruption
        OpenGL->BindVertexArray(0);
        OpenGL->BindBuffer(GL_ARRAY_BUFFER, 0);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API VertexArray_bind_index_buffer(PyCaravanVertexArray *self, PyObject *arg) {
    PyObject *module = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(module);

    if (!Py_IS_TYPE(arg, state->BufferType)) {
        PyErr_SetString(PyExc_TypeError, "Expected a caravangl.Buffer");
        return nullptr;
    }
    PyCaravanBuffer *buf = (PyCaravanBuffer *)arg;

    WithActiveGL(OpenGL, cv_state, nullptr) {
        if (_cv_ctx != self->owning_context) [[clang::unlikely]] {
            PyErr_SetString(PyExc_RuntimeError, "VAO context mismatch.");
            return nullptr;
        }

        OpenGL->BindVertexArray(self->id);

        // GL_ELEMENT_ARRAY_BUFFER binding is stored INSIDE the VAO state.
        OpenGL->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->buf.id);

        OpenGL->BindVertexArray(0);
    }
    Py_RETURN_NONE;
}

static const PyMethodDef VertexArray_methods[] = {
    {"bind_attribute", (PyCFunction)(void (*)(void))VertexArray_bind_attribute,
     METH_FASTCALL | METH_KEYWORDS, "Map a VBO to a shader attribute"},
    {"bind_index_buffer", (PyCFunction)(void (*)(void))VertexArray_bind_index_buffer, METH_O,
     nullptr},
    {}};

static const PyType_Slot VertexArray_slots[] = {{Py_tp_init, VertexArray_init},
                                                {Py_tp_dealloc, VertexArray_dealloc},
                                                {Py_tp_methods, (PyMethodDef *)VertexArray_methods},
                                                {}};
// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec VertexArray_spec = {
    .name = "caravangl.VertexArray",
    .basicsize = sizeof(PyCaravanVertexArray),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot *)VertexArray_slots,
};