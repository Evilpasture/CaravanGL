#pragma once
#include "caravangl_module.h"

// -----------------------------------------------------------------------------
// Uniform Binding Helpers & Dispatch Table
// -----------------------------------------------------------------------------

typedef void (*UniformUploadFn)(CaravanState* state, GLint location, GLsizei count, const void *ptr);

// -- Wrappers --
static inline void u_1iv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform1iv(l, c, (const GLint *)p); }
static inline void u_2iv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform2iv(l, c, (const GLint *)p); }
static inline void u_3iv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform3iv(l, c, (const GLint *)p); }
static inline void u_4iv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform4iv(l, c, (const GLint *)p); }

static inline void u_1uiv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform1uiv(l, c, (const GLuint *)p); }
static inline void u_2uiv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform2uiv(l, c, (const GLuint *)p); }
static inline void u_3uiv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform3uiv(l, c, (const GLuint *)p); }
static inline void u_4uiv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform4uiv(l, c, (const GLuint *)p); }

static inline void u_1fv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform1fv(l, c, (const GLfloat *)p); }
static inline void u_2fv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform2fv(l, c, (const GLfloat *)p); }
static inline void u_3fv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform3fv(l, c, (const GLfloat *)p); }
static inline void u_4fv(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.Uniform4fv(l, c, (const GLfloat *)p); }

static inline void u_mat2(CaravanState* s, GLint l, GLsizei c, const void *p)   { s->gl.UniformMatrix2fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat2x3(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.UniformMatrix2x3fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat2x4(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.UniformMatrix2x4fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat3x2(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.UniformMatrix3x2fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat3(CaravanState* s, GLint l, GLsizei c, const void *p)   { s->gl.UniformMatrix3fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat3x4(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.UniformMatrix3x4fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat4x2(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.UniformMatrix4x2fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat4x3(CaravanState* s, GLint l, GLsizei c, const void *p) { s->gl.UniformMatrix4x3fv(l, c, GL_FALSE, (const GLfloat *)p); }
static inline void u_mat4(CaravanState* s, GLint l, GLsizei c, const void *p)   { s->gl.UniformMatrix4fv(l, c, GL_FALSE, (const GLfloat *)p); }

// -- Dispatch Table --
[[maybe_unused]] static const UniformUploadFn uniform_upload_table[UF_COUNT] = {
    [UF_1I] = u_1iv,  [UF_2I] = u_2iv,  [UF_3I] = u_3iv,  [UF_4I] = u_4iv,
    [UF_1B] = u_1iv,  [UF_2B] = u_2iv,  [UF_3B] = u_3iv,  [UF_4B] = u_4iv,

    [UF_1U] = u_1uiv, [UF_2U] = u_2uiv, [UF_3U] = u_3uiv, [UF_4U] = u_4uiv,

    [UF_1F] = u_1fv,  [UF_2F] = u_2fv,  [UF_3F] = u_3fv,  [UF_4F] = u_4fv,

    [UF_MAT2]   = u_mat2,   [UF_MAT2x3] = u_mat2x3, [UF_MAT2x4] = u_mat2x4,
    [UF_MAT3x2] = u_mat3x2, [UF_MAT3]   = u_mat3,   [UF_MAT3x4] = u_mat3x4,
    [UF_MAT4x2] = u_mat4x2, [UF_MAT4x3] = u_mat4x3, [UF_MAT4]   = u_mat4,
};