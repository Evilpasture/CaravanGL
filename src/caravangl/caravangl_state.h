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
        state->ctx.bound.fbo_draw = fbo;
        state->gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
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

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_texture(CaravanState *state, GLuint unit, GLuint target, GLuint texture,
                                   GLuint sampler) {
    if (unit >= CARAVAN_MAX_TEXTURE_UNITS) {
        [[clang::unlikely]] return;
    }

    CaravanTextureBinding *binding = &state->ctx.bound.texture_units[unit];

// Change active unit only if needed
#ifndef __APPLE__
    // Use the 4.5+ super-path if the function exists in our table
    if (state->gl.BindTextureUnit != nullptr) {
        state->gl.BindTextureUnit(unit, texture);
    } else
#endif
    {
        // Fallback for 3.3 / macOS / Older Hardware
        if (state->ctx.bound.active_texture_unit != unit) {
            state->gl.ActiveTexture(GL_TEXTURE0 + unit);
            state->ctx.bound.active_texture_unit = unit;
        }
        state->gl.BindTexture(target, texture);
    }

    if (binding->id != texture || binding->target != target) {
        state->gl.BindTexture(target, texture);
        binding->id = texture;
        binding->target = target;
    }

    if (binding->sampler_id != sampler) {
        state->gl.BindSampler(unit, sampler);
        binding->sampler_id = sampler;
    }
}

// -----------------------------------------------------------------------------
// Render State Binders (Depth, Blend, Cull)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_depth_state(CaravanState *state, bool enabled, GLenum func,
                                      bool write_mask) {
    CaravanRenderState *rst = &state->ctx.bound.render;

    if (rst->depth_test_enabled != enabled) {
        (int)enabled ? state->gl.Enable(GL_DEPTH_TEST) : state->gl.Disable(GL_DEPTH_TEST);
        rst->depth_test_enabled = enabled;
    }

    if (enabled) {
        if (rst->depth_func != func) {
            state->gl.DepthFunc(func);
            rst->depth_func = func;
        }
        if (rst->depth_write_mask != write_mask) {
            state->gl.DepthMask((GLboolean)write_mask ? GL_TRUE : GL_FALSE);
            rst->depth_write_mask = write_mask;
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