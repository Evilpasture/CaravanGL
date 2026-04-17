#pragma once
#include "caravangl_module.h"

// -----------------------------------------------------------------------------
// Uniform Binding Helpers & Dispatch Table
// -----------------------------------------------------------------------------

/**
 * Signature for generic uniform upload functions.
 * Takes the isolated CaravanState to access the function table.
 */
typedef void (*UniformUploadFn)(CaravanState *state, GLint location, GLsizei count,
                                const void *ptr);

// -- Wrappers (Using state->gl.Member syntax) --

// Integers
static inline void u_1iv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform1iv(location, count, (const GLint *)ptr);
}
static inline void u_2iv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform2iv(location, count, (const GLint *)ptr);
}
static inline void u_3iv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform3iv(location, count, (const GLint *)ptr);
}
static inline void u_4iv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform4iv(location, count, (const GLint *)ptr);
}

// Unsigned Integers
static inline void u_1uiv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform1uiv(location, count, (const GLuint *)ptr);
}
static inline void u_2uiv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform2uiv(location, count, (const GLuint *)ptr);
}
static inline void u_3uiv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform3uiv(location, count, (const GLuint *)ptr);
}
static inline void u_4uiv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform4uiv(location, count, (const GLuint *)ptr);
}

// Floats
static inline void u_1fv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform1fv(location, count, (const GLfloat *)ptr);
}
static inline void u_2fv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform2fv(location, count, (const GLfloat *)ptr);
}
static inline void u_3fv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform3fv(location, count, (const GLfloat *)ptr);
}
static inline void u_4fv(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.Uniform4fv(location, count, (const GLfloat *)ptr);
}

// Matrices (Always Transpose = GL_FALSE per GLSL spec)
static inline void u_mat2(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix2fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat2x3(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix2x3fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat2x4(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix2x4fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat3x2(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix3x2fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat3(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix3fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat3x4(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix3x4fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat4x2(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix4x2fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat4x3(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix4x3fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat4(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix4fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}

// -- Row-Major Wrappers (Using GL_TRUE) --
static inline void u_mat2_rm(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix2fv(location, count, GL_TRUE, (const GLfloat *)ptr);
}
static inline void u_mat3_rm(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix3fv(location, count, GL_TRUE, (const GLfloat *)ptr);
}
static inline void u_mat4_rm(CaravanState *state, GLint location, GLsizei count, const void *ptr) {
    state->gl.UniformMatrix4fv(location, count, GL_TRUE, (const GLfloat *)ptr);
}

// -- Dispatch Table --

[[maybe_unused]] static const UniformUploadFn uniform_upload_table[UF_COUNT] = {
    // Ints & Bools (Bools are uploaded as Ints)
    [UF_1I] = u_1iv,
    [UF_2I] = u_2iv,
    [UF_3I] = u_3iv,
    [UF_4I] = u_4iv,
    [UF_1B] = u_1iv,
    [UF_2B] = u_2iv,
    [UF_3B] = u_3iv,
    [UF_4B] = u_4iv,

    // Unsigned
    [UF_1U] = u_1uiv,
    [UF_2U] = u_2uiv,
    [UF_3U] = u_3uiv,
    [UF_4U] = u_4uiv,

    // Floats
    [UF_1F] = u_1fv,
    [UF_2F] = u_2fv,
    [UF_3F] = u_3fv,
    [UF_4F] = u_4fv,

    // Matrices
    [UF_MAT2] = u_mat2,
    [UF_MAT2x3] = u_mat2x3,
    [UF_MAT2x4] = u_mat2x4,
    [UF_MAT3x2] = u_mat3x2,
    [UF_MAT3] = u_mat3,
    [UF_MAT3x4] = u_mat3x4,
    [UF_MAT4x2] = u_mat4x2,
    [UF_MAT4x3] = u_mat4x3,
    [UF_MAT4] = u_mat4,

    // Row-Major stuff
    [UF_MAT2_RM] = u_mat2_rm,
    [UF_MAT3_RM] = u_mat3_rm,
    [UF_MAT4_RM] = u_mat4_rm,
};