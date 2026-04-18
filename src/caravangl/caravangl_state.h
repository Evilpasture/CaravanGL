#pragma once
#include "caravangl_module.h"
#include "caravangl_specs.h"

// -----------------------------------------------------------------------------
// Dirty State Setters (Zero Driver Overhead)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_program(CaravanContext *ctx, GLuint program) {
    if (ctx->bound.program != program) [[clang::unlikely]] {
        ctx->bound.program = program;
        ctx->dirty_flags |= CV_DIRTY_PROGRAM;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_vao(CaravanContext *ctx, GLuint vao) {
    if (ctx->bound.vao != vao) [[clang::unlikely]] {
        ctx->bound.vao = vao;
        ctx->dirty_flags |= CV_DIRTY_VAO;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_fbo_draw(CaravanContext *ctx, GLuint fbo) {
    if (ctx->bound.fbo_draw != fbo) [[clang::unlikely]] {
        ctx->bound.fbo_draw = fbo;
        ctx->dirty_flags |= CV_DIRTY_FBO_DRAW;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_fbo_read(CaravanContext *ctx, GLuint fbo) {
    if (ctx->bound.fbo_read != fbo) [[clang::unlikely]] {
        ctx->bound.fbo_read = fbo;
        ctx->dirty_flags |= CV_DIRTY_FBO_READ;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_fbo_combined(CaravanContext *ctx, GLuint fbo) {
    if (ctx->bound.fbo_draw != fbo || ctx->bound.fbo_read != fbo) [[clang::unlikely]] {
        ctx->bound.fbo_draw = fbo;
        ctx->bound.fbo_read = fbo;
        ctx->dirty_flags |= (CV_DIRTY_FBO_DRAW | CV_DIRTY_FBO_READ);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_viewport(CaravanContext *ctx, const CaravanRect *viewport) {
    CaravanRect *cview = &ctx->viewport;
    if (viewport->x != cview->x || viewport->y != cview->y || viewport->width != cview->width ||
        viewport->height != cview->height) [[clang::unlikely]] {
        *cview = *viewport;
        ctx->dirty_flags |= CV_DIRTY_VIEWPORT;
    }
}

// -----------------------------------------------------------------------------
// The Resolver (The single branching hot-path)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_resolve(CaravanContext *ctx, const CaravanGLTable *const OpenGL) {
    uint32_t flags = ctx->dirty_flags;
    if (!flags) [[clang::likely]] {
        return;
    }

    if (flags & CV_DIRTY_PROGRAM) {
        OpenGL->UseProgram(ctx->bound.program);
    }

    if (flags & CV_DIRTY_VAO) {
        OpenGL->BindVertexArray(ctx->bound.vao);
    }

    if ((flags & CV_DIRTY_FBO_DRAW) && (flags & CV_DIRTY_FBO_READ) &&
        (ctx->bound.fbo_draw == ctx->bound.fbo_read)) {
        OpenGL->BindFramebuffer(GL_FRAMEBUFFER, ctx->bound.fbo_draw);
    } else {
        if (flags & CV_DIRTY_FBO_DRAW) {
            OpenGL->BindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->bound.fbo_draw);
        }
        if (flags & CV_DIRTY_FBO_READ) {
            OpenGL->BindFramebuffer(GL_READ_FRAMEBUFFER, ctx->bound.fbo_read);
        }
    }

    if (flags & CV_DIRTY_VIEWPORT) {
        OpenGL->Viewport(ctx->viewport.x, ctx->viewport.y, ctx->viewport.width,
                         ctx->viewport.height);
    }

    ctx->dirty_flags = 0;
}

// -----------------------------------------------------------------------------
// Resource Bindings (Kept immediate because they require indexing)
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
// Render State Binders (Differential Checks Maintained)
// -----------------------------------------------------------------------------
// --- HELPER SUB-FUNCTIONS ---

static inline void cv_sync_depth(const CaravanGLTable *const OpenGL, CaravanRenderState *curr,
                                 const CaravanRenderState *req) {
    if (curr->depth_test_enabled != req->depth_test_enabled) {
        (int)req->depth_test_enabled ? OpenGL->Enable(GL_DEPTH_TEST)
                                     : OpenGL->Disable(GL_DEPTH_TEST);
        curr->depth_test_enabled = req->depth_test_enabled;
    }
    if ((int)req->depth_test_enabled &&
        (curr->depth_func != req->depth_func || curr->depth_write_mask != req->depth_write_mask)) {
        OpenGL->DepthFunc(req->depth_func);
        OpenGL->DepthMask((int)req->depth_write_mask ? GL_TRUE : GL_FALSE);
        curr->depth_func = req->depth_func;
        curr->depth_write_mask = req->depth_write_mask;
    }
}

static inline void cv_sync_stencil(const CaravanGLTable *const OpenGL, CaravanRenderState *curr,
                                   const CaravanRenderState *req) {
    if (curr->stencil_test_enabled != req->stencil_test_enabled) {
        (int)req->stencil_test_enabled ? OpenGL->Enable(GL_STENCIL_TEST)
                                       : OpenGL->Disable(GL_STENCIL_TEST);
        curr->stencil_test_enabled = req->stencil_test_enabled;
    }
    if (req->stencil_test_enabled) {
        // Compare all stencil params as a block (Func, Ref, ReadMask)
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
        // Compare all Ops as a block
        if (curr->stencil_fail_op != req->stencil_fail_op ||
            curr->stencil_zfail_op != req->stencil_zfail_op ||
            curr->stencil_zpass_op != req->stencil_zpass_op) {
            OpenGL->StencilOp(req->stencil_fail_op, req->stencil_zfail_op, req->stencil_zpass_op);
            curr->stencil_fail_op = req->stencil_fail_op;
            curr->stencil_zfail_op = req->stencil_zfail_op;
            curr->stencil_zpass_op = req->stencil_zpass_op;
        }
    }
}

static inline void cv_sync_cull(const CaravanGLTable *const OpenGL, CaravanRenderState *curr,
                                const CaravanRenderState *req) {
    if (curr->cull_face_enabled != req->cull_face_enabled) {
        (int)req->cull_face_enabled ? OpenGL->Enable(GL_CULL_FACE) : OpenGL->Disable(GL_CULL_FACE);
        curr->cull_face_enabled = req->cull_face_enabled;
    }
    if ((int)req->cull_face_enabled &&
        (curr->cull_face_mode != req->cull_face_mode || curr->front_face != req->front_face)) {
        OpenGL->CullFace(req->cull_face_mode);
        OpenGL->FrontFace(req->front_face);
        curr->cull_face_mode = req->cull_face_mode;
        curr->front_face = req->front_face;
    }
}

static inline void cv_sync_blend(const CaravanGLTable *const OpenGL, CaravanRenderState *curr,
                                 const CaravanRenderState *req) {
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

// --- THE MAIN DISPATCHER ---

[[gnu::always_inline, gnu::hot]]
static inline bool cv_render_state_equals(const CaravanRenderState *FirstRenderState,
                                          const CaravanRenderState *SecondRenderState) {
#pragma unroll
    for (int i = 0; i < 32; i++) {
        if (FirstRenderState->data[i] != SecondRenderState->data[i]) {
            return false;
        }
    }
    return true;
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_sync_render_state(CaravanContext *ctx, const CaravanGLTable *const OpenGL,
                                        const CaravanRenderState *req) {
    CaravanRenderState *curr = &ctx->bound.render;

    // 1. FAST PATH: Integer block comparison (No memcmp, no linter warning)
    if (cv_render_state_equals(curr, req)) [[clang::likely]] {
        return;
    }

    // 2. SLOW PATH: Differential Update
    cv_sync_depth(OpenGL, curr, req);
    cv_sync_stencil(OpenGL, curr, req);
    cv_sync_cull(OpenGL, curr, req);
    cv_sync_blend(OpenGL, curr, req);
}