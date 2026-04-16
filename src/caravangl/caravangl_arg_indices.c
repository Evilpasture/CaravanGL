#include "caravangl_arg_indices.h"
#include <string.h>

#define GEN_SPEC(ID, NAME, TYPE, REQ)                                                              \
    [ID] = {.name = (NAME),                                                                        \
            .type_name = #TYPE,                                                                    \
            .required = (bool)(REQ),                                                               \
            .convert = FP_GET_CONVERTER((TYPE){0})},

#define SETUP_PARSER(cp, P, G, S)                                                                  \
    do {                                                                                           \
        FastArgSpec temp[] = {S(GEN_SPEC)};                                                        \
        memcpy((cp)->P##Specs, temp, sizeof(temp));                                                \
        fp_init_impl(&(cp)->P##Parser, (cp)->P##Specs, G##_COUNT);                                 \
        (cp)->P##Parser.parser_name = #P;                                                          \
    } while (false)

void caravan_init_parsers(CaravanParsers *cp) {
#define DO_SETUP(P, G, S) SETUP_PARSER(cp, P, G, S);
    FOR_ALL_PARSERS(DO_SETUP)
#undef DO_SETUP
}

void caravan_free_parsers(CaravanParsers *cp) {
#define DO_FREE(P, G, S) fp_deinit(&(cp)->P##Parser);
    FOR_ALL_PARSERS(DO_FREE)
#undef DO_FREE
}