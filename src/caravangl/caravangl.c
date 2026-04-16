/**
 * ============================================================================
 * CaravanGL
 * ============================================================================
 * Architecture: C23 + Python 3.14t (Free-Threaded)
 * Features: Isolated State, Fast-Path Parsing, Register-Speed Build, No-GIL.
 * ============================================================================
 */

#define PY_SSIZE_T_CLEAN
#include "caravangl_arg_indices.h"
#include "caravangl_loader.h"
#include "caravangl_pyspec.h"
#include "fast_build.h"
#include "pycaravangl.h"
#include <Python.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Internal Helpers
// -----------------------------------------------------------------------------

/**
 * Queries GPU hardware limits and populates the state context.
 * Called once during caravan.init().
 */
static void query_capabilities(PyObject *mod) {
    WithCaravanGL(mod, gl) {
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
            GLint viewport[4] = {};
            gl.GetIntegerv(GL_VIEWPORT, viewport);
            state->ctx.viewport = (CaravanRect){
                .x = viewport[0], .y = viewport[1], .w = viewport[2], .h = viewport[3]};
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
PyCaravanGL_API caravan_init(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf,
                             PyObject *kwnames) {
    WithCaravanGL(mod, gl) {
        PyObject *loader = nullptr;
        void *targets[Init_COUNT] = {[IDX_INIT_LOADER] = (void *)&loader};

        if (!FastParse_Unified(args, PyVectorcall_NARGS(nargsf), kwnames,
                               &state->parsers.InitParser, targets)) {
            return nullptr;
        }

        if (load_gl(state, loader) < 0) {
            return nullptr;
        }
        query_capabilities(mod);
    }
    Py_RETURN_NONE;
}

/**
 * caravangl.context() -> dict
 * Returns a snapshot of capabilities and driver info using FastBuild.
 */
static inline PyObject *build_context_dict(CaravanState *state, CaravanGLTable OpenGL) {
    // 0 nesting here!
    PyObject *caps = FastBuild_Dict("max_texture_size", state->ctx.caps.max_texture_size,
                                    "max_samples", state->ctx.caps.max_samples, "support_compute",
                                    state->ctx.caps.support_compute, "support_bindless",
                                    state->ctx.caps.support_bindless);

    PyObject *info = FastBuild_Dict("vendor", (const char *)OpenGL.GetString(GL_VENDOR), "renderer",
                                    (const char *)OpenGL.GetString(GL_RENDERER), "version",
                                    (const char *)OpenGL.GetString(GL_VERSION));

    return FastBuild_Dict("caps", caps, "info", info, "viewport",
                          FastBuild_Tuple(state->ctx.viewport.x, state->ctx.viewport.y,
                                          state->ctx.viewport.w, state->ctx.viewport.h));
}

PyCaravanGL_API caravan_context(PyObject *mod, [[maybe_unused]] PyObject *args) {
    WithCaravanGL(mod, gl) {
        if (gl.GetString == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "OpenGL not loaded. Call init() first.");
            return nullptr;
        }

        // Complexity: 1 (the call) + depth penalty of the macro
        return build_context_dict(state, gl);
    }
    return nullptr;
}

/**
 * caravangl.inspect(obj) -> dict | None
 * Returns a dictionary containing the internal OpenGL IDs and state of a
 * Caravan object.
 */
// --- Internal Inspection Helpers ---

static inline PyObject *inspect_buffer(PyCaravanBuffer *buf) {
    return FastBuild_Dict("type", "buffer", "id", (Py_ssize_t)buf->buf.id, "target",
                          (Py_ssize_t)buf->buf.target, "size", (Py_ssize_t)buf->buf.size, "usage",
                          (Py_ssize_t)buf->buf.usage);
}

static inline PyObject *inspect_pipeline(PyCaravanPipeline *pip) {
    auto state_dict = FastBuild_Dict("depth_test", pip->render_state.depth_test_enabled,
                                     "depth_write", pip->render_state.depth_write_mask,
                                     "depth_func", (Py_ssize_t)pip->render_state.depth_func,
                                     "blend", pip->render_state.blend_enabled);

    auto params_dict = FastBuild_Dict("vertex_count", (Py_ssize_t)pip->params.vertex_count,
                                      "instance_count", (Py_ssize_t)pip->params.instance_count,
                                      "first_vertex", (Py_ssize_t)pip->params.first_vertex,
                                      "base_instance", (Py_ssize_t)pip->params.base_instance);

    return FastBuild_Dict("type", "pipeline", "program", (Py_ssize_t)pip->program, "vao",
                          (Py_ssize_t)pip->vao, "topology", (Py_ssize_t)pip->topology, "index_type",
                          (Py_ssize_t)pip->index_type, "render_state", state_dict, "draw_params",
                          params_dict);
}

static inline PyObject *inspect_texture(PyCaravanTexture *tex) {
    return FastBuild_Dict("type", "texture", "id", (Py_ssize_t)tex->tex.id, "target",
                          (Py_ssize_t)tex->tex.target, "width", (Py_ssize_t)tex->tex.width,
                          "height", (Py_ssize_t)tex->tex.height, "internal_format",
                          (Py_ssize_t)tex->tex.internal_format);
}

static inline PyObject *inspect_uniform_batch(PyCaravanUniformBatch *bat) {
    return FastBuild_Dict("type", "uniform_batch", "count", (Py_ssize_t)bat->header->count,
                          "used_bytes", (Py_ssize_t)bat->current_payload_offset, "max_bytes",
                          (Py_ssize_t)bat->max_payload_bytes);
}

// --- Main Dispatcher ---

PyCaravanGL_API caravan_inspect(PyObject *mod, PyObject *arg) {
    auto state = get_caravan_state(mod);
    if (!state) {
        [[clang::unlikely]] return nullptr;
    }

    auto typ = Py_TYPE(arg);

    if (typ == state->BufferType) {
        return inspect_buffer((PyCaravanBuffer *)arg);
    }
    if (typ == state->PipelineType) {
        return inspect_pipeline((PyCaravanPipeline *)arg);
    }
    if (typ == state->TextureType) {
        return inspect_texture((PyCaravanTexture *)arg);
    }
    if (typ == state->UniformBatchType) {
        return inspect_uniform_batch((PyCaravanUniformBatch *)arg);
    }

    if (typ == state->ProgramType) {
        return FastBuild_Dict("type", "program", "id", (Py_ssize_t)((PyCaravanProgram *)arg)->id);
    }
    if (typ == state->VertexArrayType) {
        return FastBuild_Dict("type", "vertex_array", "id",
                              (Py_ssize_t)((PyCaravanVertexArray *)arg)->id);
    }

    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_clear(PyObject *mod, PyObject *const *args, Py_ssize_t nargs,
                              PyObject *kwnames) {
    WithCaravanGL(mod, gl) {
        uint32_t mask = 0;
        void *targets[Clear_COUNT] = {[IDX_CLR_MASK] = &mask};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ClearParser, targets)) {
            return nullptr;
        }

        gl.Clear(mask);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_clear_color(PyObject *mod, PyObject *const *args, Py_ssize_t nargs,
                                    PyObject *kwnames) {
    WithCaravanGL(mod, gl) {
        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        float alpha = 1.0f;
        void *targets[ClearColor_COUNT] = {[IDX_CLR_C_R] = &red,
                                           [IDX_CLR_C_G] = &green,
                                           [IDX_CLR_C_B] = &blue,
                                           [IDX_CLR_C_A] = &alpha};

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ClearColorParser, targets)) {
            return nullptr;
        }

        const GLfloat color[] = {red, green, blue, alpha};
        gl.ClearBufferfv(GL_COLOR, 0, color);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_enable_debug([[maybe_unused]] PyObject *mod,
                                     [[maybe_unused]] PyObject *args) {
#if !defined(__APPLE__)
    WithCaravanGL(mod, gl) {
        if (!gl.DebugMessageCallback) {
            PyErr_SetString(PyExc_RuntimeError, "Debug Output not supported on this driver.");
            return nullptr;
        }

        // 1. Enable debug mode in the driver
        gl.Enable(GL_DEBUG_OUTPUT);

        // 2. Force synchronous calls. This blocks the CPU until the callback
        // finishes. It makes performance worse, but gives you highly accurate error
        // call-stacks.
        gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        // 3. Register the callback and pass 'state' as the userParam
        gl.DebugMessageCallback(opengl_debug_callback, state);

        // 4. Control what messages you want to hear:

        // Enable everything by default
        gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

        // Example: Disable "Notification" level spam (like buffer memory placement
        // info)
        gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0,
                               nullptr, GL_FALSE);

        // Example: If you wanted to disable specific driver error IDs manually:
        // GLuint skip_ids[] = { 1234, 5678 };
        // gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 2,
        // skip_ids, GL_FALSE);
    }
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_RuntimeError, "OpenGL 4.3 Debug Output is not available on macOS.");
    return nullptr;
#endif
}

static const PyMethodDef caravan_methods[] = {
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

static int init_types(PyObject *mod, CaravanState *state) {
    struct {
        [[gnu::aligned(32)]]
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
    if (!mod_name) {
        return -1;
    }
#pragma unroll 4
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        auto type = PyType_FromModuleAndSpec(mod, (PyType_Spec *)types[i].spec, nullptr);
        if (!type) {
            Py_DECREF(mod_name);
            return -1;
        }

        // Set __module__ for better repr() and pickling
        PyObject_SetAttrString(type, "__module__", mod_name);

        // Add to module (PyModule_AddObjectRef is CPython 3.10+ and safer)
        if (PyModule_AddObjectRef(mod, types[i].name, type) < 0) {
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

static int init_constants(PyObject *mod) {
    static const struct {
        [[gnu::aligned(16)]]
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
                  {"ELEMENT_ARRAY_BUFFER", GL_ELEMENT_ARRAY_BUFFER},
                  {"UNSIGNED_SHORT", GL_UNSIGNED_SHORT},
                  {"UNSIGNED_INT", GL_UNSIGNED_INT},
                  {"UNSIGNED_BYTE", GL_UNSIGNED_BYTE},

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
#pragma unroll
    for (size_t i = 0; i < sizeof(consts) / sizeof(consts[0]); i++) {
        if (PyModule_AddIntConstant(mod, consts[i].name, consts[i].value) < 0) {
            return -1;
        }
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Module Lifecycle Implementation
// -----------------------------------------------------------------------------

PyCaravanGL_Status caravan_exec(PyObject *mod) {
    CaravanState *state = get_caravan_state(mod);
    if (state == nullptr) {
        return -1;
    }

    // Zero out state and prepare fast-path parsers
    memset(state, 0, sizeof(CaravanState));
    caravan_init_parsers(&state->parsers);

    // Register Classes
    if (init_types(mod, state) < 0) {
        return -1;
    }

    // Register Constants
    if (init_constants(mod) < 0) {
        return -1;
    }

    return 0;
}

[[nodiscard, gnu::always_inline]]
static inline int caravan_visit(PyTypeObject *obj, visitproc visit, void *arg) {
    if (obj) {
        int res = visit((PyObject *)obj, arg);
        if (res) {
            return res;
        }
    }
    return 0;
}

[[gnu::always_inline]]
static inline void py_caravan_clear(PyTypeObject **ptr) {
    // We take the address of the pointer so we can null it out
    PyTypeObject *tmp = *ptr;
    if (tmp != nullptr) {
        *ptr = nullptr;
        Py_DECREF(tmp);
    }
}

// A generic function pointer type for operating on your state members
typedef int (*caravan_op_t)(PyTypeObject **ptr, void *data);

[[gnu::always_inline]]
static inline int caravan_dispatch(CaravanState *state, caravan_op_t op, void *data) {
    // Single source of truth for all managed types
    PyTypeObject **members[] = {
        &state->BufferType,      &state->PipelineType,     &state->ProgramType,
        &state->VertexArrayType, &state->UniformBatchType, &state->TextureType,
    };

#pragma unroll
    for (size_t i = 0; i < sizeof(members) / sizeof(members[0]); ++i) {
        int res = op(members[i], data);
        if (res) {
            return res;
        }
    }
    return 0;
}

static int op_visit(PyTypeObject **ptr, void *arg) {
    // We reuse your existing caravan_visit logic
    return caravan_visit(*ptr, (visitproc)((void **)arg)[0], ((void **)arg)[1]);
}

static int op_clear(PyTypeObject **ptr, [[maybe_unused]] void *unused) {
    py_caravan_clear(ptr);
    return 0;
}

PyCaravanGL_Status caravan_traverse(PyObject *mod, visitproc visit, void *arg) {
    CaravanState *state = get_caravan_state(mod);
    if (!state) {
        return 0;
    }

    // Package visit and arg to pass through the dispatcher
    void *params[] = {(void *)visit, arg};
    return caravan_dispatch(state, op_visit, (void *)params);
}

PyCaravanGL_Status slot_caravan_clear(PyObject *mod) {
    CaravanState *state = get_caravan_state(mod);
    if (!state) {
        return 0;
    }

    caravan_free_parsers(&state->parsers);
    return caravan_dispatch(state, op_clear, nullptr);
}

static const PyModuleDef_Slot caravan_slots[] = {
    {Py_mod_exec, caravan_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {}};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static PyModuleDef caravan_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "caravangl",
    .m_doc = "CaravanGL: Modern Isolated OpenGL Context",
    .m_size = sizeof(CaravanState),
    .m_methods = (PyMethodDef *)caravan_methods,
    .m_slots = (PyModuleDef_Slot *)caravan_slots,
    .m_traverse = caravan_traverse,
    .m_clear = slot_caravan_clear,
    .m_free = nullptr,
};

extern PyMODINIT_FUNC PyInit_caravangl(void) {
    return PyModuleDef_Init(&caravan_module);
}