#pragma once
#include <Python.h>
#include "caravangl.h"

typedef struct {
    PyObject_HEAD
    CaravanBuffer buf;
    PyObject *weakreflist; // Support for weak references
} PyCaravanBuffer;

typedef struct PyCaravanPipeline {
    PyObject_HEAD
    
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