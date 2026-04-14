#pragma once
#include "caravangl_module.h"



// -----------------------------------------------------------------------------
// OpenGL Loader Implementation (Isolated State)
// -----------------------------------------------------------------------------

#ifndef EXTERN_GL

/**
 * Internal helper to call the Python-provided loader function.
 */
static void* load_opengl_function(PyObject* loader_function, const char* method) {
    auto* res = PyObject_CallFunction(loader_function, "(s)", method);
    if (!res) return nullptr;
    
    void* ptr = PyLong_AsVoidPtr(res);
    if (!ptr && PyErr_Occurred()) {
        Py_DECREF(res);
        return nullptr;
    }
    
    Py_DECREF(res);
    return ptr;
}

/**
 * Isolated OpenGL Loader.
 * Populates the function table inside CaravanState->gl.
 */
[[maybe_unused]]
static int load_gl(CaravanState* state, PyObject* loader) {
    if (!state) return -1;

    auto* loader_function = PyObject_GetAttrString(loader, "load_opengl_function");
    if (!loader_function) {
        PyErr_Format(PyExc_ValueError, "loader object missing 'load_opengl_function' attribute");
        return -1;
    }

    auto* missing = PyList_New(0);
    if (!missing) {
        Py_DECREF(loader_function);
        return -1;
    }

    // --- Macro: Required Functions ---
    // Targets state->gl.name and tracks failures in the 'missing' list.
    #define LOAD_REQUIRED_FUNC(ret, func_name, ...)                            \
        do {                                                                   \
            auto* temp_ptr = load_opengl_function(loader_function, #func_name);\
            if (!temp_ptr && PyErr_Occurred()) {                               \
                Py_DECREF(loader_function);                                    \
                Py_DECREF(missing);                                            \
                return -1;                                                     \
            }                                                                  \
            /* Type-safe assignment into state struct using C23 typeof */      \
            state->gl.func_name = (typeof(state->gl.func_name))temp_ptr;       \
            if (!state->gl.func_name) {                                        \
                auto* ogl_str = PyUnicode_FromString(#func_name);              \
                if (ogl_str) PyList_Append(missing, ogl_str);                  \
                Py_XDECREF(ogl_str);                                           \
            }                                                                  \
        } while (0);

    // --- Macro: Optional Functions ---
    // Targets state->gl.name but ignores failures.
    #define LOAD_OPTIONAL_FUNC(ret, func_name, ...)                            \
        do {                                                                   \
            auto* _opt = load_opengl_function(loader_function, #func_name);     \
            if (!_opt && PyErr_Occurred()) {                                   \
                PyErr_Clear();                                                 \
            }                                                                  \
            state->gl.func_name = (typeof(state->gl.func_name))_opt;           \
        } while (0);

    // --- Execute Loading ---
    GL_FUNCTIONS_3_3_CORE(LOAD_REQUIRED_FUNC)
    GL_FUNCTIONS_4_2_CORE(LOAD_REQUIRED_FUNC)
    GL_FUNCTIONS_4_3_CORE(LOAD_REQUIRED_FUNC)
    GL_FUNCTIONS_4_4_CORE(LOAD_REQUIRED_FUNC)

    GL_FUNCTIONS_4_3_OPTIONAL(LOAD_OPTIONAL_FUNC)
    GL_FUNCTIONS_4_6_OPTIONAL(LOAD_OPTIONAL_FUNC)
    GL_FUNCTIONS_EXT_BINDLESS(LOAD_OPTIONAL_FUNC)

    #undef LOAD_REQUIRED_FUNC
    #undef LOAD_OPTIONAL_FUNC

    Py_DECREF(loader_function);

    // Report missing required functions
    if (PyList_Size(missing) > 0) {
        PyErr_Format(PyExc_RuntimeError, 
                     "Failed to load required OpenGL functions: %R", missing);
        Py_DECREF(missing);
        return -1;
    }

    Py_DECREF(missing);
    return 0; // Success
}

#else

// No-op for external linking scenarios
static int load_gl([[maybe_unused]] CaravanState* state, [[maybe_unused]] PyObject* loader) {
    return 0; 
}

#endif