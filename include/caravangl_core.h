#pragma once
#include <stddef.h>
#include <stdint.h>

#define CARAVANGL_CONSTANT [[maybe_unused]] static constexpr

// -----------------------------------------------------------------------------
// Standard GL Types
// -----------------------------------------------------------------------------
typedef uint8_t GLboolean;
typedef uint32_t GLbitfield;
typedef uint32_t GLenum;
typedef uint32_t GLuint;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef void GLvoid;
typedef char GLchar;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef int8_t GLbyte;
typedef uint8_t GLubyte;
typedef int16_t GLshort;
typedef uint16_t GLushort;
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef intptr_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

typedef struct __GLsync *GLsync;

// -----------------------------------------------------------------------------
// OpenGL Constants (Typed for Safety)
// -----------------------------------------------------------------------------
CARAVANGL_CONSTANT GLboolean GL_FALSE = 0;
CARAVANGL_CONSTANT GLboolean GL_TRUE = 1;
CARAVANGL_CONSTANT GLenum GL_NO_ERROR = 0;

CARAVANGL_CONSTANT GLbitfield GL_DEPTH_BUFFER_BIT = 0x0100;
CARAVANGL_CONSTANT GLbitfield GL_STENCIL_BUFFER_BIT = 0x0400;
CARAVANGL_CONSTANT GLbitfield GL_COLOR_BUFFER_BIT = 0x4000;

CARAVANGL_CONSTANT GLenum GL_FRONT = 0x0404;
CARAVANGL_CONSTANT GLenum GL_BACK = 0x0405;
CARAVANGL_CONSTANT GLenum GL_CULL_FACE = 0x0B44;
CARAVANGL_CONSTANT GLenum GL_DEPTH_TEST = 0x0B71;
CARAVANGL_CONSTANT GLenum GL_STENCIL_TEST = 0x0B90;
CARAVANGL_CONSTANT GLenum GL_BLEND = 0x0BE2;
CARAVANGL_CONSTANT GLenum GL_TEXTURE_2D = 0x0DE1;
CARAVANGL_CONSTANT GLenum GL_UNSIGNED_SHORT = 0x1403;
CARAVANGL_CONSTANT GLenum GL_UNSIGNED_INT = 0x1405;

CARAVANGL_CONSTANT GLenum GL_TEXTURE0 = 0x84C0;
CARAVANGL_CONSTANT GLenum GL_ARRAY_BUFFER = 0x8892;
CARAVANGL_CONSTANT GLenum GL_STATIC_DRAW = 0x88E4;
CARAVANGL_CONSTANT GLenum GL_DYNAMIC_DRAW = 0x88E8;

CARAVANGL_CONSTANT GLuint GL_INVALID_INDEX = 0xFFFFFFFFu;
CARAVANGL_CONSTANT GLuint64 GL_TIMEOUT_IGNORED = 0xFFFFFFFFFFFFFFFFull;

CARAVANGL_CONSTANT GLbitfield GL_MAP_READ_BIT = 0x0001;
CARAVANGL_CONSTANT GLbitfield GL_MAP_WRITE_BIT = 0x0002;
CARAVANGL_CONSTANT GLbitfield GL_MAP_PERSISTENT_BIT = 0x0040;
CARAVANGL_CONSTANT GLbitfield GL_MAP_COHERENT_BIT = 0x0080;
CARAVANGL_CONSTANT GLbitfield GL_PERSISTENT_WRITE_FLAGS =
    (GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

// --- Implementation Limits ---
CARAVANGL_CONSTANT GLint GL_MIN_UNIFORM_BUFFER_BINDINGS = 8;
CARAVANGL_CONSTANT GLint GL_MAX_TEXTURE_IMAGE_UNITS = 16;
CARAVANGL_CONSTANT GLint GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = 48;

// -----------------------------------------------------------------------------
// Modern C23 Enums
// -----------------------------------------------------------------------------
typedef enum UniformFunction : uint8_t {
  UF_1I,
  UF_2I,
  UF_3I,
  UF_4I,
  UF_1B,
  UF_2B,
  UF_3B,
  UF_4B,
  UF_1U,
  UF_2U,
  UF_3U,
  UF_4U,
  UF_1F,
  UF_2F,
  UF_3F,
  UF_4F,
  UF_MAT2,
  UF_MAT2x3,
  UF_MAT2x4,
  UF_MAT3x2,
  UF_MAT3,
  UF_MAT3x4,
  UF_MAT4x2,
  UF_MAT4x3,
  UF_MAT4,
  UF_COUNT
} UniformFunction;

typedef enum ImageFormatTupleIndex : uint8_t {
  IF_INTERNAL_FORMAT,
  IF_FORMAT,
  IF_TYPE,
  IF_BUFFER,
  IF_COMPONENTS,
  IF_PIXEL_SIZE,
  IF_COLOR,
  IF_FLAGS,
  IF_CLEAR_TYPE,
  IF_TUPLE_SIZE
} ImageFormatTupleIndex;

#ifdef _WIN32
#define GL_API __stdcall
#else
#define GL_API
#endif

// -----------------------------------------------------------------------------
// OpenGL Single Source of Truth Lists (X-Macros)
// NO "gl" PREFIX HERE!
// -----------------------------------------------------------------------------

#define GL_FUNCTIONS_3_3_CORE(X)                                               \
  X(void, CullFace, GLenum mode)                                               \
  X(void, Clear, GLbitfield mask)                                              \
  X(void, ClearColor, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) \
  X(void, DrawArrays, GLenum mode, GLint first, GLsizei count)                 \
  X(void, DrawElements, GLenum mode, GLsizei count, GLenum type,               \
    const void *indices)                                                       \
  X(void, TexParameteri, GLenum target, GLenum pname, GLint param)             \
  X(void, TexImage2D, GLenum target, GLint level, GLint internalformat,        \
    GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,   \
    const void *pixels)                                                        \
  X(void, DepthMask, GLboolean flag)                                           \
  X(void, Disable, GLenum cap)                                                 \
  X(void, Enable, GLenum cap)                                                  \
  X(void, Flush, void)                                                         \
  X(void, DepthFunc, GLenum func)                                              \
  X(void, ReadBuffer, GLenum src)                                              \
  X(void, ReadPixels, GLint x, GLint y, GLsizei width, GLsizei height,         \
    GLenum format, GLenum type, void *pixels)                                  \
  X(GLenum, GetError, void)                                                    \
  X(void, GetIntegerv, GLenum pname, GLint *data)                              \
  X(const GLchar *, GetString, GLenum name)                                    \
  X(void, Viewport, GLint x, GLint y, GLsizei width, GLsizei height)           \
  X(void, TexSubImage2D, GLenum target, GLint level, GLint xoffset,            \
    GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type,  \
    const void *pixels)                                                        \
  X(void, BindTexture, GLenum target, GLuint texture)                          \
  X(void, DeleteTextures, GLsizei n, const GLuint *textures)                   \
  X(void, GenTextures, GLsizei n, GLuint *textures)                            \
  X(void, TexImage3D, GLenum target, GLint level, GLint internalformat,        \
    GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, \
    GLenum type, const void *pixels)                                           \
  X(void, TexSubImage3D, GLenum target, GLint level, GLint xoffset,            \
    GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,               \
    GLsizei depth, GLenum format, GLenum type, const void *pixels)             \
  X(void, ActiveTexture, GLenum texture)                                       \
  X(void, BlendFuncSeparate, GLenum sfactorRGB, GLenum dfactorRGB,             \
    GLenum sfactorAlpha, GLenum dfactorAlpha)                                  \
  X(void, BindBuffer, GLenum target, GLuint buffer)                            \
  X(void, DeleteBuffers, GLsizei n, const GLuint *buffers)                     \
  X(void, GenBuffers, GLsizei n, GLuint *buffers)                              \
  X(void, BufferData, GLenum target, GLsizeiptr size, const void *data,        \
    GLenum usage)                                                              \
  X(void, BufferSubData, GLenum target, GLintptr offset, GLsizeiptr size,      \
    const void *data)                                                          \
  X(void, GetBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size,   \
    void *data)                                                                \
  X(void, BlendEquationSeparate, GLenum modeRGB, GLenum modeAlpha)             \
  X(void, DrawBuffers, GLsizei n, const GLenum *bufs)                          \
  X(void, StencilOpSeparate, GLenum face, GLenum sfail, GLenum dpfail,         \
    GLenum dppass)                                                             \
  X(void, StencilFuncSeparate, GLenum face, GLenum func, GLint ref,            \
    GLuint mask)                                                               \
  X(void, StencilMaskSeparate, GLenum face, GLuint mask)                       \
  X(void, AttachShader, GLuint program, GLuint shader)                         \
  X(void, DetachShader, GLuint program, GLuint shader)                         \
  X(void, CompileShader, GLuint shader)                                        \
  X(GLuint, CreateProgram, void)                                               \
  X(GLuint, CreateShader, GLenum type)                                         \
  X(void, DeleteProgram, GLuint program)                                       \
  X(void, DeleteShader, GLuint shader)                                         \
  X(void, DeleteQueries, GLsizei n, const GLuint *ids)                         \
  X(void, EnableVertexAttribArray, GLuint index)                               \
  X(void, GetActiveAttrib, GLuint program, GLuint index, GLsizei bufSize,      \
    GLsizei *length, GLint *size, GLenum *type, GLchar *name)                  \
  X(void, GetActiveUniform, GLuint program, GLuint index, GLsizei bufSize,     \
    GLsizei *length, GLint *size, GLenum *type, GLchar *name)                  \
  X(GLint, GetAttribLocation, GLuint program, const GLchar *name)              \
  X(void, GetProgramiv, GLuint program, GLenum pname, GLint *params)           \
  X(void, GetProgramInfoLog, GLuint program, GLsizei bufSize, GLsizei *length, \
    GLchar *infoLog)                                                           \
  X(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params)             \
  X(void, GetShaderInfoLog, GLuint shader, GLsizei bufSize, GLsizei *length,   \
    GLchar *infoLog)                                                           \
  X(GLint, GetUniformLocation, GLuint program, const GLchar *name)             \
  X(void, LinkProgram, GLuint program)                                         \
  X(void, ShaderSource, GLuint shader, GLsizei count,                          \
    const GLchar *const *string, const GLint *length)                          \
  X(void, UseProgram, GLuint program)                                          \
  X(void, Uniform1i, GLint location, GLint v0)                                 \
  X(void, Uniform1fv, GLint location, GLsizei count, const GLfloat *value)     \
  X(void, Uniform2fv, GLint location, GLsizei count, const GLfloat *value)     \
  X(void, Uniform3fv, GLint location, GLsizei count, const GLfloat *value)     \
  X(void, Uniform4fv, GLint location, GLsizei count, const GLfloat *value)     \
  X(void, Uniform1iv, GLint location, GLsizei count, const GLint *value)       \
  X(void, Uniform2iv, GLint location, GLsizei count, const GLint *value)       \
  X(void, Uniform3iv, GLint location, GLsizei count, const GLint *value)       \
  X(void, Uniform4iv, GLint location, GLsizei count, const GLint *value)       \
  X(void, UniformMatrix2fv, GLint location, GLsizei count,                     \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix3fv, GLint location, GLsizei count,                     \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix4fv, GLint location, GLsizei count,                     \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, Uniform1f, GLint location, GLfloat v0)                               \
  X(void, Uniform1ui, GLint location, GLuint v0)                               \
  X(void, VertexAttribPointer, GLuint index, GLint size, GLenum type,          \
    GLboolean normalized, GLsizei stride, const void *pointer)                 \
  X(void, UniformMatrix2x3fv, GLint location, GLsizei count,                   \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix3x2fv, GLint location, GLsizei count,                   \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix2x4fv, GLint location, GLsizei count,                   \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix4x2fv, GLint location, GLsizei count,                   \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix3x4fv, GLint location, GLsizei count,                   \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, UniformMatrix4x3fv, GLint location, GLsizei count,                   \
    GLboolean transpose, const GLfloat *value)                                 \
  X(void, BindBufferRange, GLenum target, GLuint index, GLuint buffer,         \
    GLintptr offset, GLsizeiptr size)                                          \
  X(void, VertexAttribIPointer, GLuint index, GLint size, GLenum type,         \
    GLsizei stride, const void *pointer)                                       \
  X(void, Uniform1uiv, GLint location, GLsizei count, const GLuint *value)     \
  X(void, Uniform2uiv, GLint location, GLsizei count, const GLuint *value)     \
  X(void, Uniform3uiv, GLint location, GLsizei count, const GLuint *value)     \
  X(void, Uniform4uiv, GLint location, GLsizei count, const GLuint *value)     \
  X(void, ClearBufferiv, GLenum buffer, GLint drawbuffer, const GLint *value)  \
  X(void, ClearBufferuiv, GLenum buffer, GLint drawbuffer,                     \
    const GLuint *value)                                                       \
  X(void, ClearBufferfv, GLenum buffer, GLint drawbuffer,                      \
    const GLfloat *value)                                                      \
  X(void, ClearBufferfi, GLenum buffer, GLint drawbuffer, GLfloat depth,       \
    GLint stencil)                                                             \
  X(void, BindRenderbuffer, GLenum target, GLuint renderbuffer)                \
  X(void, DeleteRenderbuffers, GLsizei n, const GLuint *renderbuffers)         \
  X(void, GenRenderbuffers, GLsizei n, GLuint *renderbuffers)                  \
  X(void, BindFramebuffer, GLenum target, GLuint framebuffer)                  \
  X(void, DeleteFramebuffers, GLsizei n, const GLuint *framebuffers)           \
  X(void, GenFramebuffers, GLsizei n, GLuint *framebuffers)                    \
  X(void, FramebufferTexture2D, GLenum target, GLenum attachment,              \
    GLenum textarget, GLuint texture, GLint level)                             \
  X(void, FramebufferRenderbuffer, GLenum target, GLenum attachment,           \
    GLenum renderbuffertarget, GLuint renderbuffer)                            \
  X(void, GenerateMipmap, GLenum target)                                       \
  X(void, BlitFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, \
    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,       \
    GLenum filter)                                                             \
  X(void, RenderbufferStorageMultisample, GLenum target, GLsizei samples,      \
    GLenum internalformat, GLsizei width, GLsizei height)                      \
  X(void, FramebufferTextureLayer, GLenum target, GLenum attachment,           \
    GLuint texture, GLint level, GLint layer)                                  \
  X(void, BindVertexArray, GLuint array)                                       \
  X(void, DeleteVertexArrays, GLsizei n, const GLuint *arrays)                 \
  X(void, GenVertexArrays, GLsizei n, GLuint *arrays)                          \
  X(void, DrawArraysInstanced, GLenum mode, GLint first, GLsizei count,        \
    GLsizei instancecount)                                                     \
  X(void, DrawElementsInstanced, GLenum mode, GLsizei count, GLenum type,      \
    const void *indices, GLsizei instancecount)                                \
  X(void, CopyBufferSubData, GLenum readTarget, GLenum writeTarget,            \
    GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)                \
  X(GLuint, GetUniformBlockIndex, GLuint program,                              \
    const GLchar *uniformBlockName)                                            \
  X(void, GetActiveUniformBlockiv, GLuint program, GLuint uniformBlockIndex,   \
    GLenum pname, GLint *params)                                               \
  X(void, GetActiveUniformBlockName, GLuint program, GLuint uniformBlockIndex, \
    GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName)                \
  X(void, UniformBlockBinding, GLuint program, GLuint uniformBlockIndex,       \
    GLuint uniformBlockBinding)                                                \
  X(void, GenSamplers, GLsizei count, GLuint *samplers)                        \
  X(void, DeleteSamplers, GLsizei count, const GLuint *samplers)               \
  X(void, BindSampler, GLuint unit, GLuint sampler)                            \
  X(void, SamplerParameteri, GLuint sampler, GLenum pname, GLint param)        \
  X(void, SamplerParameterf, GLuint sampler, GLenum pname, GLfloat param)      \
  X(void, VertexAttribDivisor, GLuint index, GLuint divisor)                   \
  X(void, BindBufferBase, GLenum target, GLuint index, GLuint buffer)          \
  X(void *, MapBufferRange, GLenum target, GLintptr offset, GLsizeiptr length, \
    GLbitfield access)                                                         \
  X(GLboolean, UnmapBuffer, GLenum target)                                     \
  X(void, PixelStorei, GLenum pname, GLint param)                              \
  X(void, GetBufferParameteriv, GLenum target, GLenum pname, GLint *params)    \
  X(GLsync, FenceSync, GLenum condition, GLbitfield flags)                     \
  X(void, DeleteSync, GLsync sync)                                             \
  X(GLenum, ClientWaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout)   \
  X(void, WaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout)           \
  X(void, Finish, void)

#define GL_FUNCTIONS_4_2_CORE(X)                                               \
  X(void, BindImageTexture, GLuint unit, GLuint texture, GLint level,          \
    GLboolean layered, GLint layer, GLenum access, GLenum format)              \
  X(void, MemoryBarrier, GLbitfield barriers)

#define GL_FUNCTIONS_4_3_CORE(X)                                               \
  X(void, DispatchCompute, GLuint num_groups_x, GLuint num_groups_y,           \
    GLuint num_groups_z)                                                       \
  X(void, GetProgramInterfaceiv, GLuint program, GLenum programInterface,      \
    GLenum pname, GLint *params)                                               \
  X(void, GetProgramResourceiv, GLuint program, GLenum programInterface,       \
    GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize,     \
    GLsizei *length, GLint *params)                                            \
  X(void, GetProgramResourceName, GLuint program, GLenum programInterface,     \
    GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name)

#define GL_FUNCTIONS_4_4_CORE(X)                                               \
  X(void, BufferStorage, GLenum target, GLsizeiptr size, const void *data,     \
    GLbitfield flags)

#define GL_FUNCTIONS_4_3_OPTIONAL(X)                                           \
  X(void, MultiDrawArraysIndirect, GLenum mode, const void *indirect,          \
    GLsizei drawcount, GLsizei stride)                                         \
  X(void, MultiDrawElementsIndirect, GLenum mode, GLenum type,                 \
    const void *indirect, GLsizei drawcount, GLsizei stride)

#define GL_FUNCTIONS_4_6_OPTIONAL(X)                                           \
  X(void, MultiDrawArraysIndirectCount, GLenum mode, const void *indirect,     \
    GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride)                  \
  X(void, MultiDrawElementsIndirectCount, GLenum mode, GLenum type,            \
    const void *indirect, GLintptr drawcount, GLsizei maxdrawcount,            \
    GLsizei stride)

#define GL_FUNCTIONS_EXT_BINDLESS(X)                                           \
  X(GLuint64, GetTextureHandleARB, GLuint texture)                             \
  X(void, MakeTextureHandleResidentARB, GLuint64 handle)                       \
  X(void, MakeTextureHandleNonResidentARB, GLuint64 handle)