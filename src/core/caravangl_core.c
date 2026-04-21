#include "caravangl_context.h"
#include <stdio.h>

thread_local CaravanHandle *cv_current_handle = nullptr;

void caravan_init_handle(CaravanHandle *h) {
    *h = (CaravanHandle){};
    h->ctx.state_lock = (MagMutex){};

    // Initialize viewport to a safe "null" state
    h->ctx.viewport = (CaravanRect){0, 0, 0, 0};
}

void caravan_query_caps(CaravanHandle *h) {
    const CaravanGLTable *const OpenGL = &h->gl;
    CaravanContext *ctx = &h->ctx;

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
    GLint vp[4] = {};
    OpenGL->GetIntegerv(GL_VIEWPORT, vp);
    ctx->viewport = (CaravanRect){.x = vp[0], .y = vp[1], .width = vp[2], .height = vp[3]};

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

void cv_enqueue_garbage(size_t *count, size_t capacity, GLuint array[static 1], GLuint id) {
    if (id == 0) [[clang::unlikely]] {
        return;
    }

    if (*count < capacity) [[clang::likely]] {
        array[(*count)++] = id;
    } else {
        // This is now much more useful as it tells you which specific limit was hit
        // NOLINTNEXTLINE
        (void)fprintf(stderr,
                      "[CaravanGL] Critical: Garbage queue overflow (Cap: %zu). ID %u leaked.\n",
                      capacity, id);
    }
}

void caravan_flush_garbage(CaravanHandle *h) {
    CaravanGarbage *garbage = &h->garbage;
    const CaravanGLTable *const OpenGL = &h->gl;

    if (garbage->buffer_count > 0) {
        OpenGL->DeleteBuffers((GLsizei)garbage->buffer_count, garbage->buffers);
        garbage->buffer_count = 0;
    }
    if (garbage->texture_count > 0) {
        OpenGL->DeleteTextures((GLsizei)garbage->texture_count, garbage->textures);
        garbage->texture_count = 0;
    }
    if (garbage->vao_count > 0) {
        OpenGL->DeleteVertexArrays((GLsizei)garbage->vao_count, garbage->vaos);
        garbage->vao_count = 0;
    }
    if (garbage->fbo_count > 0) {
        OpenGL->DeleteFramebuffers((GLsizei)garbage->fbo_count, garbage->fbos);
        garbage->fbo_count = 0;
    }
    if (garbage->rbo_count > 0) {
        OpenGL->DeleteRenderbuffers((GLsizei)garbage->rbo_count, garbage->rbos);
        garbage->rbo_count = 0;
    }
    if (garbage->sampler_count > 0) {
        OpenGL->DeleteSamplers((GLsizei)garbage->sampler_count, garbage->samplers);
        garbage->sampler_count = 0;
    }
    if (garbage->program_count > 0) {
#pragma unroll 4
        for (size_t i = 0; i < garbage->program_count; i++) {
            OpenGL->DeleteProgram(garbage->programs[i]);
        }
        garbage->program_count = 0;
    }
    if (garbage->sync_count > 0) {
#pragma unroll 4
        for (size_t i = 0; i < garbage->sync_count; i++) {
            OpenGL->DeleteSync(garbage->syncs[i]);
        }
        garbage->sync_count = 0;
    }
    if (garbage->query_count > 0) {
        OpenGL->DeleteQueries((GLsizei)garbage->query_count, garbage->queries);
        garbage->query_count = 0;
    }
}

void caravan_make_current(CaravanHandle *h) {
    cv_current_handle = h;
    if (h != nullptr) {
        caravan_flush_garbage(h);
    }
}

void cv_enqueue_garbage_ptr(size_t *count, size_t capacity, GLsync array[static 1], void *ptr) {
    if (ptr == nullptr) {
        return;
    }
    if (*count < capacity) {
        array[(*count)++] = ptr;
    } else {
        // NOLINTNEXTLINE
        (void)fprintf(stderr, "[CaravanGL] Critical: Sync garbage overflow. Memory leaked.\n");
    }
}