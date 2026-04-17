#include "caravangl_arg_indices.h"
#include <string.h>

// Helper to map C types to Python Type Objects for guarding
#define GET_TYPE_GUARD(T) _Generic((T), bool: &PyBool_Type, default: (PyTypeObject *)nullptr)

#define GEN_SPEC(ID, NAME, TYPE, REQ)                                                              \
    [ID] = {.name = (NAME),                                                                        \
            .type_name = #TYPE,                                                                    \
            .required = (bool)(REQ),                                                               \
            .type_guard = GET_TYPE_GUARD((TYPE){0}), /* Pass the guard here */                     \
            .convert = FP_GET_CONVERTER((TYPE){0})},

// Only works if the types and sizes match exactly
#define SETUP_PARSER(cp, P, G, S)                                             \
    do {                                                                      \
        FastArgSpec temp[] = {S(GEN_SPEC)};                                   \
        _Pragma("unroll") for (size_t i = 0; i < sizeof(temp)/sizeof(temp[0]); ++i) {           \
            (cp)->P##Specs[i] = temp[i];                                      \
        }                                                                     \
        fp_init_impl(&(cp)->P##Parser, (cp)->P##Specs, G##_COUNT);            \
        (cp)->P##Parser.parser_name = #P;                                     \
    } while (false)

void caravan_init_parsers(CaravanParsers *commonptr) {
#define DO_SETUP(P, G, S) SETUP_PARSER(commonptr, P, G, S);
    FOR_ALL_PARSERS(DO_SETUP)
#undef DO_SETUP
}

void caravan_free_parsers(CaravanParsers *commonptr) {
#define DO_FREE(P, G, S) fp_deinit(&(commonptr)->P##Parser);
    FOR_ALL_PARSERS(DO_FREE)
#undef DO_FREE
}