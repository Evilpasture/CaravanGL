#pragma once
#include <Python.h>
#include "caravangl_specs.h"

typedef struct CaravanState {
    PyTypeObject* BufferType;
    PyObject* CaravanError;

    CaravanContext ctx;

    // The isolated function table
    struct {
        #define GL_PTR_GEN(ret, name, ...) ret (GL_API *name)(__VA_ARGS__);
        
        // Core Versions
        GL_FUNCTIONS_3_3_CORE(GL_PTR_GEN)
        GL_FUNCTIONS_4_2_CORE(GL_PTR_GEN)
        GL_FUNCTIONS_4_3_CORE(GL_PTR_GEN)
        GL_FUNCTIONS_4_4_CORE(GL_PTR_GEN)
        
        // Optional / Extensions
        GL_FUNCTIONS_4_3_OPTIONAL(GL_PTR_GEN)
        GL_FUNCTIONS_4_6_OPTIONAL(GL_PTR_GEN)
        GL_FUNCTIONS_EXT_BINDLESS(GL_PTR_GEN)
        
        #undef GL_PTR_GEN
    } gl;
} CaravanState;

[[maybe_unused]]
static inline CaravanState *get_caravan_state(PyObject *m) {
    return (CaravanState *)PyModule_GetState(m);
}