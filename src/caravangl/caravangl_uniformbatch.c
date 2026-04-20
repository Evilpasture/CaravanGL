#include "fast_build.h"
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

    // SAFEGUARD: Abort cleanly to Python if Out of Memory
    if (self->header == nullptr) {
        PyErr_NoMemory();
        return -1;
    }
    self->header->count = 0;

    self->payload = (char *)PyMem_Calloc(1, max_bytes);

    // SAFEGUARD: Free previous allocation and throw error
    if (self->payload == nullptr) {
        PyMem_Free(self->header);
        self->header = nullptr;
        PyErr_NoMemory();
        return -1;
    }

    self->max_bindings = max_binds;
    self->max_payload_bytes = max_bytes;
    self->current_payload_offset = 0;
    return 0;
}

PyCaravanGL_Slot UniformBatch_dealloc(PyCaravanUniformBatch *self) {
    PyTypeObject *type = Py_TYPE(self);
    if (self->header) {
        PyMem_Free(self->header);
    }
    if (self->payload) {
        PyMem_Free(self->payload);
    }
    type->tp_free((PyObject *)self);
    Py_DECREF(type);
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
    return FastBuild_Value(offset);
}

PyCaravanGL_API UniformBatch_get_data(PyCaravanUniformBatch *self, [[maybe_unused]] void *closure) {
    Py_buffer view;
    Py_ssize_t shape = (Py_ssize_t)self->max_payload_bytes;

    view.buf = self->payload;
    view.obj = (PyObject *)self; // This object "owns" the memory
    view.len = shape;
    view.readonly = 0; // Set to 1 if you want the buffer to be read-only
    view.itemsize = 1;
    view.format = "B"; // "B" is unsigned byte
    view.ndim = 1;
    view.shape = nullptr;
    view.strides = nullptr;
    view.suboffsets = nullptr;
    view.internal = nullptr;

    // This creates the view and increments Py_REFCOUNT(self)
    return PyMemoryView_FromBuffer(&view);
}

#define UNIB_NOARGS(name)                                                                          \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(UniformBatch_, name)), METH_NOARGS, nullptr}
#define UNIB_FASTCALL(name)                                                                        \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(UniformBatch_, name)), METH_FASTCALL | METH_KEYWORDS, nullptr}
#define UNIB_O(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(UniformBatch_, name)), METH_O, nullptr}
// For Read/Write
#define UNIB_GETSET(name)                                                                          \
    {#name, (getter)CARAVAN_JOIN(UniformBatch_get_, name),                                         \
     (setter)CARAVAN_JOIN(UniformBatch_set_, name), nullptr, nullptr}

// For Read-Only
#define UNIB_GET(name)                                                                             \
    {#name, (getter)CARAVAN_JOIN(UniformBatch_get_, name), nullptr, nullptr, nullptr}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec UniformBatch_spec = {
    .name = "caravangl.UniformBatch",
    .basicsize = sizeof(PyCaravanUniformBatch),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, UniformBatch_init},
            {Py_tp_dealloc, UniformBatch_dealloc},
            {Py_tp_methods,
             (PyMethodDef[]){

                 UNIB_FASTCALL(add), {}

             }

            },
            {Py_tp_getset,
             (PyGetSetDef[]){

                 UNIB_GET(data), {}

             }

            },
            {}

        },
};