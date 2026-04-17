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
static inline void cv_upload_uniform_batch(const CaravanGLTable *const OpenGL,
                                           CaravanUniformSource src) {
    const CaravanUniformHeader *header = src.header;
    const char *data = (const char *)src.payload;
#pragma unroll 4
    for (uint32_t i = 0; i < header->count; ++i) {
        const CaravanUniformBinding *binding = &header->bindings[i];
        if (binding->location == -1) {
            continue;
        }
        const void *ptr = data + binding->offset;

        UniformUploadFn func = uniform_upload_table[binding->function_id];
        if (func) {
            func(OpenGL, binding->location, binding->count, ptr);
        }
    }
}