#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// UniformBatch Object (Zero-Copy Command Stream)
// -----------------------------------------------------------------------------

PyCaravanGL_Status UniformBatch_init(PyCaravanUniformBatch *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    int max_binds = 0;
    int max_bytes = 0;
    void *targets[UniformBatchInit_COUNT] = {[IDX_UB_MAX_BINDS] = &max_binds,
                                             [IDX_UB_MAX_BYTES] = &max_bytes};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.UniformBatchInitParser, targets)) {
        return -1;
    }

    // Allocate the contiguous header (size + array of bindings)
    size_t header_size = sizeof(CaravanUniformHeader) + (sizeof(CaravanUniformBinding) * max_binds);
    self->header = (CaravanUniformHeader *)PyMem_Malloc(header_size);
    self->header->count = 0;

    self->payload = (char *)PyMem_Calloc(1, max_bytes); // Zeroed out memory
    self->max_bindings = max_binds;
    self->max_payload_bytes = max_bytes;
    self->current_payload_offset = 0;

    // Set up the memory view for Python (exposing it as raw bytes 'B')
    self->payload_buffer.buf = self->payload;
    self->payload_buffer.len = max_bytes;
    self->payload_buffer.readonly = 0;
    self->payload_buffer.itemsize = 1;
    self->payload_buffer.format = "B";
    self->payload_buffer.ndim = 1;

    static Py_ssize_t ub_stride = 1;
    self->payload_buffer.shape = (Py_ssize_t *)&self->max_payload_bytes;
    self->payload_buffer.strides = &ub_stride;

    self->payload_buffer.obj = (PyObject *)self;
    Py_INCREF(self);

    self->payload_view = PyMemoryView_FromBuffer(&self->payload_buffer);
    return 0;
}

PyCaravanGL_Slot UniformBatch_dealloc(PyCaravanUniformBatch *self) {
    PyTypeObject *tp = Py_TYPE(self);
    Py_XDECREF(self->payload_view);
    if (self->header) {
        PyMem_Free(self->header);
    }
    if (self->payload) {
        PyMem_Free(self->payload);
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

PyCaravanGL_API UniformBatch_add(PyCaravanUniformBatch *self, PyObject *const *args,
                                 Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    uint32_t func_id = 0;
    int location = 0;
    int count = 0;
    int size = 0;

    void *targets[UniformBatchAdd_COUNT] = {[IDX_UB_ADD_FUNC] = &func_id,
                                            [IDX_UB_ADD_LOC] = &location,
                                            [IDX_UB_ADD_CNT] = &count,
                                            [IDX_UB_ADD_SIZE] = &size};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.UniformBatchAddParser, targets)) {
        return nullptr;
    }

    if (self->header->count >= self->max_bindings) {
        PyErr_SetString(PyExc_RuntimeError, "UniformBatch maximum bindings exceeded.");
        return nullptr;
    }
    if (self->current_payload_offset + size > self->max_payload_bytes) {
        PyErr_SetString(PyExc_RuntimeError, "UniformBatch payload capacity exceeded.");
        return nullptr;
    }

    uint32_t offset = self->current_payload_offset;

    // Register the binding in the C header
    CaravanUniformBinding *binding = &self->header->bindings[self->header->count++];
    binding->function_id = func_id;
    binding->location = location;
    binding->count = count;
    binding->offset = offset;

    // Advance the memory allocator
    self->current_payload_offset += size;

    // Return the byte offset to Python so it knows where to write the data
    return PyLong_FromUnsignedLong(offset);
}

PyCaravanGL_API UniformBatch_get_data(PyCaravanUniformBatch *self, [[maybe_unused]] void *closure) {
    return Py_NewRef(self->payload_view);
}

static const PyGetSetDef UniformBatch_getset[] = {{"data", (getter)UniformBatch_get_data, nullptr,
                                                   "Zero-copy access to the uniform payload memory",
                                                   nullptr},
                                                  {}};

static const PyMethodDef UniformBatch_methods[] = {
    {"add", (PyCFunction)(void (*)(void))UniformBatch_add, METH_FASTCALL | METH_KEYWORDS,
     "Register a uniform and get its byte offset."},
    {}};

static const PyType_Slot UniformBatch_slots[] = {
    {Py_tp_init, UniformBatch_init},
    {Py_tp_dealloc, UniformBatch_dealloc},
    {Py_tp_methods, (PyMethodDef *)UniformBatch_methods},
    {Py_tp_getset, (PyGetSetDef *)UniformBatch_getset},
    {}};
// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec UniformBatch_spec = {
    .name = "caravangl.UniformBatch",
    .basicsize = sizeof(PyCaravanUniformBatch),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot *)UniformBatch_slots,
};