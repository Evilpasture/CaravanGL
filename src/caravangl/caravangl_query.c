#include "caravangl_context.h"
#include "caravangl_module.h"
#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Query Object (GPU Profiling & Occlusion)
// -----------------------------------------------------------------------------

PyCaravanGL_Status Query_init(PyCaravanQuery *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    uint32_t target = GL_TIME_ELAPSED;
    void *targets[QueryInit_COUNT] = {[IDX_QUERY_TARGET] = &target};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.QueryInitParser, targets)) {
        return -1;
    }

    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        self->target = target;
        OpenGL->GenQueries(1, &self->id);
    }
    return 0;
}

PyCaravanGL_Slot Query_dealloc(PyCaravanQuery *self) {
    CV_SAFE_DEALLOC(self, id, query_count, queries, OpenGL->DeleteQueries(1, &self->id));
    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyCaravanGL_Status Query_traverse(PyCaravanQuery *self, visitproc visit, void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

PyCaravanGL_Status Query_clear(PyCaravanQuery *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_API Query_begin(PyCaravanQuery *self, [[maybe_unused]] PyObject *args) {
    WithActiveGL(OpenGL, cv_state, nullptr) {
        OpenGL->BeginQuery(self->target, self->id);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Query_end(PyCaravanQuery *self, [[maybe_unused]] PyObject *args) {
    WithActiveGL(OpenGL, cv_state, nullptr) {
        OpenGL->EndQuery(self->target);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Query_record_timestamp(PyCaravanQuery *self, [[maybe_unused]] PyObject *args) {
    WithActiveGL(OpenGL, cv_state, nullptr) {
        // GL_TIMESTAMP ignores the internal target and inserts a timestamp marker
        OpenGL->QueryCounter(self->id, GL_TIMESTAMP);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Query_is_ready(PyCaravanQuery *self, [[maybe_unused]] PyObject *args) {
    GLuint available = 0;
    WithActiveGL(OpenGL, cv_state, nullptr) {
        OpenGL->GetQueryObjectuiv(self->id, GL_QUERY_RESULT_AVAILABLE, &available);
    }
    return PyBool_FromLong((long)available);
}

PyCaravanGL_API Query_get_result(PyCaravanQuery *self, [[maybe_unused]] PyObject *args) {
    GLuint64 result = 0;
    WithActiveGL(OpenGL, cv_state, nullptr) {
        // Blocks the CPU if the query isn't ready yet!
        // We use GetQueryObjectui64v because nanoseconds overflow 32-bit ints quickly
        OpenGL->GetQueryObjectui64v(self->id, GL_QUERY_RESULT, &result);
    }
    return PyLong_FromUnsignedLongLong(result);
}

#define QUERY_NOARGS(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Query_, name)), METH_NOARGS, nullptr}
#define QUERY_FASTCALL(name)                                                                       \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(Query_, name)), METH_FASTCALL | METH_KEYWORDS, nullptr}
#define QUERY_O(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Query_, name)), METH_O, nullptr}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Query_spec = {
    .name = "caravangl.Query",
    .basicsize = sizeof(PyCaravanQuery),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, Query_init},
            {Py_tp_dealloc, Query_dealloc},
            {Py_tp_traverse, Query_traverse},
            {Py_tp_clear, Query_clear},
            {Py_tp_methods,
             (PyMethodDef[]){

                 QUERY_NOARGS(begin),
                 QUERY_NOARGS(end),
                 QUERY_NOARGS(record_timestamp),
                 QUERY_NOARGS(is_ready),
                 QUERY_NOARGS(get_result),
                 {}

             }

            },
            {}

        },
};