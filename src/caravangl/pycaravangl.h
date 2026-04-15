#pragma once
#include <Python.h>
#include "caravangl_uniform_upload.h"

#define PyCaravanGL_API [[nodiscard]] PyObject *
#define PyCaravanGL_Status [[nodiscard]] int
#define PyCaravanGL_Slot void

typedef struct {
    PyObject_HEAD CaravanBuffer buf;
    PyObject *weakreflist; // Support for weak references
} PyCaravanBuffer;

typedef struct PyCaravanPipeline {
    PyObject_HEAD

        PyObject *program_ref;
    PyObject *vao_ref;

    // Core GPU Objects
    GLuint program;
    GLuint vao;

    // Topology and Indexing
    GLenum topology;   // GL_TRIANGLES, GL_LINES, etc.
    GLenum index_type; // GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, or GL_NONE (0)

    // State Requirements
    CaravanRenderState render_state;

    // Fast-mutation draw data
    CaravanDrawParams params;

    // Python memory view for zero-overhead parameter mutation
    PyObject *params_view;
    Py_buffer params_buffer;
} PyCaravanPipeline;

typedef struct {
    PyObject_HEAD GLuint id;
} PyCaravanProgram;

typedef struct {
    PyObject_HEAD GLuint id;
} PyCaravanVertexArray;

typedef struct {
    PyObject_HEAD CaravanUniformHeader *header;
    char *payload;

    uint32_t max_bindings;
    uint32_t max_payload_bytes;
    uint32_t current_payload_offset;

    // Memory view exposed to Python
    Py_buffer payload_buffer;
    PyObject *payload_view;
} PyCaravanUniformBatch;