#include "caravangl_state.h"
#include "pycaravangl.h"

PyCaravanGL_Status Texture_init(PyCaravanTexture *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        uint32_t target = GL_TEXTURE_2D;
        void *targets[TexInit_COUNT] = {[IDX_TEX_TARGET] = &target};

        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.TexInitParser, targets)) {
            return -1;
        }

        gl.GenTextures(1, &self->tex.id);
        self->tex.target = target;
        
        // Setup sane defaults for 3.3 Core (Requires this or incomplete texture error)
        gl.BindTexture(target, self->tex.id);
        gl.TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl.TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl.TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl.TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl.BindTexture(target, 0);
    }
    return 0;
}

PyCaravanGL_Slot Texture_dealloc(PyCaravanTexture *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);

    WithCaravanGL(m, gl) {
        if (self->tex.id != 0) {
            gl.DeleteTextures(1, &self->tex.id);
            self->tex.id = 0;
        }
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

PyCaravanGL_API Texture_upload(PyCaravanTexture *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        int level = 0, width = 0, height = 0, depth = 0;
        uint32_t internal_format = 0, format = 0, type = 0;
        PyObject *py_data = nullptr;

        void *targets[TexUpload_COUNT] = {
            [IDX_TEX_UPL_LEVEL] = &level, [IDX_TEX_UPL_W] = &width,
            [IDX_TEX_UPL_H] = &height,    [IDX_TEX_UPL_D] = &depth,
            [IDX_TEX_UPL_IFMT] = &internal_format, [IDX_TEX_UPL_FMT] = &format,
            [IDX_TEX_UPL_TYPE] = &type,   [IDX_TEX_UPL_DATA] = &py_data
        };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.TexUploadParser, targets)) return nullptr;

        const void *ptr = nullptr;
        Py_buffer view;
        if (py_data && py_data != Py_None) {
            if (PyObject_GetBuffer(py_data, &view, PyBUF_SIMPLE) == 0) {
                ptr = view.buf;
            } else {
                PyErr_SetString(PyExc_TypeError, "Data must support the buffer protocol.");
                return nullptr;
            }
        }

        // Save metadata
        self->tex.width = width;
        self->tex.height = height;
        self->tex.depth = depth;
        self->tex.internal_format = internal_format;
        self->tex.format = format;
        self->tex.type = type;

        // Force bind texture to unit 0 for upload
        cv_bind_texture(state, 0, self->tex.target, self->tex.id, 0);

        // Tell OpenGL to unpack memory with 1-byte alignment (standard for Python buffers)
        gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (self->tex.target == GL_TEXTURE_2D || self->tex.target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) { // Cube maps use 2D uploads per face
            gl.TexImage2D(self->tex.target, level, internal_format, width, height, 0, format, type, ptr);
        } else if (self->tex.target == GL_TEXTURE_3D || self->tex.target == GL_TEXTURE_2D_ARRAY) {
            gl.TexImage3D(self->tex.target, level, internal_format, width, height, depth, 0, format, type, ptr);
        } else if (self->tex.target == GL_TEXTURE_1D) {
            gl.TexImage1D(self->tex.target, level, internal_format, width, 0, format, type, ptr);
        }

        if (ptr) PyBuffer_Release(&view);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Texture_bind(PyCaravanTexture *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        uint32_t unit = 0;
        void *targets[TexBind_COUNT] = {[IDX_TEX_BIND_UNIT] = &unit};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.TexBindParser, targets)) return nullptr;
        
        // TODO: Assuming Sampler Object is 0 for now until Sampler objects are added
        cv_bind_texture(state, unit, self->tex.target, self->tex.id, 0);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Texture_generate_mipmap(PyCaravanTexture *self, PyObject *unused) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        cv_bind_texture(state, 0, self->tex.target, self->tex.id, 0);
        gl.GenerateMipmap(self->tex.target);
    }
    Py_RETURN_NONE;
}

static PyMethodDef Texture_methods[] = {
    {"upload", (PyCFunction)(void(*)(void))Texture_upload, METH_FASTCALL | METH_KEYWORDS, "Upload data to GPU"},
    {"bind", (PyCFunction)(void(*)(void))Texture_bind, METH_FASTCALL | METH_KEYWORDS, "Bind texture to unit"},
    {"generate_mipmap", (PyCFunction)Texture_generate_mipmap, METH_NOARGS, "Generate mipmaps"},
    {nullptr}
};

PyType_Slot Texture_slots[] = {
    {Py_tp_init, Texture_init},
    {Py_tp_dealloc, Texture_dealloc},
    {Py_tp_methods, Texture_methods},
    {0, nullptr}
};

PyType_Spec Texture_spec = {
    .name = "caravangl.Texture",
    .basicsize = sizeof(PyCaravanTexture),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Texture_slots,
};