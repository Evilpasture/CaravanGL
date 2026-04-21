#pragma once
#include "fast_parse.h"

/**
 * SCHEMA DEFINITIONS
 * These define the "contracts" for your functions.
 */

#define SCHEMA_INIT(X) X(IDX_INIT_LOADER, "loader", PyObject *, 1)

#define SCHEMA_BUF_INIT(X)                                                                         \
    X(IDX_BUF_SIZE, "size", Py_ssize_t, 1)                                                         \
    X(IDX_BUF_DATA, "data", PyObject *, 0)                                                         \
    X(IDX_BUF_TARGET, "target", uint32_t, 0)                                                       \
    X(IDX_BUF_USAGE, "usage", uint32_t, 0)

#define SCHEMA_BUF_WRITE(X)                                                                        \
    X(IDX_BUF_WRITE_DATA, "data", PyObject *, 1)                                                   \
    X(IDX_BUF_WRITE_OFFSET, "offset", int, 0)

#define SCHEMA_BUF_BIND(X) X(IDX_BUF_BIND_IDX, "index", uint32_t, 1)

#define SCHEMA_PIPELINE_INIT(X)                                                                    \
    X(IDX_PL_PROGRAM, "program", PyObject *, 1)                                                    \
    X(IDX_PL_VAO, "vao", PyObject *, 1)                                                            \
    X(IDX_PL_TOPO, "topology", uint32_t, 0)                                                        \
    X(IDX_PL_IDX_TYP, "index_type", uint32_t, 0)                                                   \
    X(IDX_PL_DEPTH, "depth_test", int, 0)                                                          \
    X(IDX_PL_DWRITE, "depth_write", int, 0)                                                        \
    X(IDX_PL_DFUNC, "depth_func", uint32_t, 0)                                                     \
    X(IDX_PL_CULL, "cull", int, 0)                                                                 \
    X(IDX_PL_CULL_MODE, "cull_mode", uint32_t, 0)                                                  \
    X(IDX_PL_FRONT_FACE, "front_face", uint32_t, 0)                                                \
    X(IDX_PL_STENCIL, "stencil_test", int, 0)                                                      \
    X(IDX_PL_SFUNC, "stencil_func", uint32_t, 0)                                                   \
    X(IDX_PL_SREF, "stencil_ref", int, 0)                                                          \
    X(IDX_PL_SRMASK, "stencil_read_mask", uint32_t, 0)                                             \
    X(IDX_PL_SWMASK, "stencil_write_mask", uint32_t, 0)                                            \
    X(IDX_PL_SFAIL, "stencil_fail", uint32_t, 0)                                                   \
    X(IDX_PL_SZFAIL, "stencil_zfail", uint32_t, 0)                                                 \
    X(IDX_PL_SZPASS, "stencil_zpass", uint32_t, 0)                                                 \
    X(IDX_PL_BLEND, "blend", int, 0)                                                               \
    X(IDX_PL_B_SRC_RGB, "blend_src_rgb", uint32_t, 0)                                              \
    X(IDX_PL_B_DST_RGB, "blend_dst_rgb", uint32_t, 0)                                              \
    X(IDX_PL_B_SRC_A, "blend_src_alpha", uint32_t, 0)                                              \
    X(IDX_PL_B_DST_A, "blend_dst_alpha", uint32_t, 0)                                              \
    X(IDX_PL_B_EQ_RGB, "blend_eq_rgb", uint32_t, 0)                                                \
    X(IDX_PL_B_EQ_A, "blend_eq_alpha", uint32_t, 0)

#define SCHEMA_PROG_INIT(X)                                                                        \
    X(IDX_PROG_VS, "vertex_shader", const char *, 1)                                               \
    X(IDX_PROG_FS, "fragment_shader", const char *, 1)

#define SCHEMA_VAO_ATTR(X)                                                                         \
    X(IDX_VAO_ATTR_LOC, "location", uint32_t, 1)                                                   \
    X(IDX_VAO_ATTR_BUF, "buffer", PyObject *, 1)                                                   \
    X(IDX_VAO_ATTR_SIZE, "size", int, 1)                                                           \
    X(IDX_VAO_ATTR_TYPE, "type", uint32_t, 1)                                                      \
    X(IDX_VAO_ATTR_NORM, "normalized", int, 0)                                                     \
    X(IDX_VAO_ATTR_STRIDE, "stride", int, 0)                                                       \
    X(IDX_VAO_ATTR_OFFSET, "offset", int, 0)                                                       \
    X(IDX_VAO_ATTR_DIV, "divisor", uint32_t, 0)

#define SCHEMA_PL_UNIFORMS(X) X(IDX_PL_U_BATCH, "batch", PyObject *, 1)

#define SCHEMA_UB_INIT(X)                                                                          \
    X(IDX_UB_MAX_BINDS, "max_bindings", int, 1)                                                    \
    X(IDX_UB_MAX_BYTES, "max_bytes", int, 1)

#define SCHEMA_UB_ADD(X)                                                                           \
    X(IDX_UB_ADD_FUNC, "func_id", uint32_t, 1)                                                     \
    X(IDX_UB_ADD_LOC, "location", int, 1)                                                          \
    X(IDX_UB_ADD_CNT, "count", int, 1)                                                             \
    X(IDX_UB_ADD_SIZE, "size", int, 1) // Bytes required for this uniform

#define SCHEMA_TEX_INIT(X) X(IDX_TEX_TARGET, "target", uint32_t, 0)

#define SCHEMA_TEX_UPLOAD(X)                                                                       \
    X(IDX_TEX_UPL_W, "width", int, 1)                                                              \
    X(IDX_TEX_UPL_H, "height", int, 1)                                                             \
    X(IDX_TEX_UPL_IFMT, "internal_format", uint32_t, 1)                                            \
    X(IDX_TEX_UPL_FMT, "format", uint32_t, 1)                                                      \
    X(IDX_TEX_UPL_TYPE, "type", uint32_t, 1)                                                       \
    X(IDX_TEX_UPL_DATA, "data", PyObject *, 0)                                                     \
    X(IDX_TEX_UPL_LEVEL, "level", int, 0)                                                          \
    X(IDX_TEX_UPL_D, "depth", int, 0)

#define SCHEMA_TEX_BIND(X)                                                                         \
    X(IDX_TEX_BIND_UNIT, "unit", uint32_t, 1)                                                      \
    X(IDX_TEX_BIND_SAMP, "sampler", PyObject *, 0)

#define SCHEMA_FBO_ATTACH(X)                                                                       \
    X(IDX_FBO_ATT_ATTACH, "attachment", uint32_t, 1)                                               \
    X(IDX_FBO_ATT_TEX, "texture", PyObject *, 1)                                                   \
    X(IDX_FBO_ATT_LEVEL, "level", int, 0)

#define SCHEMA_CLEAR(X) X(IDX_CLR_MASK, "mask", uint32_t, 1)

#define SCHEMA_CLEAR_COLOR(X)                                                                      \
    X(IDX_CLR_C_R, "r", float, 1)                                                                  \
    X(IDX_CLR_C_G, "g", float, 1)                                                                  \
    X(IDX_CLR_C_B, "b", float, 1)                                                                  \
    X(IDX_CLR_C_A, "a", float, 1)

#define SCHEMA_VIEWPORT(X)                                                                         \
    X(IDX_VP_X, "x", int, 1)                                                                       \
    X(IDX_VP_Y, "y", int, 1)                                                                       \
    X(IDX_VP_W, "width", int, 1)                                                                   \
    X(IDX_VP_H, "height", int, 1)

#define SCHEMA_SAMPLER_INIT(X)                                                                     \
    X(IDX_SAMP_MIN, "min_filter", uint32_t, 0)                                                     \
    X(IDX_SAMP_MAG, "mag_filter", uint32_t, 0)                                                     \
    X(IDX_SAMP_WRAP_S, "wrap_s", uint32_t, 0)                                                      \
    X(IDX_SAMP_WRAP_T, "wrap_t", uint32_t, 0)

#define SCHEMA_CONTEXT_INIT(X)                                                                     \
    X(IDX_CTX_LOADER, "loader", PyObject *, true)                                                  \
    X(IDX_CTX_CALLBACK, "os_make_current_cb", PyObject *, false)                                   \
    X(IDX_CTX_RELEASE_CB, "os_release_cb", PyObject *, false)

#define SCHEMA_SYNC_WAIT(X) X(IDX_SYNC_WAIT_SEC, "timeout_sec", float, false)

#define SCHEMA_QUERY_INIT(X) X(IDX_QUERY_TARGET, "target", uint32_t, false)

#define SCHEMA_COMPUTE_INIT(X) X(IDX_COMP_SRC, "source", const char *, true)

#define SCHEMA_COMPUTE_DISPATCH(X)                                                                 \
    X(IDX_COMP_X, "x", uint32_t, 1)                                                                \
    X(IDX_COMP_Y, "y", uint32_t, 1)                                                                \
    X(IDX_COMP_Z, "z", uint32_t, 1)

/** --- THE GENERATOR ENGINE --- **/

#define GEN_ENUM(ID, NAME, TYPE, REQ) ID,

// Defines GroupName_Idx enum and GroupName_COUNT
#define DEFINE_INDEX_GROUP(GroupName, Schema)                                                      \
    typedef enum { Schema(GEN_ENUM) GroupName##_COUNT } GroupName##_Idx;

// Define the index groups
DEFINE_INDEX_GROUP(Init, SCHEMA_INIT)
DEFINE_INDEX_GROUP(BufInit, SCHEMA_BUF_INIT)
DEFINE_INDEX_GROUP(BufWrite, SCHEMA_BUF_WRITE)
DEFINE_INDEX_GROUP(BufBind, SCHEMA_BUF_BIND)
DEFINE_INDEX_GROUP(PipelineInit, SCHEMA_PIPELINE_INIT)
DEFINE_INDEX_GROUP(ProgInit, SCHEMA_PROG_INIT)
DEFINE_INDEX_GROUP(VaoAttr, SCHEMA_VAO_ATTR)
DEFINE_INDEX_GROUP(PipelineUniforms, SCHEMA_PL_UNIFORMS)
DEFINE_INDEX_GROUP(UniformBatchInit, SCHEMA_UB_INIT)
DEFINE_INDEX_GROUP(UniformBatchAdd, SCHEMA_UB_ADD)
DEFINE_INDEX_GROUP(TexInit, SCHEMA_TEX_INIT)
DEFINE_INDEX_GROUP(TexUpload, SCHEMA_TEX_UPLOAD)
DEFINE_INDEX_GROUP(TexBind, SCHEMA_TEX_BIND)
DEFINE_INDEX_GROUP(FboAttach, SCHEMA_FBO_ATTACH)
DEFINE_INDEX_GROUP(Clear, SCHEMA_CLEAR)
DEFINE_INDEX_GROUP(ClearColor, SCHEMA_CLEAR_COLOR)
DEFINE_INDEX_GROUP(Viewport, SCHEMA_VIEWPORT)
DEFINE_INDEX_GROUP(SamplerInit, SCHEMA_SAMPLER_INIT)
DEFINE_INDEX_GROUP(ContextInit, SCHEMA_CONTEXT_INIT)
DEFINE_INDEX_GROUP(SyncWait, SCHEMA_SYNC_WAIT)
DEFINE_INDEX_GROUP(QueryInit, SCHEMA_QUERY_INIT)
DEFINE_INDEX_GROUP(ComputeInit, SCHEMA_COMPUTE_INIT)
DEFINE_INDEX_GROUP(ComputeDispatch, SCHEMA_COMPUTE_DISPATCH)

// Master list of all parsers
#define FOR_ALL_PARSERS(X)                                                                         \
    X(ContextInit, ContextInit, SCHEMA_CONTEXT_INIT)                                               \
    X(Init, Init, SCHEMA_INIT)                                                                     \
    X(BufInit, BufInit, SCHEMA_BUF_INIT)                                                           \
    X(BufWrite, BufWrite, SCHEMA_BUF_WRITE)                                                        \
    X(BufBind, BufBind, SCHEMA_BUF_BIND)                                                           \
    X(PipelineInit, PipelineInit, SCHEMA_PIPELINE_INIT)                                            \
    X(ProgInit, ProgInit, SCHEMA_PROG_INIT)                                                        \
    X(VaoAttr, VaoAttr, SCHEMA_VAO_ATTR)                                                           \
    X(PipelineUniforms, PipelineUniforms, SCHEMA_PL_UNIFORMS)                                      \
    X(UniformBatchInit, UniformBatchInit, SCHEMA_UB_INIT)                                          \
    X(UniformBatchAdd, UniformBatchAdd, SCHEMA_UB_ADD)                                             \
    X(TexInit, TexInit, SCHEMA_TEX_INIT)                                                           \
    X(TexUpload, TexUpload, SCHEMA_TEX_UPLOAD)                                                     \
    X(TexBind, TexBind, SCHEMA_TEX_BIND)                                                           \
    X(FboAttach, FboAttach, SCHEMA_FBO_ATTACH)                                                     \
    X(Clear, Clear, SCHEMA_CLEAR)                                                                  \
    X(ClearColor, ClearColor, SCHEMA_CLEAR_COLOR)                                                  \
    X(Viewport, Viewport, SCHEMA_VIEWPORT)                                                         \
    X(SamplerInit, SamplerInit, SCHEMA_SAMPLER_INIT)                                               \
    X(SyncWait, SyncWait, SCHEMA_SYNC_WAIT)                                                        \
    X(QueryInit, QueryInit, SCHEMA_QUERY_INIT)                                                     \
    X(ComputeInit, ComputeInit, SCHEMA_COMPUTE_INIT)                                               \
    X(ComputeDispatch, ComputeDispatch, SCHEMA_COMPUTE_DISPATCH)

// Define specialized mappers to split the declarations
#define MAP_ONLY_PARSER(Name, ...) FastParser Name##Parser;
#define MAP_ONLY_SPECS(Name, Group, ...) FastArgSpec Name##Specs[Group##_COUNT];

typedef struct CaravanParsers {
    // Pass 1: All 64-byte structs (Perfectly packed)
    FOR_ALL_PARSERS(MAP_ONLY_PARSER)

    // Pass 2: All 48-byte spec arrays
    FOR_ALL_PARSERS(MAP_ONLY_SPECS)

    // Pass 3: The leftover scalars
    size_t registry_count;
} CaravanParsers;

void caravan_init_parsers(CaravanParsers *commonptr);
void caravan_free_parsers(CaravanParsers *commonptr);
