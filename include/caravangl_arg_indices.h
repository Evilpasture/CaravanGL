#pragma once
#include "fast_parse.h"

/**
 * SCHEMA DEFINITIONS
 * These define the "contracts" for your functions.
 */

#define SCHEMA_INIT(X) \
    X(IDX_INIT_LOADER, "loader", PyObject *, 1)

#define SCHEMA_BUF_INIT(X) \
    X(IDX_BUF_SIZE,   "size",   int,      1) \
    X(IDX_BUF_DATA,   "data",   PyObject *, 0) \
    X(IDX_BUF_TARGET, "target", uint32_t,   0) \
    X(IDX_BUF_USAGE,  "usage",  uint32_t,   0)

#define SCHEMA_BUF_WRITE(X) \
    X(IDX_BUF_WRITE_DATA,   "data",   PyObject *, 1) \
    X(IDX_BUF_WRITE_OFFSET, "offset", int,      0)

#define SCHEMA_BUF_BIND(X) \
    X(IDX_BUF_BIND_IDX, "index", uint32_t, 1)

#define SCHEMA_PIPELINE_INIT(X) \
    X(IDX_PL_PROGRAM, "program",    uint32_t, 1) \
    X(IDX_PL_VAO,     "vao",        uint32_t, 1) \
    X(IDX_PL_TOPO,    "topology",   uint32_t, 0) \
    X(IDX_PL_IDX_TYP, "index_type", uint32_t, 0) \
    X(IDX_PL_DEPTH,   "depth_test", int,      0) \
    X(IDX_PL_DWRITE,  "depth_write",int,      0) \
    X(IDX_PL_DFUNC,   "depth_func", uint32_t, 0) \
    X(IDX_PL_BLEND,   "blend",      int,      0) \
    X(IDX_PL_CULL,    "cull",       int,      0)

/** --- THE GENERATOR ENGINE --- **/

#define GEN_ENUM(ID, NAME, TYPE, REQ) ID,

// Defines GroupName_Idx enum and GroupName_COUNT
#define DEFINE_INDEX_GROUP(GroupName, Schema) \
    typedef enum { Schema(GEN_ENUM) GroupName##_COUNT } GroupName##_Idx;

// Define the index groups
DEFINE_INDEX_GROUP(Init,     SCHEMA_INIT)
DEFINE_INDEX_GROUP(BufInit,  SCHEMA_BUF_INIT)
DEFINE_INDEX_GROUP(BufWrite, SCHEMA_BUF_WRITE)
DEFINE_INDEX_GROUP(BufBind,  SCHEMA_BUF_BIND)
DEFINE_INDEX_GROUP(PipelineInit, SCHEMA_PIPELINE_INIT)

// Master list of all parsers
#define FOR_ALL_PARSERS(X) \
    X(Init,     Init,     SCHEMA_INIT) \
    X(BufInit,  BufInit,  SCHEMA_BUF_INIT) \
    X(BufWrite, BufWrite, SCHEMA_BUF_WRITE) \
    X(BufBind,  BufBind,  SCHEMA_BUF_BIND) \
    X(PipelineInit, PipelineInit, SCHEMA_PIPELINE_INIT)


// Macro to declare the struct members
#define MAP_TO_DECLARE(ParserName, GroupName, Schema) \
    FastParser ParserName##Parser; \
    FastArgSpec ParserName##Specs[GroupName##_COUNT];

typedef struct CaravanParsers {
    FOR_ALL_PARSERS(MAP_TO_DECLARE)
    size_t registry_count;
} CaravanParsers;

void caravan_init_parsers(CaravanParsers *cp);
void caravan_free_parsers(CaravanParsers *cp);