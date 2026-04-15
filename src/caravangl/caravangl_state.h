#pragma once
#include "caravangl_specs.h"
#include "caravangl_module.h"

// -----------------------------------------------------------------------------
// Core Bindings (VAO, FBO, Program)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_program(CaravanState *s, GLuint program) {
    if (s->ctx.bound.program != program) [[clang::unlikely]] {
        s->ctx.bound.program = program;
        s->gl.UseProgram(program);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_vao(CaravanState *s, GLuint vao) {
    if (s->ctx.bound.vao != vao) [[clang::unlikely]] {
        s->ctx.bound.vao = vao;
        s->gl.BindVertexArray(vao);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_fbo_draw(CaravanState *s, GLuint fbo) {
    if (s->ctx.bound.fbo_draw != fbo) [[clang::unlikely]] {
        s->ctx.bound.fbo_draw = fbo;
        s->gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_viewport(CaravanState *s, const CaravanRect *vp) {
    CaravanRect *c = &s->ctx.viewport;
    if (vp->x != c->x || vp->y != c->y || vp->w != c->w || vp->h != c->h) [[clang::unlikely]] {
        s->gl.Viewport(vp->x, vp->y, vp->w, vp->h);
        *c = *vp;
    }
}

// -----------------------------------------------------------------------------
// Resource Bindings (UBO, SSBO, Textures)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_ubo_range(CaravanState *s, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if (index >= CARAVAN_MAX_UBO_BINDINGS) [[clang::unlikely]] return;

    CaravanBufferBinding *b = &s->ctx.bound.ubo[index];
    if (b->id != buffer || b->offset != offset || b->size != size) [[clang::unlikely]] {
        s->gl.BindBufferRange(GL_UNIFORM_BUFFER, index, buffer, offset, size);
        b->id = buffer;
        b->offset = offset;
        b->size = size;
    }
}

[[gnu::always_inline, gnu::hot]]
static inline void cv_bind_texture(CaravanState *s, GLuint unit, GLuint target, GLuint texture, GLuint sampler) {
    if (unit >= CARAVAN_MAX_TEXTURE_UNITS) [[clang::unlikely]] return;

    CaravanTextureBinding *b = &s->ctx.bound.texture_units[unit];
    
    // Change active unit only if needed
    if (s->ctx.bound.active_texture_unit != unit) {
        s->gl.ActiveTexture(GL_TEXTURE0 + unit);
        s->ctx.bound.active_texture_unit = unit;
    }

    if (b->id != texture || b->target != target) {
        s->gl.BindTexture(target, texture);
        b->id = texture;
        b->target = target;
    }

    if (b->sampler_id != sampler) {
        s->gl.BindSampler(unit, sampler);
        b->sampler_id = sampler;
    }
}

// -----------------------------------------------------------------------------
// Render State Binders (Depth, Blend, Cull)
// -----------------------------------------------------------------------------

[[gnu::always_inline, gnu::hot]]
static inline void cv_set_depth_state(CaravanState *s, bool enabled, GLenum func, bool write_mask) {
    CaravanRenderState *rs = &s->ctx.bound.render;

    if (rs->depth_test_enabled != enabled) {
        enabled ? s->gl.Enable(GL_DEPTH_TEST) : s->gl.Disable(GL_DEPTH_TEST);
        rs->depth_test_enabled = enabled;
    }

    if (enabled) {
        if (rs->depth_func != func) {
            s->gl.DepthFunc(func);
            rs->depth_func = func;
        }
        if (rs->depth_write_mask != write_mask) {
            s->gl.DepthMask(write_mask ? GL_TRUE : GL_FALSE);
            rs->depth_write_mask = write_mask;
        }
    }
}

// -----------------------------------------------------------------------------
// Synchronization
// -----------------------------------------------------------------------------

[[gnu::always_inline]]
static inline void cv_wait_for_last_work(CaravanState *s) {
    if (s->ctx.bound.last_work_fence) {
        auto sync_status = s->gl.ClientWaitSync(s->ctx.bound.last_work_fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        assert(sync_status != GL_WAIT_FAILED);
        s->gl.DeleteSync(s->ctx.bound.last_work_fence);
        s->ctx.bound.last_work_fence = nullptr;
    }
}

[[gnu::always_inline]]
static inline void cv_insert_work_fence(CaravanState *s) {
    // Clean up old fence if it exists
    if (s->ctx.bound.last_work_fence) {
        s->gl.DeleteSync(s->ctx.bound.last_work_fence);
    }
    s->ctx.bound.last_work_fence = s->gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}