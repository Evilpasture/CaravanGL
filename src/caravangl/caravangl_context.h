#pragma once
#include "pycaravangl.h"

// THREAD LOCAL STORAGE: Tracks the active context for the current OS thread
extern thread_local PyCaravanContext *cv_active_context;

// API to switch contexts
PyCaravanGL_API Context_make_current(PyCaravanContext *self, PyObject *args);
void cv_flush_garbage(PyCaravanContext *self);
void cv_enqueue_garbage(size_t *count, GLuint *array, GLuint id);