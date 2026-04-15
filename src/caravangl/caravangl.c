/**
 * ============================================================================
 * CaravanGL
 * ============================================================================
 * Architecture: C23 + Python 3.14t (Free-Threaded)
 * Features: Isolated State, Fast-Path Parsing, Register-Speed Build, No-GIL.
 * ============================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "caravangl_arg_indices.h"
#include "caravangl_loader.h"
#include "caravangl_pyspec.h"
#include "fast_build.h"
#include "pycaravangl.h"
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
            GLint v[4] = {};
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

    // ADDED: Texture Inspection
    if (type == state->TextureType) {
        PyCaravanTexture *t = (PyCaravanTexture *)arg;
        return FastBuild_Dict("type", "texture", 
                              "id", (long long)t->tex.id, 
                              "target", (long long)t->tex.target,
                              "width", (long long)t->tex.width,
                              "height", (long long)t->tex.height,
                              "internal_format", (long long)t->tex.internal_format);
    }

    // ADDED: UniformBatch Inspection (good for coverage)
    if (type == state->UniformBatchType) {
        PyCaravanUniformBatch *ub = (PyCaravanUniformBatch *)arg;
        return FastBuild_Dict("type", "uniform_batch", 
                              "count", (long long)ub->header->count,
                              "used_bytes", (long long)ub->current_payload_offset,
                              "max_bytes", (long long)ub->max_payload_bytes);
    }

    // If we don't recognize the object, return None natively.
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_clear(PyObject *m, PyObject *const *args, Py_ssize_t nargs,
                              PyObject *kwnames) {
    WithCaravanGL(m, gl)
    {
        uint32_t mask = 0;
        void *targets[Clear_COUNT] = {[IDX_CLR_MASK] = &mask};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ClearParser, targets))
            return nullptr;

        gl.Clear(mask);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_clear_color(PyObject *m, PyObject *const *args, Py_ssize_t nargs,
                                    PyObject *kwnames) {
    WithCaravanGL(m, gl)
    {
        float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
        void *targets[ClearColor_COUNT] = {
            [IDX_CLR_C_R] = &r, [IDX_CLR_C_G] = &g, [IDX_CLR_C_B] = &b, [IDX_CLR_C_A] = &a};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ClearColorParser, targets))
            return nullptr;

        const GLfloat color[] = {r, g, b, a};
        gl.ClearBufferfv(GL_COLOR, 0, color);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_enable_debug(PyObject *m, [[maybe_unused]] PyObject *args) {
#if !defined(__APPLE__)
    WithCaravanGL(m, gl)
    {
        if (!gl.DebugMessageCallback) {
            PyErr_SetString(PyExc_RuntimeError, "Debug Output not supported on this driver.");
            return nullptr;
        }

        // 1. Enable debug mode in the driver
        gl.Enable(GL_DEBUG_OUTPUT);

        // 2. Force synchronous calls. This blocks the CPU until the callback finishes.
        // It makes performance worse, but gives you highly accurate error call-stacks.
        gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        // 3. Register the callback and pass 'state' as the userParam
        gl.DebugMessageCallback(opengl_debug_callback, state);

        // 4. Control what messages you want to hear:

        // Enable everything by default
        gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

        // Example: Disable "Notification" level spam (like buffer memory placement info)
        gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0,
                               nullptr, GL_FALSE);

        // Example: If you wanted to disable specific driver error IDs manually:
        // GLuint skip_ids[] = { 1234, 5678 };
        // gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 2, skip_ids, GL_FALSE);
    }
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_RuntimeError, "OpenGL 4.3 Debug Output is not available on macOS.");
    return nullptr;
#endif
}

static PyMethodDef caravan_methods[] = {
    {"init", (PyCFunction)(void (*)(void))caravan_init, METH_FASTCALL | METH_KEYWORDS,
     "Initialize loader"},
    {"enable_debug", (PyCFunction)caravan_enable_debug, METH_NOARGS, "Enable GL Debug Output"},
    {"context", (PyCFunction)(void (*)(void))caravan_context, METH_NOARGS, "Get capabilities"},
    {"inspect", (PyCFunction)caravan_inspect, METH_O, "Inspect internal C/GL state"},
    {"clear", (PyCFunction)(void (*)(void))caravan_clear, METH_FASTCALL | METH_KEYWORDS,
     "Clear buffers (e.g. COLOR_BUFFER_BIT)"},
    {"clear_color", (PyCFunction)(void (*)(void))caravan_clear_color, METH_FASTCALL | METH_KEYWORDS,
     "Set clear color"},
    {}};

// -----------------------------------------------------------------------------
// Module Lifecycle
// -----------------------------------------------------------------------------

/**
 * Internal helpers for module initialization.
 */

static int init_types(PyObject *m, CaravanState *state) {
    struct {
        const PyType_Spec *spec;
        PyObject **slot;
        const char *name;
    } types[] = {
        {&Buffer_spec, (PyObject **)&state->BufferType, "Buffer"},
        {&Pipeline_spec, (PyObject **)&state->PipelineType, "Pipeline"},
        {&Program_spec, (PyObject **)&state->ProgramType, "Program"},
        {&VertexArray_spec, (PyObject **)&state->VertexArrayType, "VertexArray"},
        {&UniformBatch_spec, (PyObject **)&state->UniformBatchType, "UniformBatch"},
        {&Texture_spec, (PyObject **)&state->TextureType, "Texture"},
    };

    auto mod_name = PyUnicode_FromString("caravangl");
    if (!mod_name) return -1;

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        auto type = PyType_FromModuleAndSpec(m, (PyType_Spec *)types[i].spec, nullptr);
        if (!type) {
            Py_DECREF(mod_name);
            return -1;
        }

        // Set __module__ for better repr() and pickling
        PyObject_SetAttrString(type, "__module__", mod_name);

        // Add to module (PyModule_AddObjectRef is CPython 3.10+ and safer)
        if (PyModule_AddObjectRef(m, types[i].name, type) < 0) {
            Py_DECREF(type);
            Py_DECREF(mod_name);
            return -1;
        }

        // Store in state for fast internal C-access
        *types[i].slot = type;
        Py_DECREF(type);
    }

    Py_DECREF(mod_name);
    return 0;
}

static int init_constants(PyObject *m) {
    static const struct {
        const char *name;
        long value;
    } consts[] = {{"FLOAT", GL_FLOAT},
                  {"TRIANGLES", GL_TRIANGLES},
                  {"UF_1I", UF_1I},
                  {"UF_1F", UF_1F},
                  {"UF_2F", UF_2F},
                  {"UF_3F", UF_3F},
                  {"UF_4F", UF_4F},
                  {"UF_MAT4", UF_MAT4},

                  // Build Metadata
                  {"FREE_THREADED",
#if defined(Py_GIL_DISABLED) && Py_GIL_DISABLED
                   1
#else
                   0
#endif
                  },
                  {"DEBUG_BUILD",
#if defined(CARAVANGL_DEBUG)
                   1
#else
                   0
#endif
                  }};

    for (size_t i = 0; i < sizeof(consts) / sizeof(consts[0]); i++) {
        if (PyModule_AddIntConstant(m, consts[i].name, consts[i].value) < 0) {
            return -1;
        }
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Module Lifecycle Implementation
// -----------------------------------------------------------------------------

PyCaravanGL_Status caravan_exec(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state == nullptr) return -1;

    // Zero out state and prepare fast-path parsers
    memset(state, 0, sizeof(CaravanState));
    caravan_init_parsers(&state->parsers);

    // Register Classes
    if (init_types(m, state) < 0) return -1;

    // Register Constants
    if (init_constants(m) < 0) return -1;

    return 0;
}

PyCaravanGL_Status caravan_traverse(PyObject *m, visitproc visit, void *arg) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        Py_VISIT(state->BufferType);
        Py_VISIT(state->PipelineType);
        Py_VISIT(state->ProgramType);
        Py_VISIT(state->VertexArrayType);
        Py_VISIT(state->UniformBatchType);
        Py_VISIT(state->TextureType);
    }
    return 0;
}

PyCaravanGL_Status py_caravan_clear(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        caravan_free_parsers(&state->parsers);
        Py_CLEAR(state->BufferType);
        Py_CLEAR(state->PipelineType);
        Py_CLEAR(state->ProgramType);
        Py_CLEAR(state->VertexArrayType);
        Py_CLEAR(state->UniformBatchType);
        Py_CLEAR(state->TextureType);
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
    .m_clear = py_caravan_clear,
    .m_free = nullptr,
};

PyMODINIT_FUNC PyInit_caravangl(void) { return PyModuleDef_Init(&caravan_module); }