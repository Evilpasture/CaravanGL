#pragma once
#include "caravangl_specs.h"
#include <Python.h>

typedef struct CaravanGLTable {
#define GL_PTR_GEN(ret, name, ...) ret(GL_API *name)(__VA_ARGS__);
  GL_FUNCTIONS_3_3_CORE(GL_PTR_GEN)
  GL_FUNCTIONS_4_2_CORE(GL_PTR_GEN)
  GL_FUNCTIONS_4_3_CORE(GL_PTR_GEN)
  GL_FUNCTIONS_4_4_CORE(GL_PTR_GEN)
  GL_FUNCTIONS_4_3_OPTIONAL(GL_PTR_GEN)
  GL_FUNCTIONS_4_6_OPTIONAL(GL_PTR_GEN)
  GL_FUNCTIONS_EXT_BINDLESS(GL_PTR_GEN)
#undef GL_PTR_GEN
} CaravanGLTable;

typedef struct CaravanState {
  PyTypeObject *BufferType;
  PyObject *CaravanError;
  CaravanContext ctx;
  CaravanGLTable gl; // Use the named type here
} CaravanState;

[[maybe_unused]]
static inline CaravanState *get_caravan_state(PyObject *m) {
    if (!m || !PyModule_Check(m)) {
        return NULL;
    }
    return (CaravanState *)PyModule_GetState(m);
}

static inline CaravanGLTable gl_table(PyObject *m) {
  return get_caravan_state(m)->gl;
}

#define WithCaravanGL(module_ptr, gl_name)                                     \
  for (PyObject *_cv_m = (PyObject *)(module_ptr); _cv_m != nullptr;           \
       _cv_m = nullptr)                                                        \
    for (CaravanState *state = get_caravan_state(_cv_m); state != nullptr;      \
         state = nullptr)                                                      \
      for (CaravanGLTable gl_name = state->gl; _cv_m != nullptr;               \
           _cv_m = nullptr)