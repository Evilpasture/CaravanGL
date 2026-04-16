#include "caravangl_state.h"
#include "pycaravangl.h"

PyCaravanGL_Status Pipeline_init(PyCaravanPipeline *self, PyObject *args, PyObject *kwds) {
    PyObject *module = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = (CaravanState *)PyModule_GetState(module);
    // Variables to parse into
    PyObject *py_program = nullptr;
    PyObject *py_vao = nullptr;
    uint32_t topology = GL_TRIANGLES;
    uint32_t index_type = 0; // 0 means draw arrays (no index buffer)

    // Render State defaults
    int depth_test = 0;
    int depth_write = 1;
    uint32_t depth_func = GL_LESS;
    int blend_enabled = 0;

    void *targets[PipelineInit_COUNT] = {
        [IDX_PL_PROGRAM] = (void *)&py_program, [IDX_PL_VAO] = (void *)&py_vao,
        [IDX_PL_TOPO] = (void *)(&topology),    [IDX_PL_IDX_TYP] = (void *)(&index_type),
        [IDX_PL_DEPTH] = (void *)(&depth_test), [IDX_PL_DWRITE] = (void *)(&depth_write),
        [IDX_PL_DFUNC] = (void *)(&depth_func), [IDX_PL_BLEND] = (void *)&blend_enabled};

    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.PipelineInitParser, targets)) {
        return -1;
    }

    // 1. Assign GPU Objects
    // VALIDATION: Ensure we actually got Program and VertexArray objects
    if (Py_TYPE(py_program) != state->ProgramType || Py_TYPE(py_vao) != state->VertexArrayType) {
        PyErr_SetString(PyExc_TypeError, "Pipeline requires Program and VertexArray instances.");
        return -1;
    }

    WithCaravanGL(module, OpenGL) {
        self->program_ref = Py_NewRef(py_program);
        self->vao_ref = Py_NewRef(py_vao);

        // EXTRACT raw IDs internally
        self->program = ((PyCaravanProgram *)py_program)->id;
        self->vao = ((PyCaravanVertexArray *)py_vao)->id;
        self->topology = topology;
        self->index_type = index_type;

        // 2. Pack the Render State
        self->render_state.depth_test_enabled = (bool)depth_test;
        self->render_state.depth_write_mask = (bool)depth_write;
        self->render_state.depth_func = depth_func;
        self->render_state.blend_enabled = (bool)blend_enabled;

        // 3. Initialize default draw params
        self->params = (CaravanDrawParams){
            .vertex_count = 0, .instance_count = 1, .first_vertex = 0, .base_instance = 0};
    }

    return 0;
}

PyCaravanGL_API Pipeline_upload_uniforms(PyCaravanPipeline *self, PyObject *const *args,
                                         Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    PyObject *py_batch = nullptr;
    void *targets[PipelineUniforms_COUNT] = {[IDX_PL_U_BATCH] = (void *)&py_batch};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.PipelineUniformsParser, targets)) {
        return nullptr;
    }

    if (Py_TYPE(py_batch) != state->UniformBatchType) {
        PyErr_SetString(PyExc_TypeError, "Expected UniformBatch object.");
        return nullptr;
    }

    PyCaravanUniformBatch *batch = (PyCaravanUniformBatch *)py_batch;

    WithCaravanGL(mod, OpenGL) {
        cv_bind_program(state, self->program);
        cv_upload_uniform_batch(
            state, (CaravanUniformSource){.header = batch->header, .payload = batch->payload});
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Pipeline_draw(PyCaravanPipeline *self, [[maybe_unused]] PyObject *args) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));

    WithCaravanGL(mod, OpenGL) {
        // 0. Predictable Early-Out: If vertex or instance count is 0, do nothing.
        if (self->params.vertex_count == 0 || self->params.instance_count == 0)
            [[clang::unlikely]] {
            Py_RETURN_NONE;
        }

        // 2. Bind core objects (These compile to near-zero cycles if already bound)
        cv_bind_program(state, self->program);
        cv_bind_vao(state, self->vao);

        // 3. Apply render states
        cv_set_depth_state(state, self->render_state.depth_test_enabled,
                           self->render_state.depth_func, self->render_state.depth_write_mask);

        // ... Apply blend state, cull state, etc. ...

        // 4. Dispatch the Draw Call
        if (self->index_type != 0) {
            // Indexed Drawing (glDrawElements)

            // PREDICTABILITY FIX: Convert 'first_vertex' (element index) into a byte offset
            uintptr_t byte_offset = 0;
            switch (self->index_type) {
            case GL_UNSIGNED_INT:
                byte_offset = self->params.first_vertex * sizeof(GLuint);
                break;
            case GL_UNSIGNED_SHORT:
                byte_offset = self->params.first_vertex * sizeof(GLushort);
                break;
            case GL_UNSIGNED_BYTE:
                byte_offset = self->params.first_vertex * sizeof(GLubyte);
                break;
            default:
                byte_offset = 0;
                break;
            }
            void *index_ptr = IntToPtr(byte_offset);

            if (self->params.instance_count > 1) {
                OpenGL->DrawElementsInstanced(self->topology, (GLsizei)self->params.vertex_count,
                                              self->index_type, index_ptr,
                                              (GLsizei)self->params.instance_count);
            } else {
                OpenGL->DrawElements(self->topology, (GLsizei)self->params.vertex_count,
                                     self->index_type, index_ptr);
            }
        } else {
            // Array Drawing (glDrawArrays)

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

/**
 * Pipeline Deallocator
 */
PyCaravanGL_Status Pipeline_traverse(PyCaravanPipeline *self, visitproc visit, void *arg) {
    PyObject **members[] = {&self->program_ref, &self->vao_ref};
    TraverseContext context = {.visit = visit, .arg = arg};

    return caravan_dispatch_members(members, 2, op_visit_member, &context);
}

PyCaravanGL_Status Pipeline_clear(PyCaravanPipeline *self) {
    PyObject **members[] = {&self->program_ref, &self->vao_ref};

    return caravan_dispatch_members(members, 2, op_clear_member, nullptr);
}

// 3. Updated Dealloc: Must notify GC before freeing memory
PyCaravanGL_Slot Pipeline_dealloc(PyCaravanPipeline *self) {
    PyTypeObject *type = Py_TYPE(self);

    // Tell GC we are no longer tracking this instance
    PyObject_GC_UnTrack(self);

    // Call clear to decref members
    [[maybe_unused]] auto cleared = Pipeline_clear(self);

    type->tp_free((PyObject *)self);
    Py_DECREF(type);
}
/**
 * Getter for .params: Exposes the memoryview for zero-copy mutation
 */
PyCaravanGL_API Pipeline_get_params(PyCaravanPipeline *self, [[maybe_unused]] void *closure) {
    Py_buffer view;

    // There are 4 GLuints in CaravanDrawParams
    Py_ssize_t shape = sizeof(CaravanDrawParams) / sizeof(GLuint);
    Py_ssize_t strides = sizeof(GLuint);

    view.buf = &self->params;
    view.obj = (PyObject *)self;
    view.len = sizeof(CaravanDrawParams);
    view.readonly = 0;
    view.itemsize = sizeof(GLuint);
    view.format = "I"; // "I" is unsigned int
    view.ndim = 1;
    view.shape = &shape;
    view.strides = &strides;
    view.suboffsets = nullptr;
    view.internal = nullptr;
    self->params_shape[0] = sizeof(CaravanDrawParams) / sizeof(GLuint);
    self->params_strides[0] = sizeof(GLuint);
    view.shape = self->params_shape;
    view.strides = self->params_strides;

    return PyMemoryView_FromBuffer(&view);
}

static const PyGetSetDef Pipeline_getset[] = {
    {"params", (getter)Pipeline_get_params, nullptr, "Direct access to draw parameters", nullptr},
    {}};

static const PyMethodDef Pipeline_methods[] = {
    {"upload_uniforms", (PyCFunction)(void (*)(void))Pipeline_upload_uniforms,
     METH_FASTCALL | METH_KEYWORDS, nullptr},
    {"draw", (PyCFunction)(void (*)(void))Pipeline_draw, METH_NOARGS, "Execute the draw call."},
    {}};

static const PyType_Slot Pipeline_slots[] = {
    {Py_tp_init, Pipeline_init},
    {Py_tp_dealloc, Pipeline_dealloc},
    {Py_tp_traverse, Pipeline_traverse},
    {Py_tp_clear, Pipeline_clear},
    {Py_tp_methods, (PyMethodDef *)Pipeline_methods},
    {Py_tp_getset, (PyGetSetDef *)Pipeline_getset},
    {Py_tp_doc, "CaravanGL Pipeline: Immutable Draw State"},
    {}};

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Pipeline_spec = {
    .name = "caravangl.Pipeline",
    .basicsize = sizeof(PyCaravanPipeline),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = (PyType_Slot *)Pipeline_slots,
};