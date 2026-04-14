#pragma once
#include "caravangl_core.h"

// -----------------------------------------------------------------------------
// Core Objects
// -----------------------------------------------------------------------------

/**
 * CaravanBuffer: Represents linear memory (VBO, IBO, UBO, SSBO).
 * Using GLsizeiptr ensures we can handle buffers > 2GB on 64-bit systems.
 */
typedef struct CaravanBuffer {
    GLuint id;
    GLenum target;      // GL_ARRAY_BUFFER, GL_UNIFORM_BUFFER, etc.
    GLsizeiptr size;    // Total size in bytes
    GLenum usage;       // GL_STATIC_DRAW, GL_DYNAMIC_STORAGE_BIT, etc.
    bool is_immutable;  // Was this created with glBufferStorage?
} CaravanBuffer;

/**
 * CaravanTexture: Represents spatial data (2D, 3D, Cube, etc.)
 */
typedef struct CaravanTexture {
    GLuint id;
    GLenum target;      // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc.
    GLenum format;      // GL_RGBA, GL_DEPTH_COMPONENT, etc.
    GLenum internal_format; // GL_RGBA8, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, etc.
    GLenum type;        // GL_UNSIGNED_BYTE, GL_FLOAT, etc.
    
    // Anonymous struct to group dimensions
    struct {
        GLsizei width;
        GLsizei height;
        GLsizei depth;
    };

    GLint levels;       // Mipmap count
    GLint samples;      // For MSAA textures (0 or 1 for standard)
    
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

// -----------------------------------------------------------------------------
// Context & State Tracking
// -----------------------------------------------------------------------------

/**
 * CaravanGLContext: The "Brain" of the renderer.
 * Keeps track of currently bound state to avoid redundant GL calls (State Sorting).
 */
typedef struct CaravanContext {
    struct {
        GLuint vao;
        GLuint program;
        GLuint fbo_read;
        GLuint fbo_draw;
        
        GLuint ubo[GL_MAX_UNIFORM_BUFFER_BINDINGS];
        GLuint texture_units[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS]; 
    } bound;

    struct {
        GLint x, y;
        GLsizei w, h;
    } viewport;

    struct {
        GLint max_texture_size;
        GLint max_samples;
        GLint max_uniform_block_size;
        bool support_bindless;
        bool support_compute;
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
    GLenum mode;      // GL_TRIANGLES, GL_LINES, etc.
    GLenum index_type; // GL_UNSIGNED_INT or GL_UNSIGNED_SHORT
} CaravanMesh;