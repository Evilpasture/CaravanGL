#pragma once
#include "caravangl_module.h"

// -----------------------------------------------------------------------------
// Uniform Binding Helpers & Dispatch Table
// -----------------------------------------------------------------------------

/**
 * Signature for generic uniform upload functions.
 * Takes the isolated CaravanState to access the function table.
 */
typedef void (*UniformUploadFn)(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                                const void *ptr);

// -- Wrappers (Using OpenGL->Member syntax) --

// Integers
static inline void u_1iv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform1iv(location, count, (const GLint *)ptr);
}
static inline void u_2iv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform2iv(location, count, (const GLint *)ptr);
}
static inline void u_3iv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform3iv(location, count, (const GLint *)ptr);
}
static inline void u_4iv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform4iv(location, count, (const GLint *)ptr);
}

// Unsigned Integers
static inline void u_1uiv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform1uiv(location, count, (const GLuint *)ptr);
}
static inline void u_2uiv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform2uiv(location, count, (const GLuint *)ptr);
}
static inline void u_3uiv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform3uiv(location, count, (const GLuint *)ptr);
}
static inline void u_4uiv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform4uiv(location, count, (const GLuint *)ptr);
}

// Floats
static inline void u_1fv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform1fv(location, count, (const GLfloat *)ptr);
}
static inline void u_2fv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform2fv(location, count, (const GLfloat *)ptr);
}
static inline void u_3fv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform3fv(location, count, (const GLfloat *)ptr);
}
static inline void u_4fv(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->Uniform4fv(location, count, (const GLfloat *)ptr);
}

// Matrices (Always Transpose = GL_FALSE per GLSL spec)
static inline void u_mat2(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->UniformMatrix2fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat2x3(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                            const void *ptr) {
    OpenGL->UniformMatrix2x3fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat2x4(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                            const void *ptr) {
    OpenGL->UniformMatrix2x4fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat3x2(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                            const void *ptr) {
    OpenGL->UniformMatrix3x2fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat3(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->UniformMatrix3fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat3x4(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                            const void *ptr) {
    OpenGL->UniformMatrix3x4fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat4x2(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                            const void *ptr) {
    OpenGL->UniformMatrix4x2fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat4x3(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                            const void *ptr) {
    OpenGL->UniformMatrix4x3fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}
static inline void u_mat4(const CaravanGLTable *const OpenGL, GLint location, GLsizei count, const void *ptr) {
    OpenGL->UniformMatrix4fv(location, count, GL_FALSE, (const GLfloat *)ptr);
}

// -- Row-Major Wrappers (Using GL_TRUE) --
static inline void u_mat2_rm(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                             const void *ptr) {
    OpenGL->UniformMatrix2fv(location, count, GL_TRUE, (const GLfloat *)ptr);
}
static inline void u_mat3_rm(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                             const void *ptr) {
    OpenGL->UniformMatrix3fv(location, count, GL_TRUE, (const GLfloat *)ptr);
}
static inline void u_mat4_rm(const CaravanGLTable *const OpenGL, GLint location, GLsizei count,
                             const void *ptr) {
    OpenGL->UniformMatrix4fv(location, count, GL_TRUE, (const GLfloat *)ptr);
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