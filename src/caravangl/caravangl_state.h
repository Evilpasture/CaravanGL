#pragma once
#include "caravangl_module.h"
#include "caravangl_specs.h"

// -----------------------------------------------------------------------------
// Core Bindings (VAO, FBO, Program)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_program(CaravanState *state, GLuint program) {
    if (state->ctx.bound.program != program) [[clang::unlikely]] {
        state->ctx.bound.program = program;
        state->gl.UseProgram(program);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_vao(CaravanState *state, GLuint vao) {
    if (state->ctx.bound.vao != vao) [[clang::unlikely]] {
        state->ctx.bound.vao = vao;
        state->gl.BindVertexArray(vao);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_draw(CaravanState *state, GLuint fbo) {
    if (state->ctx.bound.fbo_draw != fbo) [[clang::unlikely]] {
        state->gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        state->ctx.bound.fbo_draw = fbo;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_read(CaravanState *state, GLuint fbo) {
    if (state->ctx.bound.fbo_read != fbo) [[clang::unlikely]] {
        state->gl.BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        state->ctx.bound.fbo_read = fbo;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_combined(CaravanState *state, GLuint fbo) {
    if (state->ctx.bound.fbo_draw != fbo || state->ctx.bound.fbo_read != fbo) [[clang::unlikely]] {
        state->gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        state->ctx.bound.fbo_draw = fbo;
        state->ctx.bound.fbo_read = fbo;
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline void cv_bind_fbo(CaravanState *state, GLFramebufferTarget target, GLuint fbo) {
    // Generalist function. Use specialized functions to avoid branching!
    switch (target) {
    case GL_FRAMEBUFFER:
        cv_bind_fbo_combined(state, fbo);
        break;
    case GL_DRAW_FRAMEBUFFER:
        cv_bind_fbo_draw(state, fbo);
        break;
    case GL_READ_FRAMEBUFFER:
        cv_bind_fbo_read(state, fbo);
        break;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_viewport(CaravanState *state, const CaravanRect *viewport) {
    CaravanRect *cview = &state->ctx.viewport;
    if (viewport->x != cview->x || viewport->y != cview->y || viewport->w != cview->w ||
        viewport->h != cview->h) [[clang::unlikely]] {
        state->gl.Viewport(viewport->x, viewport->y, viewport->w, viewport->h);
        *cview = *viewport;
    }
}

// -----------------------------------------------------------------------------
// Resource Bindings (UBO, SSBO, Textures)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_ubo_range(CaravanState *state, GLuint index, GLuint buffer,
                                     GLintptr offset, GLsizeiptr size) {
    if (index >= CARAVAN_MAX_UBO_BINDINGS) {
        [[clang::unlikely]] return;
    }

    CaravanBufferBinding *binding = &state->ctx.bound.ubo[index];
    if (binding->id != buffer || binding->offset != offset || binding->size != size)
        [[clang::unlikely]] {
        state->gl.BindBufferRange(GL_UNIFORM_BUFFER, index, buffer, offset, size);
        binding->id = buffer;
        binding->offset = offset;
        binding->size = size;
    }
}

[[gnu::always_inline, gnu::hot]] [[gnu::always_inline, gnu::hot]]
static inline void cv_bind_texture(CaravanState *state, GLuint unit, const CaravanTexture *tex,
                                   GLuint sampler) {
    if (unit >= CARAVAN_MAX_TEXTURE_UNITS) {
        [[clang::unlikely]] return;
    }

    CaravanTextureBinding *binding = &state->ctx.bound.texture_units[unit];

    GLuint texture_id = tex->id;
    GLenum target = tex->target;

    // 1. Texture Binding Logic
    if (binding->id != texture_id || binding->target != target) {
#ifndef __APPLE__
        // Prefer DSA (OpenGL 4.5+)
        if (state->gl.BindTextureUnit != nullptr) {
            state->gl.BindTextureUnit(unit, texture_id);
        } else
#endif
        {
            // Fallback (OpenGL 3.3 / macOS)
            if (state->ctx.bound.active_texture_unit != unit) {
                state->gl.ActiveTexture(GL_TEXTURE0 + unit);
                state->ctx.bound.active_texture_unit = unit;
            }
            state->gl.BindTexture(target, texture_id);
        }
        binding->id = texture_id;
        binding->target = target;
    }

    // 2. Sampler Binding Logic
    if (binding->sampler_id != sampler) {
        state->gl.BindSampler(unit, sampler);
        binding->sampler_id = sampler;
    }
}

// -----------------------------------------------------------------------------
// Render State Binders (Depth, Blend, Cull)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]] [[gnu::always_inline, gnu::hot]]
static inline void cv_sync_render_state(CaravanState *state, const CaravanRenderState *req) {
    CaravanRenderState *curr = &state->ctx.bound.render;

    // 1. Depth State Sync
    if (curr->depth_test_enabled != req->depth_test_enabled) {
        (int)req->depth_test_enabled ? state->gl.Enable(GL_DEPTH_TEST)
                                     : state->gl.Disable(GL_DEPTH_TEST);
        curr->depth_test_enabled = req->depth_test_enabled;
    }

    if (req->depth_test_enabled) {
        if (curr->depth_func != req->depth_func) {
            state->gl.DepthFunc(req->depth_func);
            curr->depth_func = req->depth_func;
        }
        if (curr->depth_write_mask != req->depth_write_mask) {
            state->gl.DepthMask((GLboolean)req->depth_write_mask ? GL_TRUE : GL_FALSE);
            curr->depth_write_mask = req->depth_write_mask;
        }
    }

    // 2. Stencil State Sync
    if (curr->stencil_test_enabled != req->stencil_test_enabled) {
        (int)req->stencil_test_enabled ? state->gl.Enable(GL_STENCIL_TEST)
                                       : state->gl.Disable(GL_STENCIL_TEST);
        curr->stencil_test_enabled = req->stencil_test_enabled;
    }

    if (req->stencil_test_enabled) {
        // Sync StencilFunc (func, ref, read_mask)
        if (curr->stencil_func != req->stencil_func || curr->stencil_ref != req->stencil_ref ||
            curr->stencil_read_mask != req->stencil_read_mask) {
            state->gl.StencilFunc(req->stencil_func, req->stencil_ref, req->stencil_read_mask);
            curr->stencil_func = req->stencil_func;
            curr->stencil_ref = req->stencil_ref;
            curr->stencil_read_mask = req->stencil_read_mask;
        }
        // Sync StencilMask
        if (curr->stencil_write_mask != req->stencil_write_mask) {
            state->gl.StencilMask(req->stencil_write_mask);
            curr->stencil_write_mask = req->stencil_write_mask;
        }
        // Sync StencilOp (fail, zfail, zpass)
        if (curr->stencil_fail_op != req->stencil_fail_op ||
            curr->stencil_zfail_op != req->stencil_zfail_op ||
            curr->stencil_zpass_op != req->stencil_zpass_op) {
            state->gl.StencilOp(req->stencil_fail_op, req->stencil_zfail_op, req->stencil_zpass_op);
            curr->stencil_fail_op = req->stencil_fail_op;
            curr->stencil_zfail_op = req->stencil_zfail_op;
            curr->stencil_zpass_op = req->stencil_zpass_op;
        }
    }
    // 3. Face Culling Sync
    if (curr->cull_face_enabled != req->cull_face_enabled) {
        (int)req->cull_face_enabled ? state->gl.Enable(GL_CULL_FACE)
                                    : state->gl.Disable(GL_CULL_FACE);
        curr->cull_face_enabled = req->cull_face_enabled;
    }
    if (req->cull_face_enabled && curr->cull_face_mode != req->cull_face_mode) {
        state->gl.CullFace(req->cull_face_mode);
        curr->cull_face_mode = req->cull_face_mode;
    }

    // 4. Blending Sync
    if (curr->blend_enabled != req->blend_enabled) {
        (int)req->blend_enabled ? state->gl.Enable(GL_BLEND) : state->gl.Disable(GL_BLEND);
        curr->blend_enabled = req->blend_enabled;
    }
    if (req->blend_enabled) {
        if (curr->blend_src_rgb != req->blend_src_rgb ||
            curr->blend_dst_rgb != req->blend_dst_rgb ||
            curr->blend_src_alpha != req->blend_src_alpha ||
            curr->blend_dst_alpha != req->blend_dst_alpha) {
            state->gl.BlendFuncSeparate(req->blend_src_rgb, req->blend_dst_rgb,
                                        req->blend_src_alpha, req->blend_dst_alpha);
            curr->blend_src_rgb = req->blend_src_rgb;
            curr->blend_dst_rgb = req->blend_dst_rgb;
            curr->blend_src_alpha = req->blend_src_alpha;
            curr->blend_dst_alpha = req->blend_dst_alpha;
        }
        if (curr->blend_eq_rgb != req->blend_eq_rgb ||
            curr->blend_eq_alpha != req->blend_eq_alpha) {
            state->gl.BlendEquationSeparate(req->blend_eq_rgb, req->blend_eq_alpha);
            curr->blend_eq_rgb = req->blend_eq_rgb;
            curr->blend_eq_alpha = req->blend_eq_alpha;
        }
    }
}

// -----------------------------------------------------------------------------
// Synchronization
// -----------------------------------------------------------------------------

[[gnu::always_inline]]
static inline void cv_wait_for_last_work(CaravanState *state) {
    if (state->ctx.bound.last_work_fence) {
        auto sync_status = state->gl.ClientWaitSync(state->ctx.bound.last_work_fence,
                                                    GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        assert(sync_status != GL_WAIT_FAILED);
        state->gl.DeleteSync(state->ctx.bound.last_work_fence);
        state->ctx.bound.last_work_fence = nullptr;
    }
}

[[gnu::always_inline]]
static inline void cv_insert_work_fence(CaravanState *state) {
    // Clean up old fence if it exists
    if (state->ctx.bound.last_work_fence) {
        state->gl.DeleteSync(state->ctx.bound.last_work_fence);
    }
    state->ctx.bound.last_work_fence = state->gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}