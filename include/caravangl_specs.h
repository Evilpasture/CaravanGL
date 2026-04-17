#pragma once
#include "caravangl_core.h"
#include "mag_mutex.h"

// -----------------------------------------------------------------------------
// Core Objects
// -----------------------------------------------------------------------------

/**
 * CaravanBuffer: Represents linear memory (VBO, IBO, UBO, SSBO).
 * Using GLsizeiptr ensures we can handle buffers > 2GB on 64-bit systems.
 */
typedef struct CaravanBuffer {
    GLuint id;
    GLenum target;     // GL_ARRAY_BUFFER, GL_UNIFORM_BUFFER, etc.
    GLsizeiptr size;   // Total size in bytes
    GLenum usage;      // GL_STATIC_DRAW, GL_DYNAMIC_STORAGE_BIT, etc.
    bool is_immutable; // Was this created with glBufferStorage?
} CaravanBuffer;

/**
 * CaravanTexture: Represents spatial data (2D, 3D, Cube, etc.)
 */
typedef struct CaravanTexture {
    GLuint id;
    GLenum target;          // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc.
    GLenum format;          // GL_RGBA, GL_DEPTH_COMPONENT, etc.
    GLenum internal_format; // GL_RGBA8, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, etc.
    GLenum type;            // GL_UNSIGNED_BYTE, GL_FLOAT, etc.

    // Anonymous struct to group dimensions
    struct {
        GLsizei width;
        GLsizei height;
        GLsizei depth;
    };

    GLint levels;  // Mipmap count
    GLint samples; // For MSAA textures (0 or 1 for standard)

    // Bindless Support (Ready for the ARB extensions we added to the loader)
    GLuint64 handle;
    bool is_resident;
} CaravanTexture;

/**
 * CaravanRenderbuffer: Offscreen storage for depth/stencil or MSAA color.
 */
typedef struct CaravanRenderbuffer {
    GLuint id;
    GLenum internal_format;
    GLsizei width;
    GLsizei height;
    GLsizei samples;
} CaravanRenderbuffer;

/**
 * CaravanFramebuffer: A collection of attachments for rendering.
 */
typedef struct CaravanFramebuffer {
    GLuint id;
    GLsizei width;
    GLsizei height;

    // Metadata to help the engine validate state
    int color_attachments_count;
    bool has_depth;
    bool has_stencil;
} CaravanFramebuffer;

typedef struct CaravanRect {
    GLint x, y;
    GLsizei width, height;
} CaravanRect;

// -----------------------------------------------------------------------------
// Context & State Tracking
// -----------------------------------------------------------------------------

static constexpr auto CARAVAN_MAX_TEXTURE_UNITS = 32;
static constexpr auto CARAVAN_MAX_UBO_BINDINGS = 16;
static constexpr auto CARAVAN_MAX_SSBO_BINDINGS = 16;

// Struct to track bounded buffer ranges (UBOs/SSBOs)
typedef struct CaravanBufferBinding {
    GLuint id;
    GLintptr offset;
    GLsizeiptr size;
} CaravanBufferBinding;

// Struct to track bound textures per unit
typedef struct CaravanTextureBinding {
    GLuint target;
    GLuint id;
    GLuint sampler_id;
} CaravanTextureBinding;

// Struct to track render pipeline state
typedef struct CaravanRenderState {
    bool cull_face_enabled;
    GLenum cull_face_mode;
    GLenum front_face;

    // Depth
    bool depth_test_enabled;
    GLenum depth_func;
    bool depth_write_mask;

    // Blend
    bool blend_enabled;
    GLenum blend_src_rgb, blend_dst_rgb;
    GLenum blend_src_alpha, blend_dst_alpha;
    GLenum blend_eq_rgb, blend_eq_alpha;

    // Stencil (NEW)
    bool stencil_test_enabled;
    GLenum stencil_func;
    GLint stencil_ref;
    GLuint stencil_read_mask;
    GLuint stencil_write_mask;
    GLenum stencil_fail_op;
    GLenum stencil_zfail_op;
    GLenum stencil_zpass_op;
} CaravanRenderState;

typedef struct CaravanContext {
    MagMutex state_lock;
    struct {
        GLuint vao;
        GLuint program;
        GLuint fbo_read;
        GLuint fbo_draw;

        // Track the active texture unit to avoid redundant glActiveTexture calls
        GLuint active_texture_unit;

        // Use engine-defined maximums for arrays
        CaravanBufferBinding ubo[CARAVAN_MAX_UBO_BINDINGS];
        CaravanBufferBinding ssbo[CARAVAN_MAX_SSBO_BINDINGS];
        CaravanTextureBinding texture_units[CARAVAN_MAX_TEXTURE_UNITS];

        CaravanRenderState render;
        GLsync last_work_fence; // For GPU/CPU sync

        // Basic Render State Caching (saves performance)
        bool depth_test_enabled;
        bool blend_enabled;
        bool cull_face_enabled;
    } bound;

    CaravanRect viewport;

    struct {
        // Basic limits
        GLint max_texture_size;
        GLint max_3d_texture_size;
        GLint max_array_texture_layers;
        GLint max_samples;

        // Binding limits (queried from hardware)
        GLint max_texture_units;
        GLint max_ubo_bindings;

        // Buffer size limits
        GLint max_uniform_block_size;
        GLint max_shader_storage_block_size;

        // FBO limits
        GLint max_color_attachments;

        // Compute limits
        GLint max_compute_work_group_invocations;

        // Feature support
        bool support_bindless;
        bool support_compute;
        bool support_anisotropy;
    } caps;

    GLenum last_error;
} CaravanContext;

typedef struct CaravanAttribute {
    GLint location;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLsizeiptr offset;
} CaravanAttribute;

typedef struct CaravanVertexLayout {
    CaravanAttribute attrs[16]; // Max typical attributes
    int attr_count;
} CaravanVertexLayout;

typedef struct CaravanProgram {
    GLuint id;
    // Map of uniform names to locations
    // In a real engine, you'd use a hash map here.
    // For now, let's assume you store the common ones.
    struct {
        GLint model;
        GLint view;
        GLint projection;
    } common_locs;
} CaravanProgram;

typedef struct CaravanMesh {
    GLuint vao;
    CaravanBuffer vbo;
    CaravanBuffer ibo; // Index Buffer
    GLsizei count;     // Number of vertices/indices
    GLenum mode;       // GL_TRIANGLES, GL_LINES, etc.
    GLenum index_type; // GL_UNSIGNED_INT or GL_UNSIGNED_SHORT
} CaravanMesh;

// Draw parameters that can be mutated via MemoryView without Python overhead
typedef struct CaravanDrawParams {
    GLuint vertex_count;
    GLuint instance_count;
    GLuint first_vertex;
    GLuint base_instance;
} CaravanDrawParams;

static constexpr size_t CARAVAN_GARBAGE_SIZE = 256;

typedef struct CaravanGarbage {
    // Objects deleted via glDelete*(count, ids)
    GLuint buffers[CARAVAN_GARBAGE_SIZE];
    size_t buffer_count;

    GLuint textures[CARAVAN_GARBAGE_SIZE];
    size_t texture_count;

    GLuint vaos[CARAVAN_GARBAGE_SIZE];
    size_t vao_count;

    GLuint fbos[CARAVAN_GARBAGE_SIZE];
    size_t fbo_count;

    GLuint rbos[CARAVAN_GARBAGE_SIZE];
    size_t rbo_count;

    GLuint samplers[CARAVAN_GARBAGE_SIZE];
    size_t sampler_count;

    // Objects deleted via glDelete*(id)
    GLuint programs[CARAVAN_GARBAGE_SIZE];
    size_t program_count;
} CaravanGarbage;