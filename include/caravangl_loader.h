#pragma once
#include "caravangl_module.h"

#ifndef EXTERN_GL

/**
 * Internal helper to bridge Python's loader to C's function pointers.
 */
static void *load_opengl_function(PyObject *loader_function, const char *method) {
    auto res = PyObject_CallFunction(loader_function, "s", method);
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
 * load_gl_table: Populates a specific context's function table.
 * Returns 0 on success, -1 on failure.
 */
[[maybe_unused]] [[nodiscard]]
static int load_gl_table(CaravanGLTable *table, PyObject *loader) {
    if (!table || !loader) {
        return -1;
    }

    // 1. Resolve the Python loader method
    PyObject *loader_function = PyObject_GetAttrString(loader, "load_opengl_function");
    if (!loader_function) {
        PyErr_SetString(PyExc_ValueError, "Loader object missing 'load_opengl_function' method.");
        return -1;
    }

    // 2. Track missing required functions
    PyObject *missing = PyList_New(0);
    if (!missing) {
        Py_DECREF(loader_function);
        return -1;
    }

    // --- MACRO: REQUIRED (OpenGL 3.3 Core Baseline) ---
#define LOAD_REQUIRED(ret, func_name, ...)                                                         \
    _Pragma("unroll 2") do {                                                                       \
        void *ptr = load_opengl_function(loader_function, "gl" #func_name);                        \
        table->func_name = (typeof(table->func_name))ptr;                                          \
        if (!ptr) {                                                                                \
            PyObject *name_obj = PyUnicode_FromString("gl" #func_name);                            \
            PyList_Append(missing, name_obj);                                                      \
            Py_DECREF(name_obj);                                                                   \
        }                                                                                          \
    }                                                                                              \
    while (false)                                                                                  \
        ;

    // --- MACRO: OPTIONAL (4.2+ and Extensions) ---
#ifndef __APPLE__
    // Standard path: Windows/Linux
#define LOAD_OPTIONAL(ret, func_name, ...)                                                         \
    do {                                                                                           \
        table->func_name =                                                                         \
            (typeof(table->func_name))load_opengl_function(loader_function, "gl" #func_name);      \
    } while (false);
#else
    // macOS path: Poisoned/Disabled for 4.2+
#define LOAD_OPTIONAL(ret, func_name, ...) /* NOP */
#endif

    // --- 3. EXECUTION ---

    // REQUIRED 3.3 Core (Must be present on all platforms)
    GL_FUNCTIONS_3_3_CORE(LOAD_REQUIRED)

    // OPTIONAL 4.2+ (May be NULL on Windows/Linux, always NULL on macOS)
    GL_FUNCTIONS_4_2_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_3_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_4_CORE(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_3_OPTIONAL(LOAD_OPTIONAL)
    GL_FUNCTIONS_4_6_OPTIONAL(LOAD_OPTIONAL)
    GL_FUNCTIONS_EXT_BINDLESS(LOAD_OPTIONAL)

#undef LOAD_REQUIRED
#undef LOAD_OPTIONAL

    Py_DECREF(loader_function);

    // 4. Final Validation
    Py_ssize_t missing_count = PyList_Size(missing);
    if (missing_count > 0) {
        PyErr_Format(PyExc_RuntimeError,
                     "Your GPU driver does not support the required OpenGL 3.3 Core profile.\n"
                     "Missing %zd required functions: %R",
                     missing_count, missing);
        Py_DECREF(missing);
        return -1;
    }

    Py_DECREF(missing);
    return 0;
}

#else
// No-op for external linking
static int load_gl_table([[maybe_unused]] CaravanGLTable *table,
                         [[maybe_unused]] PyObject *loader) {
    return 0;
}
#endif