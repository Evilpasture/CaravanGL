#pragma once
#include "fast_parse.h"

/**
 * SCHEMA DEFINITIONS
 * These define the "contracts" for your functions.
 */

#define SCHEMA_INIT(X) X(IDX_INIT_LOADER, "loader", PyObject *, 1)

#define SCHEMA_BUF_INIT(X)                                                                         \
    X(IDX_BUF_SIZE, "size", int, 1)                                                                \
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
    X(IDX_PL_BLEND, "blend", int, 0)                                                               \
    X(IDX_PL_CULL, "cull", int, 0)

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
    X(IDX_VAO_ATTR_OFFSET, "offset", int, 0)

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

#define SCHEMA_TEX_BIND(X) X(IDX_TEX_BIND_UNIT, "unit", uint32_t, 1)

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
    X(IDX_VP_W, "w", int, 1)                                                                       \
    X(IDX_VP_H, "h", int, 1)

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

// Master list of all parsers
#define FOR_ALL_PARSERS(X)                                                                         \
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
    X(Viewport, Viewport, SCHEMA_VIEWPORT)

// Macro to declare the struct members
#define MAP_TO_DECLARE(ParserName, GroupName, Schema)                                              \
    FastParser ParserName##Parser;                                                                 \
    FastArgSpec ParserName##Specs[GroupName##_COUNT];

typedef struct CaravanParsers {
    [[gnu::aligned(128)]]
    FOR_ALL_PARSERS(MAP_TO_DECLARE) size_t registry_count;
} CaravanParsers;

void caravan_init_parsers(CaravanParsers *cp);
void caravan_free_parsers(CaravanParsers *cp);
