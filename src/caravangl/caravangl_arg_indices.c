#include "caravangl_arg_indices.h"
#include <string.h>

// --- 1. THE SAFETY ENGINE ---

// Expanded for GL types: map primitives to standard Python type objects
#define GET_TYPE_GUARD(T)                                                                          \
    _Generic((T),                                                                                  \
        bool: &PyBool_Type,                                                                        \
        int: &PyLong_Type,                                                                         \
        uint32_t: &PyLong_Type,                                                                    \
        Py_ssize_t: &PyLong_Type,                                                                  \
        default: (PyTypeObject *)nullptr)

// Now outputs a FastArgDef (the translation struct)
#define GEN_SPEC(ID, NAME, TYPE, REQ)                                                              \
    [ID] = {.name = (NAME),                                                                        \
            .type_name = FP_GET_TYPE_NAME((TYPE){}),                                               \
            .required = (bool)(REQ),                                                               \
            .type_guard = GET_TYPE_GUARD((TYPE){}),                                                \
            .convert = FP_GET_CONVERTER((TYPE){})},

/**
 * SETUP_PARSER
 * 1. Creates a FastArgDef array on the stack.
 * 2. Calls fp_init_impl which mallocs the Hot/Cold arrays internally.
 */
#define SETUP_PARSER(cp, P, G, S)                                                                  \
    do {                                                                                           \
        FastArgDef temp[] = {S(GEN_SPEC)};                                                         \
        /* Pass the stack-allocated defs to the heap-allocating init */                            \
        fp_init_impl(&(cp)->P##Parser, temp, G##_COUNT);                                           \
        (cp)->P##Parser.parser_name = #P;                                                          \
    } while (false)

// --- 2. PUBLIC API ---

void caravan_init_parsers(CaravanParsers *commonptr) {
    commonptr->registry_count = 0; // If you use a registry
#define DO_SETUP(P, G, S) SETUP_PARSER(commonptr, P, G, S);
    FOR_ALL_PARSERS(DO_SETUP)
#undef DO_SETUP
}

void caravan_free_parsers(CaravanParsers *commonptr) {
#define DO_FREE(P, G, S) fp_deinit(&(commonptr)->P##Parser);
    FOR_ALL_PARSERS(DO_FREE)
#undef DO_FREE
}