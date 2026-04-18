#pragma once
#include "caravangl_module.h"

#ifndef EXTERN_GL

/**
 * Internal helper to bridge Python's loader to C's function pointers.
 */
static void *load_opengl_function(PyObject *loader_function, const char *method) {
    PyObject *res = PyObject_CallFunction(loader_function, "s", method);
    if (!res) [[clang::unlikely]] {
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
 * Logic helper to load a required function and track failures.
 */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void cv_do_load_req(PyObject *loader_fn, PyObject *missing, const char *name, void **slot) {
    void *ptr = load_opengl_function(loader_fn, name);
    *slot = ptr;
    if (!ptr) [[clang::unlikely]] {
        PyObject *name_obj = PyUnicode_FromString(name);
        if (name_obj) {
            PyList_Append(missing, name_obj);
            Py_DECREF(name_obj);
        }
    }
}

/**
 * Logic helper to load an optional function.
 */
static void cv_do_load_opt(PyObject *loader_fn, const char *name, void **slot) {
    *slot = load_opengl_function(loader_fn, name);
}

// --- SUB-LOADERS (Segmenting to keep function size small) ---

static void cv_load_group_3_3(CaravanGLTable *table, PyObject *loader_fn, PyObject *missing) {
#define LOAD_REQUIRED(ret, func_name, ...)                                                         \
    cv_do_load_req(loader_fn, missing, "gl" #func_name, (void **)&table->func_name);

    GL_FUNCTIONS_3_3_CORE(LOAD_REQUIRED)
#undef LOAD_REQUIRED
}

static void cv_load_group_modern(CaravanGLTable *table, PyObject *loader_fn) {
#ifndef __APPLE__
#define LOAD_OPTIONAL(ret, func_name, ...)                                                         \
    cv_do_load_opt(loader_fn, "gl" #func_name, (void **)&table->func_name);

    GL_FUNCTIONS_4_2_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_3_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_4_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_3_OPTIONAL(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_6_OPTIONAL(LOAD_OPTIONAL)
    GL_FUNCTIONS_EXT_BINDLESS(LOAD_OPTIONAL)
#undef LOAD_OPTIONAL
#endif
}

/**
 * load_gl_table: Populates a specific context's function table.
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

    PyObject *missing = PyList_New(0);
    if (!missing) {
        Py_DECREF(loader_function);
        return -1;
    }

    // 1. Execute segmented loading
    cv_load_group_3_3(table, loader_function, missing);
    cv_load_group_modern(table, loader_function);

    Py_DECREF(loader_function);

    // 2. Final Validation
    Py_ssize_t missing_count = PyList_Size(missing);
    if (missing_count > 0) {
        PyErr_Format(PyExc_RuntimeError,
                     "GPU driver missing %zd required OpenGL 3.3 Core functions: %R", missing_count,
                     missing);
        Py_DECREF(missing);
        return -1;
    }

    Py_DECREF(missing);
    return 0;
}

#else
static int load_gl_table([[maybe_unused]] CaravanGLTable *table,
                         [[maybe_unused]] PyObject *loader) {
    return 0;
}
#endif