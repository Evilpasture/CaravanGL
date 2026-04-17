#include "caravangl_state.h"
#include "pycaravangl.h"

PyCaravanGL_Status Sampler_init(PyCaravanSampler *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    // Sane defaults (same as what you used in Texture_init)
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

    WithCaravanGL(mod, OpenGL) {
        OpenGL->GenSamplers(1, &self->id);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_MIN_FILTER, (GLint)min_filter);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_MAG_FILTER, (GLint)mag_filter);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_WRAP_S, (GLint)wrap_s);
        OpenGL->SamplerParameteri(self->id, GL_TEXTURE_WRAP_T, (GLint)wrap_t);
    }
    return 0;
}

PyCaravanGL_Slot Sampler_dealloc(PyCaravanSampler *self) {
    PyTypeObject *type = Py_TYPE(self);
    PyObject *mod = PyType_GetModule(type);

    WithCaravanGL(mod, OpenGL) {
        if (self->id) {
// FIX: Cache Safety! If this sampler is bound, we must unbind it in our tracker.
#pragma unroll 2
            for (int i = 0; i < CARAVAN_MAX_TEXTURE_UNITS; i++) {
                if (state->ctx.bound.texture_units[i].sampler_id == self->id) {
                    state->ctx.bound.texture_units[i].sampler_id = 0;
                }
            }
            OpenGL->DeleteSamplers(1, &self->id);
            self->id = 0;
        }
    }
    type->tp_free((PyObject *)self);
    Py_DECREF(type);
}

static const PyMethodDef Sampler_methods[] = {{}};

static const PyType_Slot Sampler_slots[] = {{Py_tp_new, (void *)PyType_GenericNew},
                                            {Py_tp_init, Sampler_init},
                                            {Py_tp_dealloc, Sampler_dealloc},
                                            {Py_tp_methods, (PyMethodDef *)Sampler_methods},
                                            {}};
// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Sampler_spec = {
    .name = "caravangl.Sampler",
    .basicsize = sizeof(PyCaravanSampler),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot *)Sampler_slots,
};