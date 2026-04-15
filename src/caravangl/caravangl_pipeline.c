#include "caravangl_state.h"
#include "pycaravangl.h"

PyCaravanGL_Status Pipeline_init(PyCaravanPipeline *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));

    WithCaravanGL(m, gl)
    {
        // Variables to parse into
        PyObject *py_program = nullptr;
        PyObject *py_vao = nullptr;
        uint32_t topology = GL_TRIANGLES;
        uint32_t index_type = 0; // 0 means draw arrays (no index buffer)

        // Render State defaults
        int depth_test = 0, depth_write = 1;
        uint32_t depth_func = GL_LESS;
        int blend_enabled = 0;

        void *targets[PipelineInit_COUNT] = {
            [IDX_PL_PROGRAM] = &py_program, [IDX_PL_VAO] = &py_vao,
            [IDX_PL_TOPO] = &topology,      [IDX_PL_IDX_TYP] = &index_type,
            [IDX_PL_DEPTH] = &depth_test,   [IDX_PL_DWRITE] = &depth_write,
            [IDX_PL_DFUNC] = &depth_func,   [IDX_PL_BLEND] = &blend_enabled};

        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.PipelineInitParser, targets)) {
            return -1;
        }

        // 1. Assign GPU Objects
        // VALIDATION: Ensure we actually got Program and VertexArray objects
        if (Py_TYPE(py_program) != state->ProgramType ||
            Py_TYPE(py_vao) != state->VertexArrayType) {
            PyErr_SetString(PyExc_TypeError,
                            "Pipeline requires Program and VertexArray instances.");
            return -1;
        }

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

        // 4. Expose params as a writable Python memoryview (like HyperGL did)
        // This lets users do `pipeline.params[0] = 6` to change vertex count instantly!
        self->params_buffer.buf = &self->params;
        self->params_buffer.len = sizeof(CaravanDrawParams);
        self->params_buffer.readonly = 0;
        self->params_buffer.itemsize = sizeof(GLuint);
        self->params_buffer.format = "I"; // Unsigned Int
        self->params_buffer.ndim = 1;

        static Py_ssize_t shape = sizeof(CaravanDrawParams) / sizeof(GLuint);
        static Py_ssize_t stride = sizeof(GLuint);
        self->params_buffer.shape = &shape;
        self->params_buffer.strides = &stride;

        // FIX: Tie the buffer lifetime to the Pipeline object
        self->params_buffer.obj = (PyObject *)self;
        Py_INCREF(self);

        self->params_view = PyMemoryView_FromBuffer(&self->params_buffer);
    }
    return 0;
}

PyCaravanGL_API Pipeline_upload_uniforms(PyCaravanPipeline *self, PyObject *const *args,
                                         Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(m);

    PyObject *py_batch = nullptr;
    void *targets[PipelineUniforms_COUNT] = {[IDX_PL_U_BATCH] = &py_batch};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.PipelineUniformsParser, targets))
        return nullptr;

    if (Py_TYPE(py_batch) != state->UniformBatchType) {
        PyErr_SetString(PyExc_TypeError, "Expected UniformBatch object.");
        return nullptr;
    }

    PyCaravanUniformBatch *batch = (PyCaravanUniformBatch *)py_batch;

    WithCaravanGL(m, gl)
    {
        cv_bind_program(state, self->program);
        cv_upload_uniform_batch(state, batch->header, batch->payload);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Pipeline_draw(PyCaravanPipeline *self, PyObject *const *args, Py_ssize_t nargs,
                              PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));

    WithCaravanGL(m, gl)
    {
        // 0. Predictable Early-Out: If vertex or instance count is 0, do nothing.
        if (self->params.vertex_count == 0 || self->params.instance_count == 0)
            [[clang::unlikely]] {
            Py_RETURN_NONE;
        }

        // 1. Lock the context state
        MagMutex_Lock(&state->ctx.state_lock);

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
            }
            void *index_ptr = (void *)byte_offset;

            if (self->params.instance_count > 1) {
                gl.DrawElementsInstanced(self->topology, self->params.vertex_count,
                                         self->index_type, index_ptr, self->params.instance_count);
            } else {
                gl.DrawElements(self->topology, self->params.vertex_count, self->index_type,
                                index_ptr);
            }
        } else {
            // Array Drawing (glDrawArrays)

            if (self->params.instance_count > 1) {
                gl.DrawArraysInstanced(self->topology, self->params.first_vertex,
                                       self->params.vertex_count, self->params.instance_count);
            } else {
                gl.DrawArrays(self->topology, self->params.first_vertex, self->params.vertex_count);
            }
        }

        // 5. Unlock the context
        MagMutex_Unlock(&state->ctx.state_lock);
    }
    Py_RETURN_NONE;
}

/**
 * Pipeline Deallocator
 */
PyCaravanGL_Slot Pipeline_dealloc(PyCaravanPipeline *self) {
    PyTypeObject *tp = Py_TYPE(self);

    // Clean up the memoryview
    Py_XDECREF(self->params_view);

    // Release context reference
    PyObject *m = PyType_GetModule(tp);
    // (If you need to delete GL objects like programs/vaos, do it here with WithCaravanGL)

    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

/**
 * Getter for .params: Exposes the memoryview for zero-copy mutation
 */
PyCaravanGL_API Pipeline_get_params(PyCaravanPipeline *self, void *closure) {
    if (self->params_view) {
        return Py_NewRef(self->params_view);
    }
    Py_RETURN_NONE;
}

PyGetSetDef Pipeline_getset[] = {
    {"params", (getter)Pipeline_get_params, nullptr, "Direct access to draw parameters", nullptr},
    {}};

PyMethodDef Pipeline_methods[] = {
    {"upload_uniforms", (PyCFunction)(void (*)(void))Pipeline_upload_uniforms,
     METH_FASTCALL | METH_KEYWORDS, nullptr},
    {"draw", (PyCFunction)(void (*)(void))Pipeline_draw, METH_NOARGS, "Execute the draw call."},
    {}};

PyType_Slot Pipeline_slots[] = {{Py_tp_init, Pipeline_init},
                                {Py_tp_dealloc, Pipeline_dealloc},
                                {Py_tp_methods, Pipeline_methods},
                                {Py_tp_getset, Pipeline_getset},
                                {Py_tp_doc, "CaravanGL Pipeline: Immutable Draw State"},
                                {}};

PyType_Spec Pipeline_spec = {
    .name = "caravangl.Pipeline",
    .basicsize = sizeof(PyCaravanPipeline),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Pipeline_slots,
};