#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Program Object
// -----------------------------------------------------------------------------

static GLuint compile_shader(CaravanGLTable *gl, GLenum type, const char *source) {
    GLuint shader = gl->CreateShader(type);
    gl->ShaderSource(shader, 1, &source, nullptr);
    gl->CompileShader(shader);

    GLint success;
    gl->GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        gl->GetShaderInfoLog(shader, 512, nullptr, info_log);
        PyErr_Format(PyExc_RuntimeError, "Shader Compilation Failed:\n%s", info_log);
        gl->DeleteShader(shader);
        return 0;
    }
    return shader;
}

PyCaravanGL_Status Program_init(PyCaravanProgram *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl)
    {
        const char *vs_src = nullptr;
        const char *fs_src = nullptr;

        void *targets[ProgInit_COUNT] = {[IDX_PROG_VS] = &vs_src, [IDX_PROG_FS] = &fs_src};

        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.ProgInitParser, targets))
            return -1;

        GLuint vs = compile_shader(&gl, GL_VERTEX_SHADER, vs_src);
        if (!vs) return -1;
        GLuint fs = compile_shader(&gl, GL_FRAGMENT_SHADER, fs_src);
        if (!fs) {
            gl.DeleteShader(vs);
            return -1;
        }

        self->id = gl.CreateProgram();
        gl.AttachShader(self->id, vs);
        gl.AttachShader(self->id, fs);
        gl.LinkProgram(self->id);

        // Check Link Status
        GLint success;
        gl.GetProgramiv(self->id, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            gl.GetProgramInfoLog(self->id, 512, nullptr, info_log);
            PyErr_Format(PyExc_RuntimeError, "Program Linking Failed:\n%s", info_log);
            return -1;
        }

        // Clean up shaders
        gl.DeleteShader(vs);
        gl.DeleteShader(fs);
    }
    return 0;
}

PyCaravanGL_Slot Program_dealloc(PyCaravanProgram *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    WithCaravanGL(m, gl)
    {
        if (self->id) gl.DeleteProgram(self->id);
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

// Quick helper to fetch a uniform location from Python
PyCaravanGL_API Program_get_uniform_location(PyCaravanProgram *self, PyObject *arg) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "Uniform name must be a string");
        return nullptr;
    }
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl)
    {
        const char *name = PyUnicode_AsUTF8(arg);
        GLint loc = gl.GetUniformLocation(self->id, name);
        return PyLong_FromLong(loc);
    }
    return nullptr;
}

static PyMethodDef Program_methods[] = {{"get_uniform_location",
                                         (PyCFunction)Program_get_uniform_location, METH_O,
                                         "Get uniform location by name"},
                                        {}};

static PyType_Slot Program_slots[] = {{Py_tp_init, Program_init},
                                      {Py_tp_dealloc, Program_dealloc},
                                      {Py_tp_methods, Program_methods},
                                      {}};

PyType_Spec Program_spec = {
    .name = "caravangl.Program",
    .basicsize = sizeof(PyCaravanProgram),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Program_slots,
};