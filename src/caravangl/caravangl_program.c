#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Program Object
// -----------------------------------------------------------------------------

static GLuint compile_shader(CaravanGLTable *OpenGL, GLenum type, const char *source) {
    GLuint shader = OpenGL->CreateShader(type);
    OpenGL->ShaderSource(shader, 1, &source, nullptr);
    OpenGL->CompileShader(shader);

    GLint success = 0;
    OpenGL->GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        constexpr auto LOG_BUFFER_SIZE = 512;
        char info_log[LOG_BUFFER_SIZE];
        OpenGL->GetShaderInfoLog(shader, LOG_BUFFER_SIZE, nullptr, info_log);
        PyErr_Format(PyExc_RuntimeError, "Shader Compilation Failed:\n%s", info_log);
        OpenGL->DeleteShader(shader);
        return 0;
    }
    return shader;
}

PyCaravanGL_Status Program_init(PyCaravanProgram *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);
    const char *vs_src = nullptr;
    const char *fs_src = nullptr;

    void *targets[ProgInit_COUNT] = {[IDX_PROG_VS] = (void *)&vs_src,
                                     [IDX_PROG_FS] = (void *)&fs_src};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.ProgInitParser, targets)) {
        return -1;
    }

    WithCaravanGL(mod, OpenGL) {
        GLuint vertex_shader = compile_shader(OpenGL, GL_VERTEX_SHADER, vs_src);
        if (!vertex_shader) {
            return -1;
        }
        GLuint fragment_shader = compile_shader(OpenGL, GL_FRAGMENT_SHADER, fs_src);
        if (!fragment_shader) {
            OpenGL->DeleteShader(vertex_shader);
            return -1;
        }

        self->id = OpenGL->CreateProgram();
        OpenGL->AttachShader(self->id, vertex_shader);
        OpenGL->AttachShader(self->id, fragment_shader);
        OpenGL->LinkProgram(self->id);

        // Check Link Status
        GLint success = 0;
        OpenGL->GetProgramiv(self->id, GL_LINK_STATUS, &success);
        if (!success) {
            constexpr auto LOG_BUFFER_SIZE = 512;
            char info_log[LOG_BUFFER_SIZE];
            OpenGL->GetProgramInfoLog(self->id, LOG_BUFFER_SIZE, nullptr, info_log);
            PyErr_Format(PyExc_RuntimeError, "Program Linking Failed:\n%s", info_log);
            return -1;
        }

        // Clean up shaders
        OpenGL->DeleteShader(vertex_shader);
        OpenGL->DeleteShader(fragment_shader);
    }
    return 0;
}

PyCaravanGL_Slot Program_dealloc(PyCaravanProgram *self) {
    PyTypeObject *type = Py_TYPE(self);
    PyObject *mod = PyType_GetModule(type);
    WithCaravanGL(mod, OpenGL) {
        if (self->id) {
            OpenGL->DeleteProgram(self->id);
        }
    }
    type->tp_free((PyObject *)self);
    Py_DECREF(type);
}

// Quick helper to fetch a uniform location from Python
PyCaravanGL_API Program_get_uniform_location(PyCaravanProgram *self, PyObject *arg) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "Uniform name must be a string");
        return nullptr;
    }
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    const char *name = PyUnicode_AsUTF8(arg);
    WithCaravanGL(mod, OpenGL) {
        GLint loc = OpenGL->GetUniformLocation(self->id, name);
        return PyLong_FromLong(loc);
    }
    return nullptr;
}

static const PyMethodDef Program_methods[] = {{"get_uniform_location",
                                               (PyCFunction)Program_get_uniform_location, METH_O,
                                               "Get uniform location by name"},
                                              {}};

static const PyType_Slot Program_slots[] = {{Py_tp_init, Program_init},
                                            {Py_tp_dealloc, Program_dealloc},
                                            {Py_tp_methods, (PyMethodDef *)Program_methods},
                                            {}};

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Program_spec = {
    .name = "caravangl.Program",
    .basicsize = sizeof(PyCaravanProgram),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot *)Program_slots,
};