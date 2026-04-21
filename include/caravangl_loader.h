#pragma once
#define PY_VERSION
#include "caravangl_core.h"

#ifndef EXTERN_GL

// -----------------------------------------------------------------------------
// PURE C API (No Python Dependencies)
// -----------------------------------------------------------------------------

/**
 * Signature for the OS-level proc address loader.
 * Compatible with SDL_GL_GetProcAddress, glfwGetProcAddress, etc.
 * 'user_data' is passed through for context-aware loaders (like Python wrappers).
 */
typedef void *(*CaravanGLLoaderFunc)(const char *name, void *user_data);

/**
 * Struct to track loading failures for required functions.
 */
typedef struct CaravanGLLoadError {
    int missing_count;
    const char *first_missing;
} CaravanGLLoadError;

// --- SUB-LOADERS ---

static void cv_load_group_3_3(CaravanGLTable *table, CaravanGLLoaderFunc loader, void *user_data,
                              CaravanGLLoadError *err) {
    // Inline typedef ensures ISO C compliant casting from void* to function pointer
#define LOAD_REQUIRED(ret, func_name, ...)                                                         \
    typedef ret(GL_API *PFN_##func_name)(__VA_ARGS__);                                             \
    table->func_name = (PFN_##func_name)loader("gl" #func_name, user_data);                        \
    if (!table->func_name) [[clang::unlikely]] {                                                   \
        if (err->missing_count == 0) {                                                             \
            err->first_missing = "gl" #func_name;                                                  \
        }                                                                                          \
        err->missing_count++;                                                                      \
    }

    GL_FUNCTIONS_3_3_CORE(LOAD_REQUIRED)
#undef LOAD_REQUIRED
}

static void cv_load_group_modern(CaravanGLTable *table, CaravanGLLoaderFunc loader,
                                 void *user_data) {
#ifndef __APPLE__
#define LOAD_OPTIONAL(ret, func_name, ...)                                                         \
    typedef ret(GL_API *PFN_##func_name)(__VA_ARGS__);                                             \
    table->func_name = (PFN_##func_name)loader("gl" #func_name, user_data);

    GL_FUNCTIONS_4_2_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_3_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_4_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_5_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_3_OPTIONAL(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_6_OPTIONAL(LOAD_OPTIONAL)
    GL_FUNCTIONS_EXT_BINDLESS(LOAD_OPTIONAL)
#undef LOAD_OPTIONAL
#endif
}

/**
 * caravan_load_gl_table: Pure C entry point to populate the GL table.
 * Returns 0 on success, -1 if any REQUIRED (OpenGL 3.3) functions are missing.
 */
[[maybe_unused]] [[nodiscard]] static int caravan_load_gl_table(CaravanGLTable *table,
                                                                CaravanGLLoaderFunc loader_fn,
                                                                void *user_data,
                                                                CaravanGLLoadError *out_err) {
    if (!table || !loader_fn) {
        return -1;
    }

    CaravanGLLoadError err = {0, nullptr};

    // 1. Execute segmented loading
    cv_load_group_3_3(table, loader_fn, user_data, &err);
    cv_load_group_modern(table, loader_fn, user_data);

    if (out_err) {
        *out_err = err;
    }

    // 2. Validate
    return (err.missing_count == 0) ? 0 : -1;
}

// -----------------------------------------------------------------------------
// PYTHON SHIM (Only active when compiled as a Python extension)
// -----------------------------------------------------------------------------
#ifdef PY_VERSION
#include <Python.h>

/**
 * Trampoline function: Adapts the C loader signature to Python's C API.
 */
static void *py_caravan_proc_wrapper(const char *name, void *user_data) {
    PyObject *loader_function = (PyObject *)user_data;

    PyObject *res = PyObject_CallFunction(loader_function, "s", name);
    if (!res) [[clang::unlikely]] {
        PyErr_Clear(); // Clear here, we raise a bulk error at the end
        return nullptr;
    }

    if (res == Py_None) {
        Py_DECREF(res);
        return nullptr;
    }

    void *ptr = PyLong_AsVoidPtr(res);
    if (PyErr_Occurred()) {
        PyErr_Clear();
        ptr = nullptr;
    }

    Py_DECREF(res);
    return ptr;
}

/**
 * load_gl_table: The original Python entry point.
 * Now acts as a wrapper around the Pure C implementation.
 */
[[maybe_unused]] [[nodiscard]] static int load_gl_table(CaravanGLTable *table, PyObject *loader) {
    if (!table || !loader) {
        return -1;
    }

    PyObject *loader_function = PyObject_GetAttrString(loader, "load_opengl_function");
    if (!loader_function) {
        PyErr_SetString(PyExc_ValueError, "Loader missing 'load_opengl_function' method.");
        return -1;
    }

    CaravanGLLoadError err = {0, nullptr};

    // Call the pure C loader, passing the Python method as user_data
    int status = caravan_load_gl_table(table, py_caravan_proc_wrapper, loader_function, &err);

    Py_DECREF(loader_function);

    if (status < 0) {
        PyErr_Format(PyExc_RuntimeError,
                     "GPU driver missing %d required OpenGL 3.3 Core functions. "
                     "First missing function: %s",
                     err.missing_count, err.first_missing);
        return -1;
    }

    return 0;
}

#endif // PY_VERSION

#else
// Mock version for EXTERN_GL environments
[[maybe_unused]] static int caravan_load_gl_table(CaravanGLTable *table, CaravanGLLoaderFunc loader,
                                                  void *user_data, CaravanGLLoadError *out_err) {
    return 0;
}
#ifdef PY_VERSION
[[maybe_unused]] static int load_gl_table(CaravanGLTable *table, PyObject *loader) {
    return 0;
}
#endif
#endif