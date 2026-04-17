// --- START OF FILE caravangl_state.h ---
#pragma once
#include "caravangl_module.h"
#include "caravangl_specs.h"

// -----------------------------------------------------------------------------
// Core Bindings (VAO, FBO, Program)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_program(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                   GLuint program) {
    if (ctx->bound.program != program) [[clang::unlikely]] {
        ctx->bound.program = program;
        OpenGL->UseProgram(program);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_vao(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                               GLuint vao) {
    if (ctx->bound.vao != vao) [[clang::unlikely]] {
        ctx->bound.vao = vao;
        OpenGL->BindVertexArray(vao);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_draw(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                    GLuint fbo) {
    if (ctx->bound.fbo_draw != fbo) [[clang::unlikely]] {
        OpenGL->BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        ctx->bound.fbo_draw = fbo;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_read(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                    GLuint fbo) {
    if (ctx->bound.fbo_read != fbo) [[clang::unlikely]] {
        OpenGL->BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        ctx->bound.fbo_read = fbo;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_combined(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                        GLuint fbo) {
    if (ctx->bound.fbo_draw != fbo || ctx->bound.fbo_read != fbo) [[clang::unlikely]] {
        OpenGL->BindFramebuffer(GL_FRAMEBUFFER, fbo);
        ctx->bound.fbo_draw = fbo;
        ctx->bound.fbo_read = fbo;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_viewport(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                    const CaravanRect *viewport) {
    CaravanRect *cview = &ctx->viewport;
    if (viewport->x != cview->x || viewport->y != cview->y || viewport->width != cview->width ||
        viewport->height != cview->height) [[clang::unlikely]] {
        OpenGL->Viewport(viewport->x, viewport->y, viewport->width, viewport->height);
        *cview = *viewport;
    }
}

// -----------------------------------------------------------------------------
// Resource Bindings (UBO, SSBO, Textures)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_ubo_range(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                     GLuint index, GLuint buffer, GLintptr offset,
                                     GLsizeiptr size) {
    if (index >= CARAVAN_MAX_UBO_BINDINGS) [[clang::unlikely]] {
        return;
    }

    CaravanBufferBinding *binding = &ctx->bound.ubo[index];
    if (binding->id != buffer || binding->offset != offset || binding->size != size)
        [[clang::unlikely]] {
        OpenGL->BindBufferRange(GL_UNIFORM_BUFFER, index, buffer, offset, size);
        binding->id = buffer;
        binding->offset = offset;
        binding->size = size;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_texture(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                   GLuint unit, const CaravanTexture *tex, GLuint sampler) {
    if (unit >= CARAVAN_MAX_TEXTURE_UNITS) [[clang::unlikely]] {
        return;
    }

    CaravanTextureBinding *binding = &ctx->bound.texture_units[unit];
    GLuint texture_id = tex->id;
    GLenum target = tex->target;

    if (binding->id != texture_id || binding->target != target) {
#ifndef __APPLE__
        if (OpenGL->BindTextureUnit != nullptr) {
            OpenGL->BindTextureUnit(unit, texture_id);
        } else
#endif
        {
            if (ctx->bound.active_texture_unit != unit) {
                OpenGL->ActiveTexture(GL_TEXTURE0 + unit);
                ctx->bound.active_texture_unit = unit;
            }
            OpenGL->BindTexture(target, texture_id);
        }
        binding->id = texture_id;
        binding->target = target;
    }

    if (binding->sampler_id != sampler) {
        OpenGL->BindSampler(unit, sampler);
        binding->sampler_id = sampler;
    }
}

// -----------------------------------------------------------------------------
// Render State Binders (Depth, Blend, Cull)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_sync_render_state(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                        const CaravanRenderState *req) {
    CaravanRenderState *curr = &ctx->bound.render;

    // 1. Depth State Sync
    if (curr->depth_test_enabled != req->depth_test_enabled) {
        (int)req->depth_test_enabled ? OpenGL->Enable(GL_DEPTH_TEST)
                                     : OpenGL->Disable(GL_DEPTH_TEST);
        curr->depth_test_enabled = req->depth_test_enabled;
    }

    if (req->depth_test_enabled) {
        if (curr->depth_func != req->depth_func) {
            OpenGL->DepthFunc(req->depth_func);
            curr->depth_func = req->depth_func;
        }
        if (curr->depth_write_mask != req->depth_write_mask) {
            OpenGL->DepthMask((GLboolean)req->depth_write_mask ? GL_TRUE : GL_FALSE);
            curr->depth_write_mask = req->depth_write_mask;
        }
    }

    // 2. Stencil State Sync
    if (curr->stencil_test_enabled != req->stencil_test_enabled) {
        (int)req->stencil_test_enabled ? OpenGL->Enable(GL_STENCIL_TEST)
                                       : OpenGL->Disable(GL_STENCIL_TEST);
        curr->stencil_test_enabled = req->stencil_test_enabled;
    }

    if (req->stencil_test_enabled) {
        if (curr->stencil_func != req->stencil_func || curr->stencil_ref != req->stencil_ref ||
            curr->stencil_read_mask != req->stencil_read_mask) {
            OpenGL->StencilFunc(req->stencil_func, req->stencil_ref, req->stencil_read_mask);
            curr->stencil_func = req->stencil_func;
            curr->stencil_ref = req->stencil_ref;
            curr->stencil_read_mask = req->stencil_read_mask;
        }
        if (curr->stencil_write_mask != req->stencil_write_mask) {
            OpenGL->StencilMask(req->stencil_write_mask);
            curr->stencil_write_mask = req->stencil_write_mask;
        }
        if (curr->stencil_fail_op != req->stencil_fail_op ||
            curr->stencil_zfail_op != req->stencil_zfail_op ||
            curr->stencil_zpass_op != req->stencil_zpass_op) {
            OpenGL->StencilOp(req->stencil_fail_op, req->stencil_zfail_op, req->stencil_zpass_op);
            curr->stencil_fail_op = req->stencil_fail_op;
            curr->stencil_zfail_op = req->stencil_zfail_op;
            curr->stencil_zpass_op = req->stencil_zpass_op;
        }
    }

    // 3. Face Culling Sync
    if (curr->cull_face_enabled != req->cull_face_enabled) {
        (int)req->cull_face_enabled ? OpenGL->Enable(GL_CULL_FACE) : OpenGL->Disable(GL_CULL_FACE);
        curr->cull_face_enabled = req->cull_face_enabled;
    }
    if (req->cull_face_enabled) {
        if (curr->cull_face_mode != req->cull_face_mode) {
            OpenGL->CullFace(req->cull_face_mode);
            curr->cull_face_mode = req->cull_face_mode;
        }
        if (curr->front_face != req->front_face) {
            OpenGL->FrontFace(req->front_face);
            curr->front_face = req->front_face;
        }
    }

    // 4. Blending Sync
    if (curr->blend_enabled != req->blend_enabled) {
        (int)req->blend_enabled ? OpenGL->Enable(GL_BLEND) : OpenGL->Disable(GL_BLEND);
        curr->blend_enabled = req->blend_enabled;
    }
    if (req->blend_enabled) {
        if (curr->blend_src_rgb != req->blend_src_rgb ||
            curr->blend_dst_rgb != req->blend_dst_rgb ||
            curr->blend_src_alpha != req->blend_src_alpha ||
            curr->blend_dst_alpha != req->blend_dst_alpha) {
            OpenGL->BlendFuncSeparate(req->blend_src_rgb, req->blend_dst_rgb, req->blend_src_alpha,
                                      req->blend_dst_alpha);
            curr->blend_src_rgb = req->blend_src_rgb;
            curr->blend_dst_rgb = req->blend_dst_rgb;
            curr->blend_src_alpha = req->blend_src_alpha;
            curr->blend_dst_alpha = req->blend_dst_alpha;
        }
        if (curr->blend_eq_rgb != req->blend_eq_rgb ||
            curr->blend_eq_alpha != req->blend_eq_alpha) {
            OpenGL->BlendEquationSeparate(req->blend_eq_rgb, req->blend_eq_alpha);
            curr->blend_eq_rgb = req->blend_eq_rgb;
            curr->blend_eq_alpha = req->blend_eq_alpha;
        }
    }
}

// -----------------------------------------------------------------------------
// Synchronization
// -----------------------------------------------------------------------------

[[gnu::always_inline]]
static inline void cv_wait_for_last_work(CaravanContext *ctx, const CaravanGLTable *const OpenGL) {
    if (ctx->bound.last_work_fence) {
        // We capture the result to satisfy [[nodiscard]]
        [[maybe_unused]] GLenum status = OpenGL->ClientWaitSync(
            ctx->bound.last_work_fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);

        // In Debug mode, we verify the wait didn't fail (e.g., due to context loss)
#ifndef NDEBUG
        if (status == GL_WAIT_FAILED) {
            // Raw fprintf because we might not hold the GIL here
            // NOLINTNEXTLINE
            (void)fprintf(stderr, "[CaravanGL] Warning: glClientWaitSync failed.\n");
        }
#endif

        OpenGL->DeleteSync(ctx->bound.last_work_fence);
        ctx->bound.last_work_fence = nullptr;
    }
}

[[gnu::always_inline]]
static inline void cv_insert_work_fence(CaravanContext *ctx, const CaravanGLTable *const OpenGL) {
    if (ctx->bound.last_work_fence) {
        OpenGL->DeleteSync(ctx->bound.last_work_fence);
    }
    ctx->bound.last_work_fence = OpenGL->FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}