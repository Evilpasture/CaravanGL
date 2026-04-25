/**
 * ============================================================================
 * CaravanGL
 * ============================================================================
 * Architecture: C23 + Python 3.14t (Free-Threaded)
 * Features: Isolated State, Fast-Path Parsing, Register-Speed Build, No-GIL.
 * ============================================================================
 */

#include "caravangl_context.h"
#include "caravangl_module.h"
#include "caravangl_state.h"
#define PY_SSIZE_T_CLEAN
#include "caravangl_arg_indices.h"
#include "caravangl_pyspec.h"
#include "fast_build.h"
#include "pycaravangl.h"
#include <Python.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Module-Level API
// -----------------------------------------------------------------------------

/**
 * caravangl.context() -> dict
 * Returns a snapshot of capabilities and driver info for the active context.
 */
static inline PyObject *build_context_dict(CaravanContext *ctx,
                                           const CaravanGLTable *const OpenGL) {
    PyObject *caps =
        FastBuild_Dict("max_texture_size", ctx->caps.max_texture_size, "max_samples",
                       ctx->caps.max_samples, "support_compute", ctx->caps.support_compute,
                       "support_bindless", ctx->caps.support_bindless);

    PyObject *info = FastBuild_Dict("vendor", (const char *)OpenGL->GetString(GL_VENDOR),
                                    "renderer", (const char *)OpenGL->GetString(GL_RENDERER),
                                    "version", (const char *)OpenGL->GetString(GL_VERSION));

    return FastBuild_Dict("caps", caps, "info", info, "viewport",
                          FastBuild_Tuple(ctx->viewport.x, ctx->viewport.y, ctx->viewport.width,
                                          ctx->viewport.height));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_API caravan_meth_context([[maybe_unused]] PyObject *mod,
                                     [[maybe_unused]] PyObject *args) {
    // We use WithActiveGL to grab the TLS-bound context automatically
    WithActiveGL(OpenGL, cv_state, nullptr) {
        if (OpenGL->GetString == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "OpenGL loader not initialized.");
            return nullptr;
        }

        return build_context_dict(cv_state, OpenGL);
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
    return FastBuild_Dict(
        "type", "buffer", "id", (Py_ssize_t)buf->buf.id, "target", (Py_ssize_t)buf->buf.target,
        "size", (Py_ssize_t)buf->buf.size, "usage", (Py_ssize_t)buf->buf.usage,
        // NEW: Export the capability flags
        "is_immutable", (bool)buf->buf.is_immutable, "is_persistent", (bool)buf->buf.is_persistent);
}

static inline PyObject *inspect_pipeline(PyCaravanPipeline *pip) {
    // We explicitly cast bitfield members to (bool) so that the
    // FastBuild dispatcher creates Python Booleans instead of Integers.
    auto state_dict = FastBuild_Dict(
        // -- Booleans (Casts required for bitfields) --
        "depth_test", (bool)pip->render_state.depth_test_enabled, "depth_write",
        (bool)pip->render_state.depth_write_mask, "stencil_test",
        (bool)pip->render_state.stencil_test_enabled, "blend",
        (bool)pip->render_state.blend_enabled, "cull", (bool)pip->render_state.cull_face_enabled,

        // -- Enums (Remain as PyLong/Integers) --
        "depth_func", (Py_ssize_t)pip->render_state.depth_func, "stencil_func",
        (Py_ssize_t)pip->render_state.stencil_func, "stencil_ref",
        (Py_ssize_t)pip->render_state.stencil_ref, "stencil_zpass",
        (Py_ssize_t)pip->render_state.stencil_zpass_op, "blend_src_rgb",
        (Py_ssize_t)pip->render_state.blend_src_rgb, "blend_dst_rgb",
        (Py_ssize_t)pip->render_state.blend_dst_rgb, "cull_mode",
        (Py_ssize_t)pip->render_state.cull_face_mode);

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

static inline PyObject *inspect_compute_pipeline(PyCaravanComputePipeline *comp) {
    return FastBuild_Dict("type", "compute_pipeline", "id", (Py_ssize_t)comp->id);
}

// --- Main Dispatcher ---
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_API caravan_meth_inspect(PyObject *mod, PyObject *arg) {
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

    if (typ == state->ComputePipelineType) {
        return inspect_compute_pipeline((PyCaravanComputePipeline *)arg);
    }

    if (typ == state->ProgramType) {
        return FastBuild_Dict("type", "program", "id", (Py_ssize_t)((PyCaravanProgram *)arg)->id);
    }
    if (typ == state->VertexArrayType) {
        return FastBuild_Dict("type", "vertex_array", "id",
                              (Py_ssize_t)((PyCaravanVertexArray *)arg)->id);
    }
    if (typ == state->SamplerType) {
        auto sampler = (PyCaravanSampler *)arg;
        return FastBuild_Dict("type", "sampler", "id", (Py_ssize_t)sampler->id);
    }

    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_meth_clear(PyObject *mod, PyObject *const *args, Py_ssize_t nargs,
                                   PyObject *kwnames) {
    auto state = get_caravan_state(mod);
    uint32_t mask = 0;
    void *targets[Clear_COUNT] = {[IDX_CLR_MASK] = &mask};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ClearParser, targets)) {
        return nullptr;
    }
    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_resolve(cv_state, OpenGL); // Resolve FBO/Viewport before clearing
        OpenGL->Clear(mask);
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_meth_clear_color(PyObject *mod, PyObject *const *args, Py_ssize_t nargs,
                                         PyObject *kwnames) {
    // We still need the module state to access the static, read-only Parsers
    auto state = get_caravan_state(mod);

    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
    float alpha = 1.0F;
    void *targets[ClearColor_COUNT] = {[IDX_CLR_C_R] = &red,
                                       [IDX_CLR_C_G] = &green,
                                       [IDX_CLR_C_B] = &blue,
                                       [IDX_CLR_C_A] = &alpha};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ClearColorParser, targets)) {
        return nullptr;
    }

    // Use Active Context to perform the GL call
    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_resolve(cv_state, OpenGL); // Resolve FBO/Viewport before clearing
        const GLfloat color[] = {red, green, blue, alpha};
        OpenGL->ClearBufferfv(GL_COLOR, 0, color);
    }
    Py_RETURN_NONE;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_API caravan_meth_bind_default_framebuffer([[maybe_unused]] PyObject *mod,
                                                      [[maybe_unused]] PyObject *args) {
    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_set_fbo_combined(cv_state, 0); // Lazy bind
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API caravan_meth_viewport(PyObject *mod, PyObject *const *args, Py_ssize_t nargs,
                                      PyObject *kwnames) {
    // Keep module state for parsers (parsers are global/immutable)
    auto state = get_caravan_state(mod);

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    void *targets[Viewport_COUNT] = {&x, &y, &width, &height};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.ViewportParser, targets)) {
        return nullptr;
    }

    // WithActiveGL automatically extracts the correct context for this thread
    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_set_viewport(
            cv_state,
            &(CaravanRect){.x = x, .y = y, .width = width, .height = height}); // Lazy bind
    }
    Py_RETURN_NONE;
}

/**
 * caravangl.get_active_context() -> caravangl.Context | None
 * Allows Python to see which context is active on the current thread.
 */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_API caravan_meth_get_active_context([[maybe_unused]] PyObject *mod,
                                                [[maybe_unused]] PyObject *args) {
    if (cv_active_context) {
        return Py_NewRef(cv_active_context);
    }
    Py_RETURN_NONE;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_API caravan_meth_enable_debug([[maybe_unused]] PyObject *mod,
                                          [[maybe_unused]] PyObject *args) {
#ifndef __APPLE__
    // We fetch the module state to pass as the userParam to the callback.
    // This allows the callback to access module-level resources without globals.
    CaravanState *state = get_caravan_state(mod);

    WithActiveGL(OpenGL, cv_state, nullptr) {
        if (!OpenGL->DebugMessageCallback) {
            PyErr_SetString(PyExc_RuntimeError, "Debug Output not supported on this driver.");
            return nullptr;
        }

        // 1. Enable debug mode for the ACTIVE context on this thread
        OpenGL->Enable(GL_DEBUG_OUTPUT);

        // 2. Force synchronous calls for accurate Python-side call stacks
        OpenGL->Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        // 3. Register the callback and pass 'state' as the userParam
        OpenGL->DebugMessageCallback(opengl_debug_callback, state);

        // 4. Set default filters
        // Enable everything...
        OpenGL->DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

        // ...except for "Notification" level spam (driver-specific info)
        OpenGL->DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0,
                                    nullptr, GL_FALSE);
    }
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_RuntimeError, "OpenGL 4.3 Debug Output is not available on macOS.");
    return nullptr;
#endif
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_API caravan_meth_memory_barrier([[maybe_unused]] PyObject *mod, PyObject *arg) {
    [[maybe_unused]] uint32_t mask = PyLong_AsUnsignedLong(arg);
    WithActiveGL(OpenGL, cv_state, nullptr) {
#ifndef __APPLE__
        if (OpenGL->MemoryBarrier) {
            OpenGL->MemoryBarrier((GLbitfield)mask);
        }
#endif
    }
    Py_RETURN_NONE;
}

// -----------------------------------------------------------------------------
// Module Lifecycle
// -----------------------------------------------------------------------------

/**
 * Internal helpers for module initialization.
 */

static int init_types(PyObject *mod, CaravanState *state) {
    constexpr auto alignment = 32;
    struct {
        [[gnu::aligned(alignment)]]
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
        {&Framebuffer_spec, (PyObject **)&state->FramebufferType, "Framebuffer"},
        {&Sampler_spec, (PyObject **)&state->SamplerType, "Sampler"},
        {&Context_spec, (PyObject **)&state->ContextType, "Context"},
        {&Sync_spec, (PyObject **)&state->SyncType, "Sync"},
        {&Query_spec, (PyObject **)&state->QueryType, "Query"},
        {&ComputePipeline_spec, (PyObject **)&state->ComputePipelineType, "ComputePipeline"},
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

static int CaravanModule_AddUnsignedLongLongConstant(PyObject *mod, const char *name,
                                                     unsigned long long value) {
    PyObject *obj = PyLong_FromUnsignedLongLong(value);
    if (!obj) {
        return -1;
    }
    if (PyModule_AddObjectRef(mod, name, obj) < 0) {
        Py_DECREF(obj);
        return -1;
    }
    Py_DECREF(obj);
    return 0;
}

static int init_constants(PyObject *mod) {
    // 1. Compile-time Duplicate Check
    // If name is duplicated, this enum will have two identical identifiers
    enum GL_Duplicate_Check {
#define X(name, val) CHECK_##name,
#include "gl_constants.inc"
#undef X
    };

    // 2. Direct Expansion Registration
    // This expands into a series of if-statements. No array storage needed.
#define X(name, val)                                                                               \
    if (CaravanModule_AddUnsignedLongLongConstant(mod, #name, (unsigned long long)(val)) < 0) {    \
        return -1;                                                                                 \
    }

#include "gl_constants.inc"
#undef X

    // 3. Dynamic Build Metadata (Manual)
    if (PyModule_AddIntConstant(mod, "FREE_THREADED", 
#if defined(Py_GIL_DISABLED) && Py_GIL_DISABLED
        1
#else
        0
#endif
    ) < 0) return -1;

    if (PyModule_AddIntConstant(mod, "DEBUG_BUILD", 
#ifdef CARAVANGL_DEBUG
        1
#else
        0
#endif
    ) < 0) return -1;

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
    *state = (CaravanState){};

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

PyCaravanGL_Status caravan_traverse(PyObject *module, visitproc visit, void *arg) {
    CaravanState *state = get_caravan_state(module);
    if (state == nullptr) {
        return 0;
    }

    // List of module-level types to visit
    PyObject **members[] = {
        (PyObject **)&state->BufferType,          (PyObject **)&state->PipelineType,
        (PyObject **)&state->ProgramType,         (PyObject **)&state->VertexArrayType,
        (PyObject **)&state->UniformBatchType,    (PyObject **)&state->TextureType,
        (PyObject **)&state->ContextType,         (PyObject **)&state->SyncType,
        (PyObject **)&state->QueryType,           (PyObject **)&state->FramebufferType,
        (PyObject **)&state->ComputePipelineType,
    };

    TraverseContext context = {.visit = visit, .arg = arg};

    return caravan_dispatch_members(members, sizeof(members) / sizeof(members[0]), op_visit_member,
                                    &context);
}

PyCaravanGL_Status caravan_clear(PyObject *module) {
    CaravanState *state = get_caravan_state(module);
    if (state == nullptr) {
        return 0;
    }

    // 1. Clean up non-Python resources (parsers)
    caravan_free_parsers(&state->parsers);

    // 2. Clear module-level type references
    PyObject **members[] = {
        (PyObject **)&state->BufferType,          (PyObject **)&state->PipelineType,
        (PyObject **)&state->ProgramType,         (PyObject **)&state->VertexArrayType,
        (PyObject **)&state->UniformBatchType,    (PyObject **)&state->TextureType,
        (PyObject **)&state->ContextType,         (PyObject **)&state->SyncType,
        (PyObject **)&state->QueryType,           (PyObject **)&state->FramebufferType,
        (PyObject **)&state->ComputePipelineType,
    };

    return caravan_dispatch_members(members, sizeof(members) / sizeof(members[0]), op_clear_member,
                                    nullptr);
}

#define MODULE_NOARGS(name)                                                                        \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(caravan_meth_, name)), METH_NOARGS, nullptr}
#define MODULE_FASTCALL(name)                                                                      \
    {#name, CARAVAN_CAST(CARAVAN_JOIN(caravan_meth_, name)), METH_FASTCALL | METH_KEYWORDS, nullptr}
#define MODULE_O(name) {#name, CARAVAN_CAST(CARAVAN_JOIN(caravan_meth_, name)), METH_O, nullptr}
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static PyModuleDef caravan_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "caravangl",
    .m_doc = "CaravanGL: Modern Isolated OpenGL Context",
    .m_size = sizeof(CaravanState),
    .m_methods =
        (PyMethodDef[]){

            MODULE_NOARGS(enable_debug),
            MODULE_NOARGS(context),
            MODULE_O(inspect),
            MODULE_FASTCALL(clear),
            MODULE_FASTCALL(clear_color),
            MODULE_NOARGS(bind_default_framebuffer),
            MODULE_FASTCALL(viewport),
            MODULE_NOARGS(get_active_context),
            MODULE_O(memory_barrier),
            {}

        },
    .m_slots =
        (PyModuleDef_Slot[]){

            {Py_mod_exec, caravan_exec},
            {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
            {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
            {}

        },
    .m_traverse = caravan_traverse,
    .m_clear = caravan_clear,
    .m_free = nullptr,
};

extern PyMODINIT_FUNC PyInit_caravangl(void) {
    return PyModuleDef_Init(&caravan_module);
}