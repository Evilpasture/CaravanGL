#pragma once
#include "caravangl_uniforms.h"

// Define the binary layout of a uniform instruction
typedef struct CaravanUniformBinding {
    uint8_t function_id; // Maps to UF_COUNT (e.g., UF_MAT4)
    GLint location;
    GLsizei count;
    uint32_t offset; // Byte offset inside the data payload
} CaravanUniformBinding;

typedef struct CaravanUniformHeader {
    uint32_t count;
    CaravanUniformBinding bindings[];
} CaravanUniformHeader;

typedef struct {
    const CaravanUniformHeader *header;
    const void *payload;
} CaravanUniformSource;

/**
 * Executes a batch of uniform uploads using the function pointer table.
 */
[[gnu::always_inline, gnu::hot]]
static inline void cv_upload_uniform_batch(CaravanState *state, CaravanUniformSource src) {
    const CaravanUniformHeader *header = src.header;
    const char *data = (const char *)src.payload;
#pragma unroll 4
    for (uint32_t i = 0; i < header->count; ++i) {
        const CaravanUniformBinding *binding = &header->bindings[i];
        const void *ptr = data + binding->offset;

        if (binding->function_id < UF_COUNT) [[clang::likely]] {
            UniformUploadFn func = uniform_upload_table[binding->function_id];
            if (func != nullptr) [[clang::likely]] {
                func(state, binding->location, binding->count, ptr);
            }
        } else {
#ifndef NDEBUG
            if (fprintf(stderr, "[CaravanGL] Invalid uniform function ID: %d\n",
                        binding->function_id) < 0) {
                // the world has ended
            }
#endif
        }
    }
}