#pragma once
#include "caravangl_module.h"

// -----------------------------------------------------------------------------
// OpenGL Loader Implementation (Isolated State)
// -----------------------------------------------------------------------------

#ifndef EXTERN_GL

static void *load_opengl_function(PyObject *loader_function,
                                  const char *method) {
  // Calling the Python loader method
  auto *res = PyObject_CallFunction(loader_function, "s", method);
  if (!res)
    return nullptr;

  // If Python returns None, return nullptr immediately without error
  if (res == Py_None) {
    Py_DECREF(res);
    return nullptr;
  }

  void *ptr = PyLong_AsVoidPtr(res);

  // If PyLong_AsVoidPtr failed (e.g. not an int), catch it here
  if (PyErr_Occurred()) {
    PyErr_Clear(); // Clear the "integer required" error
    ptr = nullptr;
  }

  Py_DECREF(res);
  return ptr;
}

[[maybe_unused]]
static int load_gl(CaravanState *state, PyObject *loader) {
  if (!state)
    return -1;

  auto *loader_function =
      PyObject_GetAttrString(loader, "load_opengl_function");
  if (!loader_function) {
    PyErr_Format(PyExc_ValueError,
                 "loader object missing 'load_opengl_function'");
    return -1;
  }

  auto *missing = PyList_New(0);

// --- 1. REQUIRED: 3.3 Core Baseline ---
// (Works on macOS, Linux, Windows)
#define LOAD_REQUIRED(ret, func_name, ...)                                     \
  do {                                                                         \
    auto *ptr = load_opengl_function(loader_function, "gl" #func_name);        \
    state->gl.func_name = (typeof(state->gl.func_name))ptr;                    \
    if (!state->gl.func_name) {                                                \
      auto *s = PyUnicode_FromString("gl" #func_name);                         \
      PyList_Append(missing, s);                                               \
      Py_XDECREF(s);                                                           \
    }                                                                          \
  } while (0);

  GL_FUNCTIONS_3_3_CORE(LOAD_REQUIRED)

// --- 2. OPTIONAL: 4.2, 4.3, 4.4, 4.6 and Extensions ---
// (Will be NULL on macOS, but populated on modern Linux/Windows)
#define LOAD_OPTIONAL(ret, func_name, ...)                                     \
  do {                                                                         \
    auto *ptr = load_opengl_function(loader_function, "gl" #func_name);        \
    state->gl.func_name = (typeof(state->gl.func_name))ptr;                    \
  } while (0);

  GL_FUNCTIONS_4_2_CORE(LOAD_OPTIONAL)
  GL_FUNCTIONS_4_3_CORE(LOAD_OPTIONAL)
  GL_FUNCTIONS_4_4_CORE(LOAD_OPTIONAL)
  GL_FUNCTIONS_4_3_OPTIONAL(LOAD_OPTIONAL)
  GL_FUNCTIONS_4_6_OPTIONAL(LOAD_OPTIONAL)
  GL_FUNCTIONS_EXT_BINDLESS(LOAD_OPTIONAL)

#undef LOAD_REQUIRED
#undef LOAD_OPTIONAL

  Py_DECREF(loader_function);

  if (PyList_Size(missing) > 0) {
    PyErr_Format(PyExc_RuntimeError,
                 "Missing required OpenGL 3.3 functions: %R", missing);
    Py_DECREF(missing);
    return -1;
  }

  Py_DECREF(missing);
  return 0;
}

#else

// No-op for external linking scenarios
static int load_gl([[maybe_unused]] CaravanState *state,
                   [[maybe_unused]] PyObject *loader) {
  return 0;
}

#endif