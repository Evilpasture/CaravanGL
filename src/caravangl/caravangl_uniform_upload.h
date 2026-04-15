#pragma once
#include "caravangl_uniforms.h"

// Define the binary layout of a uniform instruction
typedef struct CaravanUniformBinding {
    uint8_t function_id; // Maps to UF_COUNT (e.g., UF_MAT4)
    GLint location;
    GLsizei count;
    uint32_t offset;     // Byte offset inside the data payload
} CaravanUniformBinding;

typedef struct CaravanUniformHeader {
    uint32_t count;
    CaravanUniformBinding bindings[];
} CaravanUniformHeader;

/**
 * Executes a batch of uniform uploads using the function pointer table.
 */
[[gnu::always_inline, gnu::hot]]
static inline void cv_upload_uniform_batch(CaravanState *s, 
                                           const void *header_buffer, 
                                           const void *data_payload) 
{
    const CaravanUniformHeader *header = (const CaravanUniformHeader *)header_buffer;
    const char *data = (const char *)data_payload;

    for (uint32_t i = 0; i < header->count; ++i) {
        const CaravanUniformBinding *b = &header->bindings[i];
        const void *ptr = data + b->offset;

        if (b->function_id < UF_COUNT) [[clang::likely]] {
            UniformUploadFn func = uniform_upload_table[b->function_id];
            if (func != nullptr) [[clang::likely]] {
                func(s, b->location, b->count, ptr);
            }
        } else {
#ifndef NDEBUG
            fprintf(stderr, "[CaravanGL] Invalid uniform function ID: %d\n", b->function_id);
#endif
        }
    }
}