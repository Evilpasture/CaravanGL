#include "caravangl_context.h"
#include "caravangl_state.h"
#include "pycaravangl.h"

PyCaravanGL_Status Texture_init(PyCaravanTexture *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    uint32_t target = GL_TEXTURE_2D;
    void *targets[TexInit_COUNT] = {[IDX_TEX_TARGET] = &target};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.TexInitParser, targets)) {
        return -1;
    }

    WithActiveGL(OpenGL, cv_state, -1) {
        // Capture context for deferred deletion logic
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);

        OpenGL->GenTextures(1, &self->tex.id);
        self->tex.target = target;

        // Bind to unit 0 to initialize parameters.
        // Note: cv_bind_texture now works on the TLS-provided cv_state.
        cv_bind_texture(cv_state, OpenGL, 0, &self->tex, 0);

        OpenGL->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        OpenGL->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        OpenGL->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        OpenGL->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    return 0;
}

PyCaravanGL_Slot Texture_dealloc(PyCaravanTexture *self) {
    // Prevent cache poisoning: if this texture is in the active context's unit cache, clear it.
    PyCaravanContext *active = cv_active_context;
    if (active) {
#pragma unroll 2
        for (int i = 0; i < CARAVAN_MAX_TEXTURE_UNITS; i++) {
            if (active->ctx.bound.texture_units[i].id == self->tex.id) {
                active->ctx.bound.texture_units[i].id = 0;
            }
        }
    }

    // Use our thread-safe deferred deletion macro
    CV_SAFE_DEALLOC(self, tex.id, texture_count, textures,
                    OpenGL->DeleteTextures(1, &self->tex.id));

    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Texture_traverse(PyCaravanTexture *self, visitproc visit, void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

static int Texture_clear(PyCaravanTexture *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_API Texture_upload(PyCaravanTexture *self, PyObject *const *args, Py_ssize_t nargs,
                               PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    int level = 0;
    int width = 0;
    int height = 0;
    int depth = 0;
    uint32_t internal_format = 0;
    uint32_t format = 0;
    uint32_t type = 0;
    PyObject *py_data = nullptr;

    void *targets[TexUpload_COUNT] = {[IDX_TEX_UPL_LEVEL] = &level,
                                      [IDX_TEX_UPL_W] = &width,
                                      [IDX_TEX_UPL_H] = &height,
                                      [IDX_TEX_UPL_D] = &depth,
                                      [IDX_TEX_UPL_IFMT] = &internal_format,
                                      [IDX_TEX_UPL_FMT] = &format,
                                      [IDX_TEX_UPL_TYPE] = &type,
                                      [IDX_TEX_UPL_DATA] = (void *)&py_data};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.TexUploadParser, targets)) {
        return nullptr;
    }

    const void *ptr = nullptr;
    Py_buffer view;
    if (py_data && py_data != Py_None) {
        if (PyObject_GetBuffer(py_data, &view, PyBUF_SIMPLE) != 0) {
            PyErr_SetString(PyExc_TypeError, "Data must support the buffer protocol.");
            return nullptr;
        }
        ptr = view.buf;
    }

    // Textures are shared resources; any active context can perform the upload.
    WithActiveGL(OpenGL, cv_state, nullptr) {
        self->tex.width = width;
        self->tex.height = height;
        self->tex.depth = depth;
        self->tex.internal_format = internal_format;
        self->tex.format = format;
        self->tex.type = type;

        cv_bind_texture(cv_state, OpenGL, 0, &self->tex, 0);
        OpenGL->PixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (self->tex.target == GL_TEXTURE_2D ||
            self->tex.target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
            OpenGL->TexImage2D(self->tex.target, level, (GLint)internal_format, width, height, 0,
                               format, type, ptr);
        } else if (self->tex.target == GL_TEXTURE_3D || self->tex.target == GL_TEXTURE_2D_ARRAY) {
            OpenGL->TexImage3D(self->tex.target, level, (GLint)internal_format, width, height,
                               depth, 0, format, type, ptr);
        } else if (self->tex.target == GL_TEXTURE_1D) {
            OpenGL->TexImage1D(self->tex.target, level, (GLint)internal_format, width, 0, format,
                               type, ptr);
        }

        if (ptr) {
            PyBuffer_Release(&view);
        }
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Texture_bind(PyCaravanTexture *self, PyObject *const *args, Py_ssize_t nargs,
                             PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    uint32_t unit = 0;
    PyObject *py_sampler = nullptr;
    void *targets[TexBind_COUNT] = {[IDX_TEX_BIND_UNIT] = &unit,
                                    [IDX_TEX_BIND_SAMP] = (void *)&py_sampler};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.TexBindParser, targets)) {
        return nullptr;
    }

    GLuint sampler_id = 0;
    if (py_sampler && py_sampler != Py_None) {
        if (!Py_IS_TYPE(py_sampler, state->SamplerType)) {
            PyErr_Format(PyExc_TypeError, "sampler must be a caravangl.Sampler");
            return nullptr;
        }
        sampler_id = ((PyCaravanSampler *)py_sampler)->id;
    }

    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_bind_texture(cv_state, OpenGL, unit, &self->tex, sampler_id);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Texture_generate_mipmap(PyCaravanTexture *self, [[maybe_unused]] PyObject *unused) {
    WithActiveGL(OpenGL, cv_state, nullptr) {
        // Bind to unit 0 for the operation
        cv_bind_texture(cv_state, OpenGL, 0, &self->tex, 0);
        OpenGL->GenerateMipmap(self->tex.target);
    }
    Py_RETURN_NONE;
}

#define TEX_NOARGS(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Texture_, name)), METH_NOARGS, nullptr}
#define TEX_FASTCALL(name)                                                                         \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(Texture_, name)), METH_FASTCALL | METH_KEYWORDS, nullptr}
#define TEX_O(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Texture_, name)), METH_O, nullptr}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Texture_spec = {
    .name = "caravangl.Texture",
    .basicsize = sizeof(PyCaravanTexture),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, Texture_init},
            {Py_tp_dealloc, Texture_dealloc},
            {Py_tp_traverse, Texture_traverse},
            {Py_tp_clear, Texture_clear},
            {Py_tp_methods,
             (PyMethodDef[]){

                 TEX_FASTCALL(upload), TEX_FASTCALL(bind), TEX_NOARGS(generate_mipmap), {}

             }

            },
            {}

        },
};