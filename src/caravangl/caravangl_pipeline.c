#include "caravangl_context.h"
#include "caravangl_state.h"
#include "pycaravangl.h"

PyCaravanGL_Status Pipeline_init(PyCaravanPipeline *self, PyObject *args, PyObject *kwds) {
    PyObject *module = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(module);

    PyObject *py_program = nullptr;
    PyObject *py_vao = nullptr;
    uint32_t topology = GL_TRIANGLES;
    uint32_t index_type = 0;

    // Render State defaults
    int depth_test = 0;
    int depth_write = 1;
    int cull = 0;
    int stencil_test = 0;
    int blend = 0;
    uint32_t depth_func = GL_LESS;
    uint32_t cull_mode = GL_BACK;
    uint32_t front_face = GL_CCW;

    // Stencil Defaults
    uint32_t s_func = GL_ALWAYS;
    uint32_t s_read = ~0U;
    uint32_t s_write = ~0U;
    int s_ref = 0;
    uint32_t s_fail = GL_KEEP;
    uint32_t s_zfail = GL_KEEP;
    uint32_t s_zpass = GL_KEEP;

    // Blend Defaults
    uint32_t b_src_rgb = GL_SRC_ALPHA;
    uint32_t b_dst_rgb = GL_ONE_MINUS_SRC_ALPHA;
    uint32_t b_src_a = GL_ONE;
    uint32_t b_dst_a = GL_ZERO;
    uint32_t b_eq_rgb = GL_FUNC_ADD;
    uint32_t b_eq_a = GL_FUNC_ADD;

    void *targets[PipelineInit_COUNT] = {
        [IDX_PL_PROGRAM] = (void *)&py_program,   [IDX_PL_VAO] = (void *)&py_vao,
        [IDX_PL_TOPO] = (void *)&topology,        [IDX_PL_IDX_TYP] = (void *)&index_type,
        [IDX_PL_DEPTH] = (void *)&depth_test,     [IDX_PL_DWRITE] = (void *)&depth_write,
        [IDX_PL_DFUNC] = (void *)&depth_func,     [IDX_PL_CULL] = (void *)&cull,
        [IDX_PL_CULL_MODE] = (void *)&cull_mode,  [IDX_PL_FRONT_FACE] = (void *)&front_face,
        [IDX_PL_STENCIL] = (void *)&stencil_test, [IDX_PL_SFUNC] = (void *)&s_func,
        [IDX_PL_SREF] = (void *)&s_ref,           [IDX_PL_SRMASK] = (void *)&s_read,
        [IDX_PL_SWMASK] = (void *)&s_write,       [IDX_PL_SFAIL] = (void *)&s_fail,
        [IDX_PL_SZFAIL] = (void *)&s_zfail,       [IDX_PL_SZPASS] = (void *)&s_zpass,
        [IDX_PL_BLEND] = (void *)&blend,          [IDX_PL_B_SRC_RGB] = (void *)&b_src_rgb,
        [IDX_PL_B_DST_RGB] = (void *)&b_dst_rgb,  [IDX_PL_B_SRC_A] = (void *)&b_src_a,
        [IDX_PL_B_DST_A] = (void *)&b_dst_a,      [IDX_PL_B_EQ_RGB] = (void *)&b_eq_rgb,
        [IDX_PL_B_EQ_A] = (void *)&b_eq_a};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.PipelineInitParser, targets)) {
        return -1;
    }

    if (!Py_IS_TYPE(py_program, state->ProgramType) ||
        !Py_IS_TYPE(py_vao, state->VertexArrayType)) {
        PyErr_SetString(PyExc_TypeError, "Pipeline requires Program and VertexArray instances.");
        return -1;
    }

    WithActiveGL(OpenGL, cv_state, -1) {
        // 1. Ownership & References
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        Py_XSETREF(self->program_ref, Py_NewRef(py_program));
        Py_XSETREF(self->vao_ref, Py_NewRef(py_vao));

        if (((PyCaravanVertexArray *)py_vao)->owning_context != _cv_ctx) {
            PyErr_SetString(PyExc_RuntimeError, "VertexArray belongs to a different Context.");
            return -1;
        }

        self->program = ((PyCaravanProgram *)py_program)->id;
        self->vao = ((PyCaravanVertexArray *)py_vao)->id;
        self->topology = topology;
        self->index_type = index_type;

        // 2. Baked Render State
        // We use a compound literal. C99+ rules guarantee that any fields
        // not explicitly mentioned (like _bitfield_pad and _extra_padding)
        // are initialized to ZERO. This makes the cv_render_state_equals
        // comparison deterministic.
        self->render_state =
            (CaravanRenderState){.cull_face_mode = cull_mode,
                                 .front_face = front_face,
                                 .depth_func = depth_func,
                                 .blend_src_rgb = b_src_rgb,
                                 .blend_dst_rgb = b_dst_rgb,
                                 .blend_src_alpha = b_src_a,
                                 .blend_dst_alpha = b_dst_a,
                                 .blend_eq_rgb = b_eq_rgb,
                                 .blend_eq_alpha = b_eq_a,
                                 .stencil_func = s_func,
                                 .stencil_ref = s_ref,
                                 .stencil_read_mask = s_read,
                                 .stencil_write_mask = s_write,
                                 .stencil_fail_op = s_fail,
                                 .stencil_zfail_op = s_zfail,
                                 .stencil_zpass_op = s_zpass,

                                 // Bitfield assignments (0 or 1)
                                 .cull_face_enabled = (uint32_t)(cull != 0),
                                 .depth_test_enabled = (uint32_t)(depth_test != 0),
                                 .depth_write_mask = (uint32_t)(depth_write != 0),
                                 .blend_enabled = (uint32_t)(blend != 0),
                                 .stencil_test_enabled = (uint32_t)(stencil_test != 0)};

        self->params = (CaravanDrawParams){
            .vertex_count = 0, .instance_count = 1, .first_vertex = 0, .base_instance = 0};
    }

    if (!PyObject_GC_IsTracked((PyObject *)self)) {
        PyObject_GC_Track((PyObject *)self);
    }
    return 0;
}

PyCaravanGL_API Pipeline_upload_uniforms(PyCaravanPipeline *self, PyObject *const *args,
                                         Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    auto state = get_caravan_state(mod);

    PyObject *py_batch = nullptr;
    void *targets[PipelineUniforms_COUNT] = {[IDX_PL_U_BATCH] = (void *)&py_batch};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.PipelineUniformsParser, targets)) {
        return nullptr;
    }

    if (!Py_IS_TYPE(py_batch, state->UniformBatchType)) {
        PyErr_SetString(PyExc_TypeError, "Expected UniformBatch object.");
        return nullptr;
    }

    auto batch = (PyCaravanUniformBatch *)py_batch;

    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_set_program(cv_state, self->program);
        cv_resolve(cv_state, OpenGL); // MUST resolve before upload!
        cv_upload_uniform_batch(
            OpenGL, (CaravanUniformSource){.header = batch->header, .payload = batch->payload});
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Pipeline_draw(PyCaravanPipeline *self, [[maybe_unused]] PyObject *args) {
    if (self->params.vertex_count == 0 || self->params.instance_count == 0) {
        Py_RETURN_NONE;
    }

    WithActiveGL(OpenGL, cv_state, nullptr) {
        // Validation: Ensure we aren't trying to draw a context-local VAO in the wrong context
        if (_cv_ctx != self->owning_context) [[clang::unlikely]] {
            PyErr_SetString(PyExc_RuntimeError, "Pipeline draw context mismatch (VAO is local).");
            return nullptr;
        }

        // 1. Stage the state
        cv_set_program(cv_state, self->program);
        cv_set_vao(cv_state, self->vao);

        // 2. Resolve bindings into driver calls
        cv_resolve(cv_state, OpenGL);

        // 3. Sync Render State (Diff checks)
        cv_sync_render_state(cv_state, OpenGL, &self->render_state);

        if (self->index_type != 0) {
            uintptr_t byte_offset = 0;
            if (self->index_type == GL_UNSIGNED_INT) {
                byte_offset = (uintptr_t)self->params.first_vertex * 4;
            } else if (self->index_type == GL_UNSIGNED_SHORT) {
                byte_offset = (uintptr_t)self->params.first_vertex * 2;
            } else {
                byte_offset = self->params.first_vertex;
            }

            if (self->params.instance_count > 1) {
                OpenGL->DrawElementsInstanced(self->topology, (GLsizei)self->params.vertex_count,
                                              self->index_type, IntToPtr(byte_offset),
                                              (GLsizei)self->params.instance_count);
            } else {
                OpenGL->DrawElements(self->topology, (GLsizei)self->params.vertex_count,
                                     self->index_type, IntToPtr(byte_offset));
            }
        } else {
            if (self->params.instance_count > 1) {
                OpenGL->DrawArraysInstanced(self->topology, (GLsizei)self->params.first_vertex,
                                            (GLsizei)self->params.vertex_count,
                                            (GLsizei)self->params.instance_count);
            } else {
                OpenGL->DrawArrays(self->topology, (GLsizei)self->params.first_vertex,
                                   (GLsizei)self->params.vertex_count);
            }
        }
    }
    Py_RETURN_NONE;
}

PyCaravanGL_Status Pipeline_traverse(PyCaravanPipeline *self, visitproc visit, void *arg) {
    Py_VISIT(self->program_ref);
    Py_VISIT(self->vao_ref);
    Py_VISIT(self->owning_context);
    return 0;
}

PyCaravanGL_Status Pipeline_clear(PyCaravanPipeline *self) {
    Py_CLEAR(self->program_ref);
    Py_CLEAR(self->vao_ref);
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_Slot Pipeline_dealloc(PyCaravanPipeline *self) {
    PyTypeObject *type = Py_TYPE(self);
    if (PyObject_GC_IsTracked((PyObject *)self)) {
        PyObject_GC_UnTrack(self);
    }
    (void)Pipeline_clear(self);
    type->tp_free((PyObject *)self);
    Py_DECREF(type);
}

/**
 * Getter for .params: Exposes the memoryview for zero-copy mutation
 */
PyCaravanGL_API Pipeline_get_params(PyCaravanPipeline *self, [[maybe_unused]] void *closure) {
    Py_buffer view;

    view.buf = &self->params;
    view.obj = (PyObject *)self;
    view.len = sizeof(CaravanDrawParams);
    view.readonly = 0;
    view.itemsize = sizeof(GLuint);
    view.format = "I"; // "I" is unsigned int
    view.ndim = 1;
    view.suboffsets = nullptr;
    view.internal = nullptr;
    self->params_shape[0] = sizeof(CaravanDrawParams) / sizeof(GLuint);
    self->params_strides[0] = sizeof(GLuint);
    view.shape = self->params_shape;
    view.strides = self->params_strides;

    return PyMemoryView_FromBuffer(&view);
}

#define PIPE_NOARGS(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Pipeline_, name)), METH_NOARGS, nullptr}
#define PIPE_FASTCALL(name)                                                                        \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(Pipeline_, name)), METH_FASTCALL | METH_KEYWORDS, nullptr}
#define PIPE_O(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(Pipeline_, name)), METH_O, nullptr}

// For Read/Write
#define PIPE_GETSET(name)                                                                          \
    {#name, (getter)CARAVAN_JOIN(Pipeline_get_, name), (setter)CARAVAN_JOIN(Pipeline_set_, name),  \
     nullptr, nullptr}

// For Read-Only
#define PIPE_GET(name) {#name, (getter)CARAVAN_JOIN(Pipeline_get_, name), nullptr, nullptr, nullptr}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Pipeline_spec = {
    .name = "caravangl.Pipeline",
    .basicsize = sizeof(PyCaravanPipeline),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots =
        (PyType_Slot[]){

            {Py_tp_new, PyType_GenericNew},
            {Py_tp_init, Pipeline_init},
            {Py_tp_dealloc, Pipeline_dealloc},
            {Py_tp_traverse, Pipeline_traverse},
            {Py_tp_clear, Pipeline_clear},
            {Py_tp_methods,
             (PyMethodDef[]){

                 PIPE_FASTCALL(upload_uniforms), PIPE_NOARGS(draw), {}

             }

            },
            {Py_tp_getset,

             (PyGetSetDef[]){

                 PIPE_GET(params), {}

             }

            },
            {Py_tp_doc, "CaravanGL Pipeline: Immutable Draw State"},
            {}

        },
};
