#include "caravangl_context.h"
#include "caravangl_module.h"
#include "pycaravangl.h"
// clang-format off
#include "caravangl_loader.h"
// clang-format on

thread_local PyCaravanContext *cv_active_context = nullptr;

PyCaravanGL_API Context_make_current(PyCaravanContext *self, [[maybe_unused]] PyObject *args) {
    // 1. Call Python-only callbacks first (e.g. glfwMakeContextCurrent)
    if (self->os_make_current_cb && self->os_make_current_cb != Py_None) {
        PyObject *res = PyObject_CallNoArgs(self->os_make_current_cb);
        if (!res)
            return nullptr;
        Py_DECREF(res);
    }

    // 2. Synchronize Python TLS
    cv_active_context = self;

    // 3. Trigger Core Logic (Handles the mutex and garbage flush internally)
    caravan_make_current(&self->handle);

    Py_RETURN_NONE;
}

PyCaravanGL_API Context_enter(PyCaravanContext *self, [[maybe_unused]] PyObject *args) {
    if (Context_make_current(self, nullptr) == nullptr) {
        return nullptr;
    }
    return Py_NewRef(self);
}

PyCaravanGL_API Context_exit(PyCaravanContext *self, [[maybe_unused]] PyObject *args) {
    if (self->os_release_cb && self->os_release_cb != Py_None) {
        PyObject *res = PyObject_CallNoArgs(self->os_release_cb);
        Py_XDECREF(res);
    }
    // Clear BOTH pointers on exit
    cv_active_context = nullptr;
    cv_current_handle = nullptr;

    Py_RETURN_NONE;
}

PyCaravanGL_Status Context_init(PyCaravanContext *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    PyObject *loader = nullptr;
    PyObject *callback = Py_None;
    PyObject *release_cb = Py_None;

    void *targets[ContextInit_COUNT] = {[IDX_CTX_LOADER] = (void *)&loader,
                                        [IDX_CTX_CALLBACK] = (void *)&callback,
                                        [IDX_CTX_RELEASE_CB] = (void *)&release_cb};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.ContextInitParser, targets)) {
        return -1;
    }

    // 1. Core Init (Pure C)
    caravan_init_handle(&self->handle);

    // 2. Load OpenGL functions via the Shim
    if (load_gl_table(&self->handle.gl, loader) < 0) {
        return -1;
    }

    // 3. Core Capability Query (Pure C)
    caravan_query_caps(&self->handle);

    // 4. Handle the Python Callbacks
    self->os_make_current_cb = Py_NewRef(callback);
    self->os_release_cb = Py_NewRef(release_cb);

    return 0;
}

PyCaravanGL_Slot Context_dealloc(PyCaravanContext *self) {
    PyTypeObject *type = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    Py_CLEAR(self->os_make_current_cb);
    Py_CLEAR(self->os_release_cb);

    type->tp_free((PyObject *)self);
    Py_DECREF(type);
}

PyCaravanGL_Status Context_traverse(PyCaravanContext *self, visitproc visit, void *arg) {
    Py_VISIT(self->os_make_current_cb);
    return 0;
}

PyCaravanGL_Status Context_clear(PyCaravanContext *self) {
    Py_CLEAR(self->os_make_current_cb);
    return 0;
}

// Getter for os_make_current_cb
PyCaravanGL_API Context_get_os_make_current_cb(PyCaravanContext *self,
                                               [[maybe_unused]] void *closure) {
    return Py_NewRef(self->os_make_current_cb);
}

// Setter for os_make_current_cb
PyCaravanGL_Status Context_set_os_make_current_cb(PyCaravanContext *self, PyObject *value,
                                                  [[maybe_unused]] void *closure) {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete os_make_current_cb");
        return -1;
    }
    Py_XSETREF(self->os_make_current_cb, Py_NewRef(value));
    return 0;
}

// NEW: Getter for os_release_cb
PyCaravanGL_API Context_get_os_release_cb(PyCaravanContext *self, [[maybe_unused]] void *closure) {
    return Py_NewRef(self->os_release_cb);
}

// NEW: Setter for os_release_cb
PyCaravanGL_Status Context_set_os_release_cb(PyCaravanContext *self, PyObject *value,
                                             [[maybe_unused]] void *closure) {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete os_release_cb");
        return -1;
    }
    Py_XSETREF(self->os_release_cb, Py_NewRef(value));
    return 0;
}

#define CONTEXT_NOARGS(name)                                                                       \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(Context_, name)), METH_NOARGS, nullptr}
#define CONTEXT_FASTCALL(name)                                                                     \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(Context_, name)), METH_FASTCALL | METH_KEYWORDS, nullptr}
#define CONTEXT_O(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Context_, name)), METH_O, nullptr}

// For Read/Write
#define CONTEXT_GETSET(name)                                                                       \
    {#name, (getter)CARAVAN_JOIN(Context_get_, name), (setter)CARAVAN_JOIN(Context_set_, name),    \
     nullptr, nullptr}

// For Read-Only
#define CONTEXT_GET(name)                                                                          \
    {#name, (getter)CARAVAN_JOIN(Context_get_, name), nullptr, nullptr, nullptr}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Context_spec = {
    .name = "caravangl.Context",
    .basicsize = sizeof(PyCaravanContext),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, Context_init},
            {Py_tp_dealloc, Context_dealloc},
            {Py_tp_getset,
             (PyGetSetDef[]){

                 CONTEXT_GETSET(os_make_current_cb), CONTEXT_GETSET(os_release_cb), {}

             }

            },
            {Py_tp_methods,
             (PyMethodDef[]){

                 CONTEXT_NOARGS(make_current),
                 // No macro for these methods!
                 {"__enter__", (PyCFunction)Context_enter, METH_NOARGS, nullptr},
                 {"__exit__", (PyCFunction)Context_exit, METH_VARARGS,
                  nullptr}, // __exit__ takes 3 args
                 {}

             }

            },
            {Py_tp_traverse, Context_traverse},
            {Py_tp_clear, Context_clear},
            {}

        }

};