#include "caravangl_context.h"
#include "caravangl_loader.h"

// Define the TLS variable
thread_local PyCaravanContext *cv_active_context = nullptr;

// -----------------------------------------------------------------------------
// Internal Helpers
// -----------------------------------------------------------------------------

/**
 * Queries GPU hardware limits and populates the state context.
 * Called once during caravan.init() (already inside the lock).
 */
static void query_capabilities(PyCaravanContext *self) {
    CaravanGLTable *OpenGL = &self->gl;
    CaravanContext *ctx = &self->ctx;
    if (OpenGL->GetIntegerv == nullptr) {
        return;
    }

    // Textures
    OpenGL->GetIntegerv(GL_MAX_TEXTURE_SIZE, &ctx->caps.max_texture_size);
    OpenGL->GetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &ctx->caps.max_3d_texture_size);
    OpenGL->GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &ctx->caps.max_array_texture_layers);
    OpenGL->GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ctx->caps.max_texture_units);

    // FBOs & Buffers
    OpenGL->GetIntegerv(GL_MAX_SAMPLES, &ctx->caps.max_samples);
    OpenGL->GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &ctx->caps.max_color_attachments);
    OpenGL->GetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &ctx->caps.max_uniform_block_size);
    OpenGL->GetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &ctx->caps.max_ubo_bindings);

    // Viewport
    GLint viewport[4] = {};
    OpenGL->GetIntegerv(GL_VIEWPORT, viewport);
    ctx->viewport = (CaravanRect){
        .x = viewport[0], .y = viewport[1], .width = viewport[2], .height = viewport[3]};

#ifndef __APPLE__
    ctx->caps.support_compute = (OpenGL->DispatchCompute != nullptr);
    ctx->caps.support_bindless = (OpenGL->GetTextureHandleARB != nullptr);

    if (ctx->caps.support_compute && OpenGL->GetIntegerv != nullptr) {
        OpenGL->GetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                            &ctx->caps.max_compute_work_group_invocations);
        OpenGL->GetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,
                            &ctx->caps.max_shader_storage_block_size);
    }
#else
    // Hardcode to false for Mac (OpenGL 4.1 limit)
    ctx->caps.support_compute = false;
    ctx->caps.support_bindless = false;
    ctx->caps.max_compute_work_group_invocations = 0;
    ctx->caps.max_shader_storage_block_size = 0;
#endif
}

void cv_enqueue_garbage(size_t *count, GLuint *array, GLuint id) {
    if (id == 0) {
        return;
    }
    if (*count < CARAVAN_GARBAGE_SIZE) {
        array[(*count)++] = id;
    } else {
        // Fallback: If the queue is totally jammed, we log a warning.
        // In a high-performance engine, you might realloc here,
        // but for GL 3.3, 256 objects per context-switch is usually plenty.
        // NOLINTNEXTLINE
        (void)fprintf(stderr, "[CaravanGL] Critical: Garbage queue overflow. ID %u leaked.\n", id);
    }
}

/**
 * Flushes pending deletes. Must be called WHILE the context is locked
 * and OS-Current.
 */
void cv_flush_garbage(PyCaravanContext *self) {
    // This is called while the MagMutex is already held by Context_make_current
    CaravanGarbage *garbage = &self->garbage;

    if (garbage->buffer_count > 0) {
        self->gl.DeleteBuffers((GLsizei)garbage->buffer_count, garbage->buffers);
        garbage->buffer_count = 0;
    }

    if (garbage->texture_count > 0) {
        self->gl.DeleteTextures((GLsizei)garbage->texture_count, garbage->textures);
        garbage->texture_count = 0;
    }

    if (garbage->vao_count > 0) {
        self->gl.DeleteVertexArrays((GLsizei)garbage->vao_count, garbage->vaos);
        garbage->vao_count = 0;
    }

    if (garbage->fbo_count > 0) {
        self->gl.DeleteFramebuffers((GLsizei)garbage->fbo_count, garbage->fbos);
        garbage->fbo_count = 0;
    }

    if (garbage->rbo_count > 0) {
        self->gl.DeleteRenderbuffers((GLsizei)garbage->rbo_count, garbage->rbos);
        garbage->rbo_count = 0;
    }

    if (garbage->sampler_count > 0) {
        self->gl.DeleteSamplers((GLsizei)garbage->sampler_count, garbage->samplers);
        garbage->sampler_count = 0;
    }

    if (garbage->program_count > 0) {
#pragma unroll 4
        for (int i = 0; i < garbage->program_count; i++) {
            self->gl.DeleteProgram(garbage->programs[i]);
        }
        garbage->program_count = 0;
    }

    if (garbage->sync_count > 0) {
#pragma unroll 4
        for (int i = 0; i < garbage->sync_count; i++) {
            self->gl.DeleteSync(garbage->syncs[i]);
        }
        garbage->sync_count = 0;
    }

    if (garbage->query_count > 0) {
        self->gl.DeleteQueries((GLsizei)garbage->query_count, garbage->queries);
        garbage->query_count = 0;
    }
}

/**
 * ctx.make_current()
 * Activates this context for the current thread.
 */
PyCaravanGL_API Context_make_current(PyCaravanContext *self, [[maybe_unused]] PyObject *args) {
    // 1. LOCK FIRST. No one can steal the context or draw while we are switching.
    MagMutex_Lock(&self->ctx.state_lock);

    // 2. Call the OS callback while holding the lock
    if (self->os_make_current_cb && self->os_make_current_cb != Py_None) {
        PyObject *res = PyObject_CallNoArgs(self->os_make_current_cb);
        if (!res) {
            MagMutex_Unlock(&self->ctx.state_lock); // Don't forget to unlock on error!
            return nullptr;
        }
        Py_DECREF(res);
    }

    // 3. Set the Thread-Local pointer
    cv_active_context = self;

    // 4. Flush garbage
    cv_flush_garbage(self);

    // 5. UNLOCK
    MagMutex_Unlock(&self->ctx.state_lock);

    Py_RETURN_NONE;
}

PyCaravanGL_Status Context_init(PyCaravanContext *self, PyObject *args, PyObject *kwds) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    PyObject *loader = nullptr;
    PyObject *callback = Py_None;

    void *targets[ContextInit_COUNT] = {[IDX_CTX_LOADER] = (void *)&loader,
                                        [IDX_CTX_CALLBACK] = (void *)&callback};

    // Use FastParse for context initialization
    if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.ContextInitParser, targets)) {
        return -1;
    }

    // 1. Initialize Thread-Safety and Shadow State
    memset(&self->ctx.state_lock, 0, sizeof(MagMutex));

    // Zero out the tracking structures
    memset(&self->ctx.bound, 0, sizeof(self->ctx.bound));
    memset(&self->garbage, 0, sizeof(self->garbage));

    // Initialize viewport to a safe "null" state
    self->ctx.viewport = (CaravanRect){0, 0, 0, 0};

    // 2. Load OpenGL functions directly into this context instance
    // Note: We use the refactored load_gl_table from caravangl_loader.h
    if (load_gl_table(&self->gl, loader) < 0) {
        // load_gl_table already raises the Python RuntimeError on failure
        return -1;
    }

    // Now populate the hardware capabilities for this specific context
    query_capabilities(self);

    // 3. Handle the Make-Current Callback
    // If user provided a callback, store it; otherwise it stays Py_None
    self->os_make_current_cb = Py_NewRef(callback);

    return 0;
}

PyCaravanGL_Slot Context_dealloc(PyCaravanContext *self) {
    PyTypeObject *type = Py_TYPE(self);
    Py_XDECREF(self->os_make_current_cb);
    type->tp_free((PyObject *)self);
    Py_DECREF(type);
}

PyCaravanGL_Status Context_traverse(PyCaravanContext *self, visitproc visit, void *arg) {
    Py_VISIT(self->os_make_current_cb);
    return 0;
}

PyCaravanGL_Status Context_clear(PyCaravanContext *self) {
    Py_CLEAR(self->os_make_current_cb);
    return 0;
}

// Getter for the callback
PyCaravanGL_API Context_get_callback(PyCaravanContext *self, [[maybe_unused]] void *closure) {
    return Py_NewRef(self->os_make_current_cb);
}

// Setter for the callback
PyCaravanGL_Status Context_set_callback(PyCaravanContext *self, PyObject *value, void *closure) {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the os_make_current_cb attribute");
        return -1;
    }
    PyObject *old = self->os_make_current_cb;
    self->os_make_current_cb = Py_NewRef(value);
    Py_XDECREF(old);
    return 0; // Success
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
const PyType_Spec Context_spec = {
    .name = "caravangl.Context",
    .basicsize = sizeof(PyCaravanContext),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = (PyType_Slot[]){
        {Py_tp_init, Context_init},
        {Py_tp_dealloc, Context_dealloc},
        {Py_tp_getset, (PyGetSetDef[]){{"os_make_current_cb", (getter)Context_get_callback,
                                        (setter)Context_set_callback,
                                        "Callback to make this context current in the OS", nullptr},
                                       {}}},
        {Py_tp_methods, (PyMethodDef[]){{"make_current", (PyCFunction)Context_make_current,
                                         METH_NOARGS, "Activate this context"},
                                        {}}},
        {Py_tp_traverse, Context_traverse},
        {Py_tp_clear, Context_clear},
        {}}};