#include "caravangl_context.h"
#include "pycaravangl.h"

PyCaravanGL_Status Sampler_init(PyCaravanSampler *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    uint32_t min_filter = GL_LINEAR;
    uint32_t mag_filter = GL_LINEAR;
    uint32_t wrap_s = GL_CLAMP_TO_EDGE;
    uint32_t wrap_t = GL_CLAMP_TO_EDGE;

    void *targets[SamplerInit_COUNT] = {[IDX_SAMP_MIN] = &min_filter,
                                        [IDX_SAMP_MAG] = &mag_filter,
                                        [IDX_SAMP_WRAP_S] = &wrap_s,
                                        [IDX_SAMP_WRAP_T] = &wrap_t};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.SamplerInitParser, targets)) {
        return -1;
    }

    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        OpenGL->GenSamplers(1, &self->id);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_MIN_FILTER, (GLint)min_filter);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_MAG_FILTER, (GLint)mag_filter);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_WRAP_S, (GLint)wrap_s);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_WRAP_T, (GLint)wrap_t);
    }
    return 0;
}

static int Sampler_traverse(PyCaravanSampler *self, visitproc visit, void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

static int Sampler_clear(PyCaravanSampler *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_Slot Sampler_dealloc(PyCaravanSampler *self) {
    // 1. Prevent cache poisoning on the currently active context
    PyCaravanContext *active = cv_active_context;
    if (active && self->id != 0) {
        MagMutex_Lock(&active->ctx.state_lock);
#pragma unroll 2
        for (int i = 0; i < CARAVAN_MAX_TEXTURE_UNITS; i++) {
            if (active->ctx.bound.texture_units[i].sampler_id == self->id) {
                active->ctx.bound.texture_units[i].sampler_id = 0;
            }
        }
        MagMutex_Unlock(&active->ctx.state_lock);
    }

    // 2. Thread-safe deferred deletion
    CV_SAFE_DEALLOC(self, id, sampler_count, samplers, OpenGL->DeleteSamplers(1, &self->id));

    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static const PyMethodDef Sampler_methods[] = {{}};

static const PyType_Slot Sampler_slots[] = {{Py_tp_new, (void *)PyType_GenericNew},
                                            {Py_tp_init, Sampler_init},
                                            {Py_tp_dealloc, Sampler_dealloc},
                                            {Py_tp_traverse, Sampler_traverse},
                                            {Py_tp_clear, Sampler_clear},
                                            {Py_tp_methods, (PyMethodDef *)Sampler_methods},
                                            {}};

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Sampler_spec = {
    .name = "caravangl.Sampler",
    .basicsize = sizeof(PyCaravanSampler),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = (PyType_Slot *)Sampler_slots,
};