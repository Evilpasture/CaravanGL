#pragma once
#include "caravangl_specs.h"
#include <Python.h>
#if __has_include("caravangl_arg_indices.h")
#include "caravangl_arg_indices.h"
#endif

typedef struct CaravanGLTable {
  // 3.3 is safe on Mac
  #define GL_PTR_GEN(ret, name, ...) ret(GL_API *name)(__VA_ARGS__);
  GL_FUNCTIONS_3_3_CORE(GL_PTR_GEN)
  #undef GL_PTR_GEN

  // 4.2+ gets the "Silly Apple" treatment
  #ifndef __APPLE__
    // Standard path for Linux/Windows
    #define GL_PTR_4_2(ret, name, ...) ret(GL_API *name)(__VA_ARGS__);
  #else
    // Mac path: Functions exist in the struct (to keep offsets same) 
    // but are poisoned to cause a compile error if used.
    #define GL_PTR_4_2(ret, name, ...) \
      __attribute__((unavailable("OpenGL 4.2+ is not supported on macOS"))) \
      ret(GL_API *name)(__VA_ARGS__);
  #endif

  GL_FUNCTIONS_4_2_CORE(GL_PTR_4_2)
  GL_FUNCTIONS_4_3_CORE(GL_PTR_4_2)
  GL_FUNCTIONS_4_4_CORE(GL_PTR_4_2)
  GL_FUNCTIONS_4_3_OPTIONAL(GL_PTR_4_2)
  GL_FUNCTIONS_4_6_OPTIONAL(GL_PTR_4_2)
  GL_FUNCTIONS_EXT_BINDLESS(GL_PTR_4_2)
  
  #undef GL_PTR_DEPRECATED
} CaravanGLTable;

typedef struct CaravanState {
  PyTypeObject *BufferType;
  PyTypeObject *PipelineType;
  PyObject *CaravanError;
  CaravanContext ctx;
  CaravanGLTable gl;
  #if __has_include("caravangl_arg_indices.h")
  CaravanParsers parsers;
  #endif
} CaravanState;

[[maybe_unused]]
static inline CaravanState *get_caravan_state(PyObject *m) {
    if (!m || !PyModule_Check(m)) {
        return nullptr;
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

#ifdef NDEBUG
    /* --- RELEASE MODE: High Performance, No Stalls --- */
    
    // Just execute the call. No error checking, no stalls.
    #define CARAVAN_GL_CHECK(gl_ptr, call) (call)
    #define CARAVAN_GL_CHECK_INT(gl_ptr, call) (call)

#else
    /* --- DEBUG MODE: Strict Error Checking --- */

    #define CARAVAN_GL_CHECK(gl_ptr, call)                                     \
      do {                                                                     \
        while ((gl_ptr).GetError() != GL_NO_ERROR);                            \
        (call);                                                                \
        GLenum _cv_err = (gl_ptr).GetError();                                  \
        if (_cv_err != GL_NO_ERROR) {                             \
          PyErr_Format(PyExc_RuntimeError, "OpenGL Error [0x%04X] in: %s",     \
                       _cv_err, #call);                                        \
          return nullptr;                                                      \
        }                                                                      \
      } while (false)

    #define CARAVAN_GL_CHECK_INT(gl_ptr, call)                                 \
      do {                                                                     \
        while ((gl_ptr).GetError() != GL_NO_ERROR);                            \
        (call);                                                                \
        GLenum _cv_err = (gl_ptr).GetError();                                  \
        if (_cv_err != GL_NO_ERROR) {                             \
          PyErr_Format(PyExc_RuntimeError, "OpenGL Error [0x%04X] in: %s",     \
                       _cv_err, #call);                                        \
          return -1;                                                           \
        }                                                                      \
      } while (false)

#endif