#include "caravangl_context.h"
#include "pycaravangl.h"

GLuint compile_shader(const CaravanGLTable *OpenGL, GLenum type, const char *source);

PyCaravanGL_Status ComputePipeline_init(PyCaravanComputePipeline *self, PyObject *args,
                                        PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);
    const char *source = nullptr;

    void *targets[ComputeInit_COUNT] = {[IDX_COMP_SRC] = (void *)&source};
    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.ComputeInitParser, targets)) {
        return -1;
    }

    WithActiveGL(OpenGL, cv_state, -1) {
#ifdef __APPLE__
        PyErr_SetString(PyExc_RuntimeError, "Compute Shaders are not supported on macOS.");
        return -1;
#else
        if (OpenGL->CreateShader == nullptr) {
            return -1;
        }

        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        GLuint shader = compile_shader(OpenGL, GL_COMPUTE_SHADER, source);
        if (!shader) {
            return -1;
        }

        self->id = OpenGL->CreateProgram();
        OpenGL->AttachShader(self->id, shader);
        OpenGL->LinkProgram(self->id);
        OpenGL->DeleteShader(shader);

        GLint success = 0;
        OpenGL->GetProgramiv(self->id, GL_LINK_STATUS, &success);
        if (!success) {
            static constexpr size_t INFO_BUFFER = 512;
            char info[INFO_BUFFER];
            OpenGL->GetProgramInfoLog(self->id, INFO_BUFFER, nullptr, info);
            PyErr_Format(PyExc_RuntimeError, "Compute Link Failed: %s", info);
            return -1;
        }
#endif
    }
    return 0;
}

PyCaravanGL_API ComputePipeline_dispatch(PyCaravanComputePipeline *self, PyObject *args) {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
    if (!PyArg_ParseTuple(args, "III", &x, &y, &z)) {
        return nullptr;
    }

    WithActiveGL(OpenGL, cv_state, nullptr) {
#ifndef __APPLE__
        OpenGL->UseProgram(self->id);
        OpenGL->DispatchCompute(x, y, z);
#endif
    }
    Py_RETURN_NONE;
}

PyCaravanGL_Status ComputePipeline_traverse(PyCaravanComputePipeline *self, visitproc visit,
                                            void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

PyCaravanGL_Status ComputePipeline_clear(PyCaravanComputePipeline *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_Slot ComputePipeline_dealloc(PyCaravanComputePipeline *self) {
    CV_SAFE_DEALLOC(self, id, program_count, programs, OpenGL->DeleteProgram(self->id));
    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

const PyType_Spec ComputePipeline_spec = {
    .name = "caravangl.ComputePipeline",
    .basicsize = sizeof(PyCaravanComputePipeline),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_init, ComputePipeline_init},
            {Py_tp_dealloc, ComputePipeline_dealloc},
            {Py_tp_traverse, ComputePipeline_traverse},
            {Py_tp_clear, ComputePipeline_clear},
            {Py_tp_methods,
             (PyMethodDef[]){

                 {"dispatch", (PyCFunction)ComputePipeline_dispatch, METH_VARARGS,
                  "Launch compute kernel"},
                 {}

             }

            },
            {}

        }

};