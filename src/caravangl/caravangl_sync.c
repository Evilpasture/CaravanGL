#include "caravangl_context.h"
#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Sync Object (GPU/CPU Fence)
// -----------------------------------------------------------------------------

PyCaravanGL_Status Sync_init(PyCaravanSync *self, [[maybe_unused]] PyObject *args) {
    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        // Insert the fence into the GPU command stream immediately
        self->sync_obj = OpenGL->FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    return 0;
}

PyCaravanGL_Slot Sync_dealloc(PyCaravanSync *self) {
    // Custom deferred dealloc because GLsync is a pointer, not a GLuint
    if (self->sync_obj != nullptr && self->owning_context) {
        auto owning_context = self->owning_context;
        if (owning_context == cv_active_context) {
            WithContext(owning_context, OpenGL, _unused) {
                OpenGL->DeleteSync(self->sync_obj);
            }
        } else {
            MagMutex_Lock(&owning_context->ctx.state_lock);
            if (owning_context->garbage.sync_count < CARAVAN_GARBAGE_SIZE) {
                owning_context->garbage.syncs[owning_context->garbage.sync_count++] =
                    self->sync_obj;
            } else {
                // Garbage queue full fallback
                // NOLINTNEXTLINE
                (void)fprintf(stderr, "[CaravanGL] Warning: Sync garbage queue full.\n");
            }
            MagMutex_Unlock(&owning_context->ctx.state_lock);
        }
        self->sync_obj = nullptr;
    }

    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyCaravanGL_Status Sync_traverse(PyCaravanSync *self, visitproc visit, void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

PyCaravanGL_Status Sync_clear(PyCaravanSync *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_API Sync_wait(PyCaravanSync *self, PyObject *const *args, Py_ssize_t nargs,
                          PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    // Default to -1.0 (which we treat as None/Infinity)
    float sec = -1.0f;
    void *targets[SyncWait_COUNT] = {[IDX_SYNC_WAIT_SEC] = &sec};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.SyncWaitParser, targets)) {
        return nullptr;
    }

    GLuint64 timeout_ns = GL_TIMEOUT_IGNORED;
    if (sec >= 0.0f) {
        timeout_ns = (GLuint64)((double)sec * 1e9);
    }

    WithActiveGL(OpenGL, cv_state, nullptr) {
        GLenum res = OpenGL->ClientWaitSync(self->sync_obj, GL_SYNC_FLUSH_COMMANDS_BIT, timeout_ns);
        return PyLong_FromLong(res);
    }
    return nullptr;
}
// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Sync_spec = {
    .name = "caravangl.Sync",
    .basicsize = sizeof(PyCaravanSync),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, Sync_init},
            {Py_tp_dealloc, Sync_dealloc},
            {Py_tp_traverse, Sync_traverse},
            {Py_tp_clear, Sync_clear},
            {Py_tp_methods,
             (PyMethodDef[]){

                 {"wait", (PyCFunction)Sync_wait, METH_FASTCALL | METH_KEYWORDS,
                  "Block thread until GPU finishes prior commands."},
                 {}

             }

            },
            {}

        },
};