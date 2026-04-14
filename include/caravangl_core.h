#pragma once
#include <stddef.h>
#include <stdint.h>

#define CARAVANGL_CONSTANT [[maybe_unused]] static constexpr

// -----------------------------------------------------------------------------
// Standard GL Types
// -----------------------------------------------------------------------------
typedef uint8_t   GLboolean;
typedef uint32_t  GLbitfield;
typedef uint32_t  GLenum;
typedef uint32_t  GLuint;
typedef int32_t   GLint;
typedef int32_t   GLsizei;
typedef void      GLvoid;
typedef char      GLchar;
typedef float     GLfloat;
typedef float     GLclampf;
typedef double    GLdouble;
typedef double    GLclampd;
typedef int8_t    GLbyte;
typedef uint8_t   GLubyte;
typedef int16_t   GLshort;
typedef uint16_t  GLushort;
typedef int64_t   GLint64;
typedef uint64_t  GLuint64;
typedef intptr_t  GLintptr;
typedef ptrdiff_t GLsizeiptr;

typedef struct __GLsync *GLsync;

// -----------------------------------------------------------------------------
// OpenGL Constants (Typed for Safety)
// -----------------------------------------------------------------------------
CARAVANGL_CONSTANT GLboolean GL_FALSE = 0;
CARAVANGL_CONSTANT GLboolean GL_TRUE  = 1;
CARAVANGL_CONSTANT GLenum    GL_NO_ERROR = 0;

CARAVANGL_CONSTANT GLbitfield GL_DEPTH_BUFFER_BIT   = 0x0100;
CARAVANGL_CONSTANT GLbitfield GL_STENCIL_BUFFER_BIT = 0x0400;
CARAVANGL_CONSTANT GLbitfield GL_COLOR_BUFFER_BIT   = 0x4000;

CARAVANGL_CONSTANT GLenum GL_FRONT          = 0x0404;
CARAVANGL_CONSTANT GLenum GL_BACK           = 0x0405;
CARAVANGL_CONSTANT GLenum GL_CULL_FACE      = 0x0B44;
CARAVANGL_CONSTANT GLenum GL_DEPTH_TEST     = 0x0B71;
CARAVANGL_CONSTANT GLenum GL_STENCIL_TEST   = 0x0B90;
CARAVANGL_CONSTANT GLenum GL_BLEND          = 0x0BE2;
CARAVANGL_CONSTANT GLenum GL_TEXTURE_2D     = 0x0DE1;
CARAVANGL_CONSTANT GLenum GL_UNSIGNED_SHORT = 0x1403;
CARAVANGL_CONSTANT GLenum GL_UNSIGNED_INT   = 0x1405;

CARAVANGL_CONSTANT GLenum GL_TEXTURE0       = 0x84C0;
CARAVANGL_CONSTANT GLenum GL_ARRAY_BUFFER   = 0x8892;
CARAVANGL_CONSTANT GLenum GL_STATIC_DRAW    = 0x88E4;
CARAVANGL_CONSTANT GLenum GL_DYNAMIC_DRAW   = 0x88E8;

CARAVANGL_CONSTANT GLuint GL_INVALID_INDEX  = 0xFFFFFFFFu;
CARAVANGL_CONSTANT GLuint64 GL_TIMEOUT_IGNORED = 0xFFFFFFFFFFFFFFFFull;

CARAVANGL_CONSTANT GLbitfield GL_MAP_READ_BIT       = 0x0001;
CARAVANGL_CONSTANT GLbitfield GL_MAP_WRITE_BIT      = 0x0002;
CARAVANGL_CONSTANT GLbitfield GL_MAP_PERSISTENT_BIT = 0x0040;
CARAVANGL_CONSTANT GLbitfield GL_MAP_COHERENT_BIT   = 0x0080;
CARAVANGL_CONSTANT GLbitfield GL_PERSISTENT_WRITE_FLAGS = (GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

// --- Implementation Limits ---
CARAVANGL_CONSTANT GLint GL_MIN_UNIFORM_BUFFER_BINDINGS = 8;
CARAVANGL_CONSTANT GLint GL_MAX_TEXTURE_IMAGE_UNITS    = 16;
CARAVANGL_CONSTANT GLint GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = 48;

// -----------------------------------------------------------------------------
// Modern C23 Enums
// -----------------------------------------------------------------------------
typedef enum UniformFunction : uint8_t {
    UF_1I, UF_2I, UF_3I, UF_4I,
    UF_1B, UF_2B, UF_3B, UF_4B,
    UF_1U, UF_2U, UF_3U, UF_4U,
    UF_1F, UF_2F, UF_3F, UF_4F,
    UF_MAT2, UF_MAT2x3, UF_MAT2x4,
    UF_MAT3x2, UF_MAT3, UF_MAT3x4,
    UF_MAT4x2, UF_MAT4x3, UF_MAT4,
    UF_COUNT
} UniformFunction;

typedef enum ImageFormatTupleIndex : uint8_t {
    IF_INTERNAL_FORMAT, IF_FORMAT, IF_TYPE, IF_BUFFER,
    IF_COMPONENTS, IF_PIXEL_SIZE, IF_COLOR, IF_FLAGS,
    IF_CLEAR_TYPE, IF_TUPLE_SIZE
} ImageFormatTupleIndex;

// -----------------------------------------------------------------------------
// OpenGL Single Source of Truth Lists (X-Macros)
// Format: X(Return Type, Function Name, Arguments...)
// -----------------------------------------------------------------------------

// ======================= OPENGL 3.3 CORE =======================
// Includes 1.x, 2.x, 3.0, 3.1, 3.2 and 3.3 functions that form the modern baseline
#define GL_FUNCTIONS_3_3_CORE(X) \
  X(void, glCullFace, GLenum mode) \
  X(void, glClear, GLbitfield mask) \
  X(void, glTexParameteri, GLenum target, GLenum pname, GLint param) \
  X(void, glTexImage2D, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) \
  X(void, glDepthMask, GLboolean flag) \
  X(void, glDisable, GLenum cap) \
  X(void, glEnable, GLenum cap) \
  X(void, glFlush, void) \
  X(void, glDepthFunc, GLenum func) \
  X(void, glReadBuffer, GLenum src) \
  X(void, glReadPixels, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) \
  X(GLenum, glGetError, void) \
  X(void, glGetIntegerv, GLenum pname, GLint *data) \
  X(const GLchar *, glGetString, GLenum name) \
  X(void, glViewport, GLint x, GLint y, GLsizei width, GLsizei height) \
  X(void, glTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) \
  X(void, glBindTexture, GLenum target, GLuint texture) \
  X(void, glDeleteTextures, GLsizei n, const GLuint *textures) \
  X(void, glGenTextures, GLsizei n, GLuint *textures) \
  X(void, glTexImage3D, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) \
  X(void, glTexSubImage3D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) \
  X(void, glActiveTexture, GLenum texture) \
  X(void, glBlendFuncSeparate, GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) \
  X(void, glBindBuffer, GLenum target, GLuint buffer) \
  X(void, glDeleteBuffers, GLsizei n, const GLuint *buffers) \
  X(void, glGenBuffers, GLsizei n, GLuint *buffers) \
  X(void, glBufferData, GLenum target, GLsizeiptr size, const void *data, GLenum usage) \
  X(void, glBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const void *data) \
  X(void, glGetBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, void *data) \
  X(void, glBlendEquationSeparate, GLenum modeRGB, GLenum modeAlpha) \
  X(void, glDrawBuffers, GLsizei n, const GLenum *bufs) \
  X(void, glStencilOpSeparate, GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) \
  X(void, glStencilFuncSeparate, GLenum face, GLenum func, GLint ref, GLuint mask) \
  X(void, glStencilMaskSeparate, GLenum face, GLuint mask) \
  X(void, glAttachShader, GLuint program, GLuint shader) \
  X(void, glDetachShader, GLuint program, GLuint shader) \
  X(void, glCompileShader, GLuint shader) \
  X(GLuint, glCreateProgram, void) \
  X(GLuint, glCreateShader, GLenum type) \
  X(void, glDeleteProgram, GLuint program) \
  X(void, glDeleteShader, GLuint shader) \
  X(void, glDeleteQueries, GLsizei n, const GLuint *ids) \
  X(void, glEnableVertexAttribArray, GLuint index) \
  X(void, glGetActiveAttrib, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) \
  X(void, glGetActiveUniform, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) \
  X(GLint, glGetAttribLocation, GLuint program, const GLchar *name) \
  X(void, glGetProgramiv, GLuint program, GLenum pname, GLint *params) \
  X(void, glGetProgramInfoLog, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) \
  X(void, glGetShaderiv, GLuint shader, GLenum pname, GLint *params) \
  X(void, glGetShaderInfoLog, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) \
  X(GLint, glGetUniformLocation, GLuint program, const GLchar *name) \
  X(void, glLinkProgram, GLuint program) \
  X(void, glShaderSource, GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) \
  X(void, glUseProgram, GLuint program) \
  X(void, glUniform1i, GLint location, GLint v0) \
  X(void, glUniform1fv, GLint location, GLsizei count, const GLfloat *value) \
  X(void, glUniform2fv, GLint location, GLsizei count, const GLfloat *value) \
  X(void, glUniform3fv, GLint location, GLsizei count, const GLfloat *value) \
  X(void, glUniform4fv, GLint location, GLsizei count, const GLfloat *value) \
  X(void, glUniform1iv, GLint location, GLsizei count, const GLint *value) \
  X(void, glUniform2iv, GLint location, GLsizei count, const GLint *value) \
  X(void, glUniform3iv, GLint location, GLsizei count, const GLint *value) \
  X(void, glUniform4iv, GLint location, GLsizei count, const GLint *value) \
  X(void, glUniformMatrix2fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix3fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniform1f, GLint location, GLfloat v0) \
  X(void, glUniform1ui, GLint location, GLuint v0) \
  X(void, glVertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) \
  X(void, glUniformMatrix2x3fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix3x2fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix2x4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix4x2fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix3x4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glUniformMatrix4x3fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
  X(void, glBindBufferRange, GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) \
  X(void, glVertexAttribIPointer, GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) \
  X(void, glUniform1uiv, GLint location, GLsizei count, const GLuint *value) \
  X(void, glUniform2uiv, GLint location, GLsizei count, const GLuint *value) \
  X(void, glUniform3uiv, GLint location, GLsizei count, const GLuint *value) \
  X(void, glUniform4uiv, GLint location, GLsizei count, const GLuint *value) \
  X(void, glClearBufferiv, GLenum buffer, GLint drawbuffer, const GLint *value) \
  X(void, glClearBufferuiv, GLenum buffer, GLint drawbuffer, const GLuint *value) \
  X(void, glClearBufferfv, GLenum buffer, GLint drawbuffer, const GLfloat *value) \
  X(void, glClearBufferfi, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) \
  X(void, glBindRenderbuffer, GLenum target, GLuint renderbuffer) \
  X(void, glDeleteRenderbuffers, GLsizei n, const GLuint *renderbuffers) \
  X(void, glGenRenderbuffers, GLsizei n, GLuint *renderbuffers) \
  X(void, glBindFramebuffer, GLenum target, GLuint framebuffer) \
  X(void, glDeleteFramebuffers, GLsizei n, const GLuint *framebuffers) \
  X(void, glGenFramebuffers, GLsizei n, GLuint *framebuffers) \
  X(void, glFramebufferTexture2D, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) \
  X(void, glFramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) \
  X(void, glGenerateMipmap, GLenum target) \
  X(void, glBlitFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) \
  X(void, glRenderbufferStorageMultisample, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) \
  X(void, glFramebufferTextureLayer, GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) \
  X(void, glBindVertexArray, GLuint array) \
  X(void, glDeleteVertexArrays, GLsizei n, const GLuint *arrays) \
  X(void, glGenVertexArrays, GLsizei n, GLuint *arrays) \
  X(void, glDrawArraysInstanced, GLenum mode, GLint first, GLsizei count, GLsizei instancecount) \
  X(void, glDrawElementsInstanced, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) \
  X(void, glCopyBufferSubData, GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) \
  X(GLuint, glGetUniformBlockIndex, GLuint program, const GLchar *uniformBlockName) \
  X(void, glGetActiveUniformBlockiv, GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) \
  X(void, glGetActiveUniformBlockName, GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) \
  X(void, glUniformBlockBinding, GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) \
  X(void, glGenSamplers, GLsizei count, GLuint *samplers) \
  X(void, glDeleteSamplers, GLsizei count, const GLuint *samplers) \
  X(void, glBindSampler, GLuint unit, GLuint sampler) \
  X(void, glSamplerParameteri, GLuint sampler, GLenum pname, GLint param) \
  X(void, glSamplerParameterf, GLuint sampler, GLenum pname, GLfloat param) \
  X(void, glVertexAttribDivisor, GLuint index, GLuint divisor) \
  X(void, glBindBufferBase, GLenum target, GLuint index, GLuint buffer) \
  X(void*, glMapBufferRange, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) \
  X(GLboolean, glUnmapBuffer, GLenum target) \
  X(void, glPixelStorei, GLenum pname, GLint param) \
  X(void, glGetBufferParameteriv, GLenum target, GLenum pname, GLint *params) \
  X(GLsync, glFenceSync, GLenum condition, GLbitfield flags) \
  X(void, glDeleteSync, GLsync sync) \
  X(GLenum, glClientWaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout) \
  X(void, glWaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout) \
  X(void, glFinish, void)

// ======================= OPENGL 4.2 CORE =======================
// Adds Image Load/Store & generic Memory Barriers
#define GL_FUNCTIONS_4_2_CORE(X) \
  X(void, glBindImageTexture, GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) \
  X(void, glMemoryBarrier, GLbitfield barriers)

// ======================= OPENGL 4.3 CORE =======================
// Adds Compute Shaders, SSBOs, Indirect Drawing, and New Query interfaces
#define GL_FUNCTIONS_4_3_CORE(X) \
  X(void, glDispatchCompute, GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) \
  X(void, glGetProgramInterfaceiv, GLuint program, GLenum programInterface, GLenum pname, GLint *params) \
  X(void, glGetProgramResourceiv, GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params) \
  X(void, glGetProgramResourceName, GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name)

// ======================= OPENGL 4.4 CORE =======================
// Adds Immutable Buffer Storage
#define GL_FUNCTIONS_4_4_CORE(X) \
  X(void, glBufferStorage, GLenum target, GLsizeiptr size, const void *data, GLbitfield flags)

// ======================= OPTIONAL / EXPERIMENTAL =======================
// Features that might not be available depending on the driver (e.g. macOS tops at 4.1, some linux drivers top at 4.5, etc.)

// OpenGL 4.3 Core (but marked as optional in original logic)
#define GL_FUNCTIONS_4_3_OPTIONAL(X) \
  X(void, glMultiDrawArraysIndirect, GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride) \
  X(void, glMultiDrawElementsIndirect, GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride)

// OpenGL 4.6 Core (originally ARB_indirect_parameters)
#define GL_FUNCTIONS_4_6_OPTIONAL(X) \
  X(void, glMultiDrawArraysIndirectCount, GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) \
  X(void, glMultiDrawElementsIndirectCount, GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride)

// ARB_bindless_texture (Nvidia/AMD extension, rarely in core context)
#define GL_FUNCTIONS_EXT_BINDLESS(X) \
  X(GLuint64, glGetTextureHandleARB, GLuint texture) \
  X(void, glMakeTextureHandleResidentARB, GLuint64 handle) \
  X(void, glMakeTextureHandleNonResidentARB, GLuint64 handle)

// -----------------------------------------------------------------------------
// Type-Safe Generation
// -----------------------------------------------------------------------------
#ifdef _WIN32
    #define GL_API __stdcall
#else
    #define GL_API
#endif

// Master macro for declaring the function pointer type AND the global pointer instance
#define DECLARE_GL_FUNC(ret, name, ...) \
    typedef ret (GL_API *PFN##name##PROC)(__VA_ARGS__); \
    [[maybe_unused]] static PFN##name##PROC name = nullptr;

// Expand all lists into Declarations
GL_FUNCTIONS_3_3_CORE(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_2_CORE(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_3_CORE(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_4_CORE(DECLARE_GL_FUNC)

GL_FUNCTIONS_4_3_OPTIONAL(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_6_OPTIONAL(DECLARE_GL_FUNC)
GL_FUNCTIONS_EXT_BINDLESS(DECLARE_GL_FUNC)