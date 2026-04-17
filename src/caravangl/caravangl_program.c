#include "caravangl_context.h"
#include "pycaravangl.h"
#include <ctype.h> // Required for isspace()

// -----------------------------------------------------------------------------
// Program Object
// -----------------------------------------------------------------------------

static GLuint compile_shader(const CaravanGLTable *const OpenGL, GLenum type, const char *source) {
    if (source == nullptr) [[clang::unlikely]]
        {return 0;}

    const char *orig_source = source;

    // 1. SKIP EVERYTHING UNTIL '#'
    // This bypasses BOMs, invisible zero-width spaces, leading comments,
    // and weird Python docstring indentation junk.
    #pragma unroll 2
    while (*source && *source != '#') {
        source++;
    }

    // Fallback: If there's no '#', the user might be using a very old
    // GLSL version or passed garbage. Try the standard aggressive trim.
    if (*source == '\0') {
        source = orig_source;
        #pragma unroll 2
        while (*source && isspace((unsigned char)*source))
            {source++;}
    }

    if (*source == '\0') {
        PyErr_SetString(PyExc_ValueError, "Shader source is empty or invalid.");
        return 0;
    }

    // 2. Trailing Trim
    GLint length = (GLint)strlen(source);
    #pragma unroll 2
    while (length > 0 && isspace((unsigned char)source[length - 1])) {
        length--;
    }

    GLuint shader = OpenGL->CreateShader(type);
    OpenGL->ShaderSource(shader, 1, &source, &length);
    OpenGL->CompileShader(shader);

    GLint success = 0;
    OpenGL->GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        constexpr auto LOG_BUFFER_SIZE = 1024;
        char info_log[LOG_BUFFER_SIZE];
        OpenGL->GetShaderInfoLog(shader, LOG_BUFFER_SIZE, nullptr, info_log);
        const char *type_str = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";

        PyErr_Format(PyExc_RuntimeError,
                     "%s Shader Compilation Failed!\n"
                     "Cleaned Source Start: %.32s\n"
                     "Driver Error: %s",
                     type_str, source, info_log);

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

    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
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
    CV_SAFE_DEALLOC(self, id, program_count, programs, OpenGL->DeleteProgram(self->id));

    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

// Quick helper to fetch a uniform location from Python
PyCaravanGL_API Program_get_uniform_location(PyCaravanProgram *self, PyObject *arg) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "Uniform name must be a string");
        return nullptr;
    }

    const char *name = PyUnicode_AsUTF8(arg);

    // We use WithActiveGL because GetUniformLocation requires an active GL context
    // on the current thread to talk to the driver.
    WithActiveGL(OpenGL, cv_state, nullptr) {
        GLint loc = OpenGL->GetUniformLocation(self->id, name);
        return PyLong_FromLong(loc);
    }

    // The macro returns nullptr if no context is active,
    // so this line is technically unreachable but required for compiler completeness.
    return nullptr;
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Program_spec = {
    .name = "caravangl.Program",
    .basicsize = sizeof(PyCaravanProgram),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots =
        (PyType_Slot[]){{Py_tp_init, Program_init},
                        {Py_tp_dealloc, Program_dealloc},
                        {Py_tp_methods,
                         (PyMethodDef[]){

                             {"get_uniform_location", (PyCFunction)Program_get_uniform_location,
                              METH_O, "Get uniform location by name"},
                             {}}

                        },
                        {}},
};