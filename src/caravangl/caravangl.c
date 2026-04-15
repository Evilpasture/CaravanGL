/**
 * ============================================================================
 * CaravanGL - Final Implementation
 * ============================================================================
 * Architecture: C23 + Python 3.14t (Free-Threaded)
 * Features: Isolated State, Fast-Path Parsing, Register-Speed Build, No-GIL.
 * ============================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "caravangl_loader.h"
#include "caravangl_arg_indices.h"
#include "fast_build.h"
#include "pycaravangl.h"
#include "caravangl_pipeline.h"
#include "caravangl_buffer.h"
#include "caravangl_uniformbatch.h"
#include "caravangl_program.h"
#include <string.h>


// -----------------------------------------------------------------------------
// Internal Helpers
// -----------------------------------------------------------------------------

/**
 * Queries GPU hardware limits and populates the state context.
 * Called once during caravan.init().
 */
static void query_capabilities(PyObject *m) {
    WithCaravanGL(m, gl)
    {
        if (gl.GetIntegerv != nullptr) {
            // Textures
            gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &state->ctx.caps.max_texture_size);
            gl.GetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &state->ctx.caps.max_3d_texture_size);
            gl.GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &state->ctx.caps.max_array_texture_layers);
            gl.GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &state->ctx.caps.max_texture_units);

            // FBOs & Buffers
            gl.GetIntegerv(GL_MAX_SAMPLES, &state->ctx.caps.max_samples);
            gl.GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &state->ctx.caps.max_color_attachments);
            gl.GetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &state->ctx.caps.max_uniform_block_size);
            gl.GetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &state->ctx.caps.max_ubo_bindings);

            // Viewport
            GLint v[4] = {0};
            gl.GetIntegerv(GL_VIEWPORT, v);
            state->ctx.viewport = (CaravanRect){.x = v[0], .y = v[1], .w = v[2], .h = v[3]};
        }

#ifndef __APPLE__
        state->ctx.caps.support_compute = (gl.DispatchCompute != nullptr);
        state->ctx.caps.support_bindless = (gl.GetTextureHandleARB != nullptr);

        if (state->ctx.caps.support_compute && gl.GetIntegerv != nullptr) {
            gl.GetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                           &state->ctx.caps.max_compute_work_group_invocations);
            gl.GetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,
                           &state->ctx.caps.max_shader_storage_block_size);
        }
#else
        // Hardcode to false for Mac (OpenGL 4.1 limit)
        state->ctx.caps.support_compute = false;
        state->ctx.caps.support_bindless = false;
        state->ctx.caps.max_compute_work_group_invocations = 0;
        state->ctx.caps.max_shader_storage_block_size = 0;
#endif
    }
}

// -----------------------------------------------------------------------------
// Module-Level API
// -----------------------------------------------------------------------------

/**
 * caravan.init(loader)
 * Loads the function table and discovers GPU capabilities.
 */
PyCaravanGL_API caravan_init(PyObject *m, PyObject *const *args, Py_ssize_t nargsf,
                              PyObject *kwnames) {
    WithCaravanGL(m, gl)
    {
        PyObject *loader = nullptr;
        void *targets[Init_COUNT] = {[IDX_INIT_LOADER] = &loader};

        if (!FastParse_Unified(args, PyVectorcall_NARGS(nargsf), kwnames,
                               &state->parsers.InitParser, targets)) {
            return nullptr;
        }

        if (load_gl(state, loader) < 0) return nullptr;
        query_capabilities(m);
    }
    Py_RETURN_NONE;
}

/**
 * caravangl.context() -> dict
 * Returns a snapshot of capabilities and driver info using FastBuild.
 */
PyCaravanGL_API caravan_context(PyObject *m, [[maybe_unused]] PyObject *args) {
    WithCaravanGL(m, gl)
    {
        if (gl.GetString == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "OpenGL not loaded. Call init() first.");
            return nullptr;
        }

        return FastBuild_Dict("caps",
                              FastBuild_Dict("max_texture_size", state->ctx.caps.max_texture_size,
                                             "max_samples", state->ctx.caps.max_samples,
                                             "support_compute", state->ctx.caps.support_compute,
                                             "support_bindless", state->ctx.caps.support_bindless),
                              "info",
                              FastBuild_Dict("vendor", (const char *)gl.GetString(GL_VENDOR),
                                             "renderer", (const char *)gl.GetString(GL_RENDERER),
                                             "version", (const char *)gl.GetString(GL_VERSION)),
                              "viewport",
                              FastBuild_Tuple(state->ctx.viewport.x, state->ctx.viewport.y,
                                              state->ctx.viewport.w, state->ctx.viewport.h));
    }
    return nullptr;
}

/**
 * caravangl.inspect(obj) -> dict | None
 * Returns a dictionary containing the internal OpenGL IDs and state of a Caravan object.
 */
PyCaravanGL_API caravan_inspect(PyObject *m, PyObject *arg) {
    CaravanState *state = get_caravan_state(m);
    if (!state) return nullptr;

    PyTypeObject *type = Py_TYPE(arg);

    // -------------------------------------------------------------------------
    // BUFFER
    // -------------------------------------------------------------------------
    if (type == state->BufferType) {
        PyCaravanBuffer *b = (PyCaravanBuffer *)arg;
        return FastBuild_Dict("type", (const char *)"buffer", "id", (long long)b->buf.id, "target",
                              (long long)b->buf.target, "size", (long long)b->buf.size, "usage",
                              (long long)b->buf.usage);
    }

    // -------------------------------------------------------------------------
    // PIPELINE
    // -------------------------------------------------------------------------
    if (type == state->PipelineType) { // Assumes PipelineType is stored in CaravanState
        PyCaravanPipeline *p = (PyCaravanPipeline *)arg;

        // Return a structured dictionary reflecting the exact C struct
        return FastBuild_Dict("type", (const char *)"pipeline", "program", (long long)p->program,
                              "vao", (long long)p->vao, "topology", (long long)p->topology,
                              "index_type", (long long)p->index_type,

                              "render_state",
                              FastBuild_Dict("depth_test", p->render_state.depth_test_enabled,
                                             "depth_write", p->render_state.depth_write_mask,
                                             "depth_func", (long long)p->render_state.depth_func,
                                             "blend", p->render_state.blend_enabled),

                              "draw_params",
                              FastBuild_Dict("vertex_count", (long long)p->params.vertex_count,
                                             "instance_count", (long long)p->params.instance_count,
                                             "first_vertex", (long long)p->params.first_vertex,
                                             "base_instance", (long long)p->params.base_instance));
    }

    if (type == state->ProgramType) {
        PyCaravanProgram *p = (PyCaravanProgram *)arg;
        return FastBuild_Dict("type", "program", "id", (long long)p->id);
    }

    // ADDED: VertexArray Inspection
    if (type == state->VertexArrayType) {
        PyCaravanVertexArray *v = (PyCaravanVertexArray *)arg;
        return FastBuild_Dict("type", "vertex_array", "id", (long long)v->id);
    }

    // TODO: Add Texture / Framebuffer / Compute inspections here as you build them.

    // If we don't recognize the object, return None natively.
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_test_render(PyObject *m, [[maybe_unused]] PyObject *const *args,
                                     [[maybe_unused]] Py_ssize_t nargs,
                                     [[maybe_unused]] PyObject *kwnames) {
    WithCaravanGL(m, gl)
    {
        if (gl.ClearBufferfv == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "GL not initialized");
            return nullptr;
        }
        const GLfloat clear_color[] = {0.1f, 0.2f, 0.3f, 1.0f};
        gl.ClearBufferfv(GL_COLOR, 0, clear_color);
        Py_RETURN_NONE;
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
// VertexArray Object
// -----------------------------------------------------------------------------

PyCaravanGL_Status VertexArray_init(PyCaravanVertexArray *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl)
    {
        gl.GenVertexArrays(1, &self->id);
    }
    return 0;
}

PyCaravanGL_Slot VertexArray_dealloc(PyCaravanVertexArray *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    WithCaravanGL(m, gl)
    {
        if (self->id) gl.DeleteVertexArrays(1, &self->id);
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

PyCaravanGL_API VertexArray_bind_attribute(PyCaravanVertexArray *self, PyObject *const *args,
                                            Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl)
    {
        uint32_t location = 0, type = GL_FLOAT;
        int size = 0, normalized = 0, stride = 0, offset = 0;
        PyObject *py_buffer = nullptr;

        void *targets[VaoAttr_COUNT] = {
            [IDX_VAO_ATTR_LOC] = &location,    [IDX_VAO_ATTR_BUF] = &py_buffer,
            [IDX_VAO_ATTR_SIZE] = &size,       [IDX_VAO_ATTR_TYPE] = &type,
            [IDX_VAO_ATTR_NORM] = &normalized, [IDX_VAO_ATTR_STRIDE] = &stride,
            [IDX_VAO_ATTR_OFFSET] = &offset};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.VaoAttrParser, targets))
            return nullptr;

        // Verify it's actually our Buffer object
        if (Py_TYPE(py_buffer) != state->BufferType) {
            PyErr_SetString(PyExc_TypeError, "buffer must be a caravangl.Buffer");
            return nullptr;
        }
        PyCaravanBuffer *buf = (PyCaravanBuffer *)py_buffer;

        // Bind VAO and VBO, then map the attribute
        gl.BindVertexArray(self->id);
        gl.BindBuffer(GL_ARRAY_BUFFER, buf->buf.id);

        gl.EnableVertexAttribArray(location);
        gl.VertexAttribPointer(location, size, type, normalized ? GL_TRUE : GL_FALSE, stride,
                               (void *)(uintptr_t)offset);

        // Unbind VAO to prevent accidental corruption
        gl.BindVertexArray(0);
    }
    Py_RETURN_NONE;
}

static PyMethodDef VertexArray_methods[] = {
    {"bind_attribute", (PyCFunction)(void (*)(void))VertexArray_bind_attribute,
     METH_FASTCALL | METH_KEYWORDS, "Map a VBO to a shader attribute"},
    {nullptr}};

static PyType_Slot VertexArray_slots[] = {{Py_tp_init, VertexArray_init},
                                          {Py_tp_dealloc, VertexArray_dealloc},
                                          {Py_tp_methods, VertexArray_methods},
                                          {0, nullptr}};

static PyType_Spec VertexArray_spec = {
    .name = "caravangl.VertexArray",
    .basicsize = sizeof(PyCaravanVertexArray),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = VertexArray_slots,
};

static PyMethodDef caravan_methods[] = {
    {"init", (PyCFunction)(void (*)(void))caravan_init, METH_FASTCALL | METH_KEYWORDS,
     "Initialize loader"},
    {"context", (PyCFunction)(void (*)(void))caravan_context, METH_NOARGS,
     "Get capabilities"},
    {"inspect", (PyCFunction)caravan_inspect, METH_O, "Inspect internal C/GL state"},
    {"test_render", (PyCFunction)(void (*)(void))caravan_test_render, METH_FASTCALL | METH_KEYWORDS,
     "Test render"},
    {nullptr, nullptr, 0, nullptr}};

// -----------------------------------------------------------------------------
// Module Lifecycle
// -----------------------------------------------------------------------------

PyCaravanGL_Status caravan_exec(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state == nullptr) return -1;
    memset(state, 0, sizeof(CaravanState));
    caravan_init_parsers(&state->parsers);

    // Buffer
    state->BufferType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Buffer_spec, nullptr);
    PyModule_AddObjectRef(m, "Buffer", (PyObject *)state->BufferType);

    // Pipeline
    state->PipelineType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Pipeline_spec, nullptr);
    PyModule_AddObjectRef(m, "Pipeline", (PyObject *)state->PipelineType);

    // Program - FIX: Assign to state->ProgramType
    state->ProgramType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Program_spec, nullptr);
    PyModule_AddObjectRef(m, "Program", (PyObject *)state->ProgramType);

    // VertexArray - FIX: Assign to state->VertexArrayType
    state->VertexArrayType =
        (PyTypeObject *)PyType_FromModuleAndSpec(m, &VertexArray_spec, nullptr);
    PyModule_AddObjectRef(m, "VertexArray", (PyObject *)state->VertexArrayType);

    state->UniformBatchType =
        (PyTypeObject *)PyType_FromModuleAndSpec(m, &UniformBatch_spec, nullptr);
    PyModule_AddObjectRef(m, "UniformBatch", (PyObject *)state->UniformBatchType);

    PyModule_AddIntConstant(m, "FLOAT", GL_FLOAT);
    PyModule_AddIntConstant(m, "TRIANGLES", GL_TRIANGLES);
    PyModule_AddIntConstant(m, "UF_1I", UF_1I);
    PyModule_AddIntConstant(m, "UF_1F", UF_1F);
    PyModule_AddIntConstant(m, "UF_2F", UF_2F);
    PyModule_AddIntConstant(m, "UF_3F", UF_3F);
    PyModule_AddIntConstant(m, "UF_4F", UF_4F);
    PyModule_AddIntConstant(m, "UF_MAT4", UF_MAT4);
    return 0;
}

PyCaravanGL_Status caravan_traverse(PyObject *m, visitproc visit, void *arg) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        Py_VISIT(state->BufferType);
        Py_VISIT(state->PipelineType);
        Py_VISIT(state->ProgramType);
        Py_VISIT(state->VertexArrayType);
    }
    return 0;
}

PyCaravanGL_Status caravan_clear(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        caravan_free_parsers(&state->parsers);
        Py_CLEAR(state->BufferType);
        Py_CLEAR(state->PipelineType);
        Py_CLEAR(state->ProgramType);
        Py_CLEAR(state->VertexArrayType);
    }
    return 0;
}

static PyModuleDef_Slot caravan_slots[] = {
    {Py_mod_exec, caravan_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {}};

static struct PyModuleDef caravan_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "caravangl",
    .m_doc = "CaravanGL: Modern Isolated OpenGL Context",
    .m_size = sizeof(CaravanState),
    .m_methods = caravan_methods,
    .m_slots = caravan_slots,
    .m_traverse = caravan_traverse,
    .m_clear = caravan_clear,
    .m_free = nullptr,
};

PyMODINIT_FUNC PyInit_caravangl(void) { return PyModuleDef_Init(&caravan_module); }