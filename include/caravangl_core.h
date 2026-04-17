#pragma once
#include <stddef.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// Standard GL Types
// -----------------------------------------------------------------------------
typedef uint8_t GLboolean;
typedef uint32_t GLbitfield;
typedef uint32_t GLenum;
typedef uint32_t GLuint;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef void GLvoid;
typedef char GLchar;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef int8_t GLbyte;
typedef uint8_t GLubyte;
typedef int16_t GLshort;
typedef uint16_t GLushort;
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef intptr_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef uint16_t GLhalf;
typedef uint64_t GLuint64EXT;
typedef int64_t GLint64EXT;

// NOLINTNEXTLINE(bugprone-reserved-identifier, cert-dcl37-*)
typedef struct __GLsync *GLsync;

// -----------------------------------------------------------------------------
// OpenGL Constants — Strictly per Khronos OpenGL->xml / OpenGL 4.6 Core
// -----------------------------------------------------------------------------

// --- Boolean & Generic Sentinels ---
[[maybe_unused]] static constexpr GLboolean GL_FALSE = 0;
[[maybe_unused]] static constexpr GLboolean GL_TRUE = 1;
[[maybe_unused]] static constexpr GLuint GL_INVALID_INDEX = 0xFFFFFFFFU;
[[maybe_unused]] static constexpr GLuint64 GL_TIMEOUT_IGNORED = 0xFFFFFFFFFFFFFFFFULL;
[[maybe_unused]] static constexpr GLuint GL_INVALID_ENUM_VALUE =
    0xFFFFFFFFU; // returned by GetProgramResourceIndex for not-found

// --- Hint ---
typedef enum GLHint : GLenum {
    GL_DONT_CARE = 0x1100,
} GLHint;

// --- Data Types ---
typedef enum GLDataType : GLenum {
    GL_BYTE = 0x1400,
    GL_UNSIGNED_BYTE = 0x1401,
    GL_SHORT = 0x1402,
    GL_UNSIGNED_SHORT = 0x1403,
    GL_INT = 0x1404,
    GL_UNSIGNED_INT = 0x1405,
    GL_FLOAT = 0x1406,
    GL_DOUBLE = 0x140A,
    GL_HALF_FLOAT = 0x140B,
    GL_FIXED = 0x140C,
    GL_INT_2_10_10_10_REV = 0x8D9F,
    GL_UNSIGNED_INT_2_10_10_10_REV = 0x8368,
    GL_UNSIGNED_INT_10F_11F_11F_REV = 0x8C3B,
    GL_UNSIGNED_BYTE_3_3_2 = 0x8032,
    GL_UNSIGNED_SHORT_4_4_4_4 = 0x8033,
    GL_UNSIGNED_SHORT_5_5_5_1 = 0x8034,
    GL_UNSIGNED_INT_8_8_8_8 = 0x8035,
    GL_UNSIGNED_INT_10_10_10_2 = 0x8036,
    GL_UNSIGNED_BYTE_2_3_3_REV = 0x8362,
    GL_UNSIGNED_SHORT_5_6_5 = 0x8363,
    GL_UNSIGNED_SHORT_5_6_5_REV = 0x8364,
    GL_UNSIGNED_SHORT_4_4_4_4_REV = 0x8365,
    GL_UNSIGNED_SHORT_1_5_5_5_REV = 0x8366,
    GL_UNSIGNED_INT_8_8_8_8_REV = 0x8367,
    GL_UNSIGNED_INT_24_8 = 0x84FA,
    GL_FLOAT_32_UNSIGNED_INT_24_8_REV = 0x8DAD,
} GLDataType;

// --- Primitives ---
typedef enum GLPrimitive : GLenum {
    GL_POINTS = 0x0000,
    GL_LINES = 0x0001,
    GL_LINE_LOOP = 0x0002,
    GL_LINE_STRIP = 0x0003,
    GL_TRIANGLES = 0x0004,
    GL_TRIANGLE_STRIP = 0x0005,
    GL_TRIANGLE_FAN = 0x0006,
    GL_LINES_ADJACENCY = 0x000A,
    GL_LINE_STRIP_ADJACENCY = 0x000B,
    GL_TRIANGLES_ADJACENCY = 0x000C,
    GL_TRIANGLE_STRIP_ADJACENCY = 0x000D,
    GL_PATCHES = 0x000E,
} GLPrimitive;

// --- Blending ---
typedef enum GLBlendFactor : GLenum {
    GL_ZERO = 0,
    GL_ONE = 1,
    GL_SRC_COLOR = 0x0300,
    GL_ONE_MINUS_SRC_COLOR = 0x0301,
    GL_SRC_ALPHA = 0x0302,
    GL_ONE_MINUS_SRC_ALPHA = 0x0303,
    GL_DST_ALPHA = 0x0304,
    GL_ONE_MINUS_DST_ALPHA = 0x0305,
    GL_DST_COLOR = 0x0306,
    GL_ONE_MINUS_DST_COLOR = 0x0307,
    GL_SRC_ALPHA_SATURATE = 0x0308,
    GL_CONSTANT_COLOR = 0x8001,
    GL_ONE_MINUS_CONSTANT_COLOR = 0x8002,
    GL_CONSTANT_ALPHA = 0x8003,
    GL_ONE_MINUS_CONSTANT_ALPHA = 0x8004,
    GL_SRC1_COLOR = 0x88F9,
    GL_ONE_MINUS_SRC1_COLOR = 0x88FA,
    GL_SRC1_ALPHA = 0x8589,
    GL_ONE_MINUS_SRC1_ALPHA = 0x88FB,
} GLBlendFactor;

typedef enum GLBlendEquation : GLenum {
    GL_FUNC_ADD = 0x8006,
    GL_FUNC_SUBTRACT = 0x800A,
    GL_FUNC_REVERSE_SUBTRACT = 0x800B,
    GL_MIN = 0x8007,
    GL_MAX = 0x8008,
} GLBlendEquation;

// --- Clearing ---
typedef enum GLClearMask : uint32_t {
    GL_DEPTH_BUFFER_BIT = 0x00000100,
    GL_STENCIL_BUFFER_BIT = 0x00000400,
    GL_COLOR_BUFFER_BIT = 0x00004000,
} GLClearMask;

typedef enum GLClearBufferTarget : GLenum {
    GL_COLOR = 0x1800,
    GL_DEPTH = 0x1801,
    GL_STENCIL = 0x1802,
} GLClearBufferTarget;

// --- Errors ---
typedef enum GLError : GLenum {
    GL_NO_ERROR = 0,
    GL_INVALID_ENUM = 0x0500,
    GL_INVALID_VALUE = 0x0501,
    GL_INVALID_OPERATION = 0x0502,
    GL_STACK_OVERFLOW = 0x0503,
    GL_STACK_UNDERFLOW = 0x0504,
    GL_OUT_OF_MEMORY = 0x0505,
    GL_INVALID_FRAMEBUFFER_OPERATION = 0x0506,
    GL_CONTEXT_LOST = 0x0507,
} GLError;

// --- String Queries ---
typedef enum GLStringName : GLenum {
    GL_VENDOR = 0x1F00,
    GL_RENDERER = 0x1F01,
    GL_VERSION = 0x1F02,
    GL_EXTENSIONS = 0x1F03,
    GL_SHADING_LANGUAGE_VERSION = 0x8B8C,
} GLStringName;

// --- Face, Winding, and Polygon Mode ---
typedef enum GLFrontFaceDirection : GLenum {
    GL_CW = 0x0900,
    GL_CCW = 0x0901,
} GLFrontFaceDirection;

typedef enum GLFace : GLenum {
    GL_FRONT = 0x0404,
    GL_BACK = 0x0405,
    GL_FRONT_AND_BACK = 0x0408,
} GLFace;

typedef enum GLPolygonMode : GLenum {
    GL_POINT = 0x1B00,
    GL_LINE = 0x1B01,
    GL_FILL = 0x1B02,
} GLPolygonMode;

// --- Compare Functions (depth, stencil, shadow samplers) ---
typedef enum GLCompareFunction : GLenum {
    GL_NEVER = 0x0200,
    GL_LESS = 0x0201,
    GL_EQUAL = 0x0202,
    GL_LEQUAL = 0x0203,
    GL_GREATER = 0x0204,
    GL_NOTEQUAL = 0x0205,
    GL_GEQUAL = 0x0206,
    GL_ALWAYS = 0x0207,
} GLCompareFunction;

// --- Stencil Operations ---
typedef enum GLStencilOp : GLenum {
    GL_KEEP = 0x1E00,
    GL_REPLACE = 0x1E01,
    GL_INCR = 0x1E02,
    GL_DECR = 0x1E03,
    GL_INVERT = 0x150A,
    GL_INCR_WRAP = 0x8507,
    GL_DECR_WRAP = 0x8508,
} GLStencilOp;

// --- Capabilities ---
typedef enum GLCapability : GLenum {
    GL_CULL_FACE = 0x0B44,
    GL_DEPTH_TEST = 0x0B71,
    GL_STENCIL_TEST = 0x0B90,
    GL_BLEND = 0x0BE2,
    GL_SCISSOR_TEST = 0x0C11,
    GL_COLOR_LOGIC_OP = 0x0BF2,
    GL_DITHER = 0x0BD0,
    GL_LINE_SMOOTH = 0x0B20,
    GL_POLYGON_SMOOTH = 0x0B41,
    GL_POLYGON_OFFSET_POINT = 0x2A01,
    GL_POLYGON_OFFSET_LINE = 0x2A02,
    GL_POLYGON_OFFSET_FILL = 0x8037,
    GL_MULTISAMPLE = 0x809D,
    GL_SAMPLE_ALPHA_TO_COVERAGE = 0x809E,
    GL_SAMPLE_ALPHA_TO_ONE = 0x809F,
    GL_SAMPLE_COVERAGE = 0x80A0,
    GL_SAMPLE_SHADING = 0x8C36,
    GL_SAMPLE_MASK = 0x8E51,
    GL_PRIMITIVE_RESTART = 0x8F9D,
    GL_PRIMITIVE_RESTART_FIXED_INDEX = 0x8D69,
    GL_RASTERIZER_DISCARD = 0x8C89,
    GL_FRAMEBUFFER_SRGB = 0x8DB9,
    GL_DEPTH_CLAMP = 0x864F,
    GL_TEXTURE_CUBE_MAP_SEAMLESS = 0x884F,
    GL_PROGRAM_POINT_SIZE = 0x8642,
    GL_DEBUG_OUTPUT = 0x92E0,
    GL_DEBUG_OUTPUT_SYNCHRONOUS = 0x8242,
} GLCapability;

// --- State Queries ---
typedef enum GLGetParameter : GLenum {
    GL_VIEWPORT = 0x0BA2,
    GL_DEPTH_RANGE = 0x0B70,
    GL_DEPTH_WRITEMASK = 0x0B72,
    GL_DEPTH_CLEAR_VALUE = 0x0B73,
    GL_DEPTH_FUNC = 0x0B74,
    GL_STENCIL_CLEAR_VALUE = 0x0B91,
    GL_STENCIL_FUNC = 0x0B92,
    GL_STENCIL_VALUE_MASK = 0x0B93,
    GL_STENCIL_FAIL = 0x0B94,
    GL_STENCIL_PASS_DEPTH_FAIL = 0x0B95,
    GL_STENCIL_PASS_DEPTH_PASS = 0x0B96,
    GL_STENCIL_REF = 0x0B97,
    GL_STENCIL_WRITEMASK = 0x0B98,
    GL_SCISSOR_BOX = 0x0C10,
    GL_COLOR_CLEAR_VALUE = 0x0C22,
    GL_COLOR_WRITEMASK = 0x0C23,

    GL_MAX_TEXTURE_SIZE = 0x0D33,
    GL_MAX_VIEWPORT_DIMS = 0x0D3A,
    GL_SUBPIXEL_BITS = 0x0D50,
    GL_MAX_ELEMENTS_VERTICES = 0x80E8,
    GL_MAX_ELEMENTS_INDICES = 0x80E9,
    GL_MAX_3D_TEXTURE_SIZE = 0x8073,
    GL_MAX_DRAW_BUFFERS = 0x8824,
    GL_MAX_VERTEX_ATTRIBS = 0x8869,
    GL_MAX_TEXTURE_IMAGE_UNITS = 0x8872,
    GL_MAX_FRAGMENT_UNIFORM_COMPONENTS = 0x8B49,
    GL_MAX_VERTEX_UNIFORM_COMPONENTS = 0x8B4A,
    GL_MAX_VARYING_FLOATS = 0x8B4B,
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = 0x8B4C,
    GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = 0x8B4D,
    GL_MAX_RENDERBUFFER_SIZE = 0x84E8,
    GL_MAX_COLOR_ATTACHMENTS = 0x8CDF,
    GL_MAX_SAMPLES = 0x8D57,
    GL_MAX_RECTANGLE_TEXTURE_SIZE = 0x84F8,
    GL_MAX_TEXTURE_LOD_BIAS = 0x84FD,
    GL_MAX_CUBE_MAP_TEXTURE_SIZE = 0x851C,
    GL_MAX_ARRAY_TEXTURE_LAYERS = 0x88FF,
    GL_MAX_TEXTURE_BUFFER_SIZE = 0x8C2B,
    GL_MAX_UNIFORM_BUFFER_BINDINGS = 0x8A2F,
    GL_MAX_UNIFORM_BLOCK_SIZE = 0x8A30,
    GL_MAX_COMBINED_UNIFORM_BLOCKS = 0x8A2E,
    GL_MAX_VERTEX_UNIFORM_BLOCKS = 0x8A2B,
    GL_MAX_GEOMETRY_UNIFORM_BLOCKS = 0x8A2C,
    GL_MAX_FRAGMENT_UNIFORM_BLOCKS = 0x8A2D,
    GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS = 0x90DD,
    GL_MAX_SHADER_STORAGE_BLOCK_SIZE = 0x90DE,
    GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS = 0x90DB,
    GL_MAX_COMPUTE_UNIFORM_BLOCKS = 0x91BB,
    GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS = 0x91BC,
    GL_MAX_COMPUTE_IMAGE_UNIFORMS = 0x91BD,
    GL_MAX_COMPUTE_WORK_GROUP_COUNT = 0x91BE,
    GL_MAX_COMPUTE_WORK_GROUP_SIZE = 0x91BF,
    GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS = 0x90EB,
    GL_MAX_IMAGE_UNITS = 0x8F38,
    GL_MAX_VERTEX_IMAGE_UNIFORMS = 0x90CA,
    GL_MAX_FRAGMENT_IMAGE_UNIFORMS = 0x90CE,
    GL_MAX_COMBINED_IMAGE_UNIFORMS = 0x90CF,
    GL_MAX_GEOMETRY_IMAGE_UNIFORMS = 0x90CD,
    GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS = 0x90CB,
    GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS = 0x90CC,
    GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS = 0x90D6,
    GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS = 0x90DA,
    GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS = 0x90D7,
    GL_MAX_VERTEX_ATOMIC_COUNTERS = 0x92D2,
    GL_MAX_FRAGMENT_ATOMIC_COUNTERS = 0x92D6,
    GL_MAX_COMBINED_ATOMIC_COUNTERS = 0x92D7,
    GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE = 0x92D8,
    GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET = 0x82D9,
    GL_MAX_VERTEX_ATTRIB_BINDINGS = 0x82DA,
    GL_MAX_VERTEX_ATTRIB_STRIDE = 0x82E5,
    GL_MAX_DEBUG_MESSAGE_LENGTH = 0x9143,
    GL_MAX_DEBUG_LOGGED_MESSAGES = 0x9144,
    GL_MAX_DEBUG_GROUP_STACK_DEPTH = 0x826C,
    GL_MAX_LABEL_LENGTH = 0x82E8,
    GL_NUM_EXTENSIONS = 0x821D,
    GL_MAJOR_VERSION = 0x821B,
    GL_MINOR_VERSION = 0x821C,
    GL_CONTEXT_FLAGS = 0x821E,
    GL_CONTEXT_PROFILE_MASK = 0x9126,
    GL_CURRENT_PROGRAM = 0x8B8D,
    GL_DRAW_INDIRECT_BUFFER_BINDING = 0x8F43,
    GL_ACTIVE_ATOMIC_COUNTER_BUFFERS = 0x92D9,
} GLGetParameter;

// --- Context Flags ---
typedef enum GLContextFlag : GLenum {
    GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT = 0x00000001,
    GL_CONTEXT_FLAG_DEBUG_BIT = 0x00000002,
    GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT = 0x00000004,
    GL_CONTEXT_CORE_PROFILE_BIT = 0x00000001,
    GL_CONTEXT_COMPATIBILITY_PROFILE_BIT = 0x00000002,
} GLContextFlag;

// --- Texture Targets ---
typedef enum GLTextureTarget : GLenum {
    GL_TEXTURE_1D = 0x0DE0,
    GL_TEXTURE_2D = 0x0DE1,
    GL_TEXTURE_3D = 0x806F,
    GL_TEXTURE_1D_ARRAY = 0x8C18,
    GL_TEXTURE_2D_ARRAY = 0x8C1A,
    GL_TEXTURE_RECTANGLE = 0x84F5,
    GL_TEXTURE_CUBE_MAP = 0x8513,
    GL_TEXTURE_CUBE_MAP_ARRAY = 0x9009,
    GL_TEXTURE_2D_MULTISAMPLE = 0x9100,
    GL_TEXTURE_2D_MULTISAMPLE_ARRAY = 0x9102,
    GL_TEXTURE_BUFFER = 0x8C2A,
    GL_PROXY_TEXTURE_1D = 0x8063,
    GL_PROXY_TEXTURE_2D = 0x8064,
    GL_PROXY_TEXTURE_3D = 0x8070,
    GL_PROXY_TEXTURE_CUBE_MAP = 0x851B,
} GLTextureTarget;

typedef enum GLTextureCubeMapFace : GLenum {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X = 0x8515,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X = 0x8516,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y = 0x8517,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y = 0x8518,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z = 0x8519,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z = 0x851A,
} GLTextureCubeMapFace;

// --- Texture Parameters ---
typedef enum GLTextureParameter : GLenum {
    GL_TEXTURE_MIN_FILTER = 0x2801,
    GL_TEXTURE_MAG_FILTER = 0x2800,
    GL_TEXTURE_WRAP_S = 0x2802,
    GL_TEXTURE_WRAP_T = 0x2803,
    GL_TEXTURE_WRAP_R = 0x8072,
    GL_TEXTURE_BASE_LEVEL = 0x813C,
    GL_TEXTURE_MAX_LEVEL = 0x813D,
    GL_TEXTURE_MIN_LOD = 0x813A,
    GL_TEXTURE_MAX_LOD = 0x813B,
    GL_TEXTURE_LOD_BIAS = 0x8501,
    GL_TEXTURE_COMPARE_MODE = 0x884C,
    GL_TEXTURE_COMPARE_FUNC = 0x884D,
    GL_TEXTURE_SWIZZLE_R = 0x8E42,
    GL_TEXTURE_SWIZZLE_G = 0x8E43,
    GL_TEXTURE_SWIZZLE_B = 0x8E44,
    GL_TEXTURE_SWIZZLE_A = 0x8E45,
    GL_TEXTURE_SWIZZLE_RGBA = 0x8E46,
    GL_TEXTURE_BORDER_COLOR = 0x1004,
    GL_TEXTURE_MAX_ANISOTROPY = 0x84FE, // ARB_texture_filter_anisotropic (core 4.6)
    GL_DEPTH_STENCIL_TEXTURE_MODE = 0x90EA,
} GLTextureParameter;

typedef enum GLTextureFilter : GLenum {
    GL_NEAREST = 0x2600,
    GL_LINEAR = 0x2601,
    GL_NEAREST_MIPMAP_NEAREST = 0x2700,
    GL_LINEAR_MIPMAP_NEAREST = 0x2701,
    GL_NEAREST_MIPMAP_LINEAR = 0x2702,
    GL_LINEAR_MIPMAP_LINEAR = 0x2703,
} GLTextureFilter;

typedef enum GLTextureWrapMode : GLenum {
    GL_REPEAT = 0x2901,
    GL_CLAMP_TO_EDGE = 0x812F,
    GL_CLAMP_TO_BORDER = 0x812D,
    GL_MIRRORED_REPEAT = 0x8370,
    GL_MIRROR_CLAMP_TO_EDGE = 0x8743,
} GLTextureWrapMode;

typedef enum GLTextureCompareMode : GLenum {
    GL_COMPARE_REF_TO_TEXTURE = 0x884E,
    GL_TEXTURE_COMPARE_MODE_NONE = 0, // GL_NONE; no shadow comparison
} GLTextureCompareMode;

typedef enum GLTextureSwizzle : GLenum {
    GL_RED_COMPONENT = 0x1903,
    GL_GREEN = 0x1904,
    GL_BLUE = 0x1905,
    GL_ALPHA = 0x1906,
    // GL_ZERO and GL_ONE from GLBlendFactor are also valid swizzle values
} GLTextureSwizzle;

// --- Texture Units ---
typedef enum GLTextureUnit : GLenum {
    GL_TEXTURE0 = 0x84C0,
    GL_TEXTURE1 = 0x84C1,
    GL_TEXTURE2 = 0x84C2,
    GL_TEXTURE3 = 0x84C3,
    GL_TEXTURE4 = 0x84C4,
    GL_TEXTURE5 = 0x84C5,
    GL_TEXTURE6 = 0x84C6,
    GL_TEXTURE7 = 0x84C7,
    GL_TEXTURE8 = 0x84C8,
    GL_TEXTURE9 = 0x84C9,
    GL_TEXTURE10 = 0x84CA,
    GL_TEXTURE11 = 0x84CB,
    GL_TEXTURE12 = 0x84CC,
    GL_TEXTURE13 = 0x84CD,
    GL_TEXTURE14 = 0x84CE,
    GL_TEXTURE15 = 0x84CF,
    GL_TEXTURE16 = 0x84D0,
    GL_TEXTURE17 = 0x84D1,
    GL_TEXTURE18 = 0x84D2,
    GL_TEXTURE19 = 0x84D3,
    GL_TEXTURE20 = 0x84D4,
    GL_TEXTURE21 = 0x84D5,
    GL_TEXTURE22 = 0x84D6,
    GL_TEXTURE23 = 0x84D7,
    GL_TEXTURE24 = 0x84D8,
    GL_TEXTURE25 = 0x84D9,
    GL_TEXTURE26 = 0x84DA,
    GL_TEXTURE27 = 0x84DB,
    GL_TEXTURE28 = 0x84DC,
    GL_TEXTURE29 = 0x84DD,
    GL_TEXTURE30 = 0x84DE,
    GL_TEXTURE31 = 0x84DF,
} GLTextureUnit;

// --- Internal Formats (Sized Color) ---
typedef enum GLInternalFormat : GLenum {
    // Normalized unsigned
    GL_R8 = 0x8229,
    GL_R8_SNORM = 0x8F94,
    GL_R16 = 0x822A,
    GL_R16_SNORM = 0x8F98,
    GL_RG8 = 0x822B,
    GL_RG8_SNORM = 0x8F95,
    GL_RG16 = 0x822C,
    GL_RG16_SNORM = 0x8F99,
    GL_R3_G3_B2 = 0x2A10,
    GL_RGB4 = 0x804F,
    GL_RGB5 = 0x8050,
    GL_RGB8 = 0x8051,
    GL_RGB8_SNORM = 0x8F96,
    GL_RGB10 = 0x8052,
    GL_RGB12 = 0x8053,
    GL_RGB16 = 0x8054,
    GL_RGB16_SNORM = 0x8F9A,
    GL_RGBA2 = 0x8055,
    GL_RGBA4 = 0x8056,
    GL_RGB5_A1 = 0x8057,
    GL_RGBA8 = 0x8058,
    GL_RGBA8_SNORM = 0x8F97,
    GL_RGB10_A2 = 0x8059,
    GL_RGB10_A2UI = 0x906F,
    GL_RGBA12 = 0x805A,
    GL_RGBA16 = 0x805B,
    GL_RGBA16_SNORM = 0x8F9B,
    // sRGB
    GL_SRGB8 = 0x8C41,
    GL_SRGB8_ALPHA8 = 0x8C43,
    // Float
    GL_R16F = 0x822D,
    GL_RG16F = 0x822F,
    GL_RGB16F = 0x881B,
    GL_RGBA16F = 0x881A,
    GL_R32F = 0x822E,
    GL_RG32F = 0x8230,
    GL_RGB32F = 0x8815,
    GL_RGBA32F = 0x8814,
    GL_R11F_G11F_B10F = 0x8C3A,
    GL_RGB9_E5 = 0x8C3D,
    // Integer
    GL_R8I = 0x8231,
    GL_R8UI = 0x8232,
    GL_R16I = 0x8233,
    GL_R16UI = 0x8234,
    GL_R32I = 0x8235,
    GL_R32UI = 0x8236,
    GL_RG8I = 0x8237,
    GL_RG8UI = 0x8238,
    GL_RG16I = 0x8239,
    GL_RG16UI = 0x823A,
    GL_RG32I = 0x823B,
    GL_RG32UI = 0x823C,
    GL_RGB8I = 0x8D8F,
    GL_RGB8UI = 0x8D7D,
    GL_RGB16I = 0x8D89,
    GL_RGB16UI = 0x8D77,
    GL_RGB32I = 0x8D83,
    GL_RGB32UI = 0x8D71,
    GL_RGBA8I = 0x8D8E,
    GL_RGBA8UI = 0x8D7C,
    GL_RGBA16I = 0x8D88,
    GL_RGBA16UI = 0x8D76,
    GL_RGBA32I = 0x8D82,
    GL_RGBA32UI = 0x8D70,
    // Depth / Stencil
    GL_DEPTH_COMPONENT16 = 0x81A5,
    GL_DEPTH_COMPONENT24 = 0x81A6,
    GL_DEPTH_COMPONENT32 = 0x81A7,
    GL_DEPTH_COMPONENT32F = 0x8CAC,
    GL_DEPTH24_STENCIL8 = 0x88F0,
    GL_DEPTH32F_STENCIL8 = 0x8CAD,
    GL_STENCIL_INDEX1 = 0x8D46,
    GL_STENCIL_INDEX4 = 0x8D47,
    GL_STENCIL_INDEX8 = 0x8D48,
    GL_STENCIL_INDEX16 = 0x8D49,
    // Compressed
    GL_COMPRESSED_RED = 0x8225,
    GL_COMPRESSED_RG = 0x8226,
    GL_COMPRESSED_RGB = 0x84ED,
    GL_COMPRESSED_RGBA = 0x84EE,
    GL_COMPRESSED_SRGB = 0x8C48,
    GL_COMPRESSED_SRGB_ALPHA = 0x8C49,
    GL_COMPRESSED_RED_RGTC1 = 0x8DBB,
    GL_COMPRESSED_SIGNED_RED_RGTC1 = 0x8DBC,
    GL_COMPRESSED_RG_RGTC2 = 0x8DBD,
    GL_COMPRESSED_SIGNED_RG_RGTC2 = 0x8DBE,
    GL_COMPRESSED_RGBA_BPTC_UNORM = 0x8E8C,
    GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM = 0x8E8D,
    GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT = 0x8E8E,
    GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F,
} GLInternalFormat;

// --- Pixel Formats (transfer/upload) ---
typedef enum GLPixelFormat : GLenum {
    GL_RED = 0x1903,
    GL_RG = 0x8227,
    GL_RGB = 0x1907,
    GL_RGBA = 0x1908,
    GL_BGR = 0x80E0,
    GL_BGRA = 0x80E1,
    GL_RED_INTEGER = 0x8D94,
    GL_RG_INTEGER = 0x8228,
    GL_RGB_INTEGER = 0x8D98,
    GL_RGBA_INTEGER = 0x8D99,
    GL_BGR_INTEGER = 0x8D9A,
    GL_BGRA_INTEGER = 0x8D9B,
    GL_STENCIL_INDEX = 0x1901,
    GL_DEPTH_COMPONENT = 0x1902,
    GL_DEPTH_STENCIL = 0x84F9,
} GLPixelFormat;

// --- Pixel Store Parameters ---
typedef enum GLPixelStoreParameter : GLenum {
    GL_UNPACK_SWAP_BYTES = 0x0CF0,
    GL_UNPACK_LSB_FIRST = 0x0CF1,
    GL_UNPACK_ROW_LENGTH = 0x0CF2,
    GL_UNPACK_SKIP_ROWS = 0x0CF3,
    GL_UNPACK_SKIP_PIXELS = 0x0CF4,
    GL_UNPACK_ALIGNMENT = 0x0CF5,
    GL_UNPACK_IMAGE_HEIGHT = 0x806E,
    GL_UNPACK_SKIP_IMAGES = 0x806D,
    GL_PACK_SWAP_BYTES = 0x0D00,
    GL_PACK_LSB_FIRST = 0x0D01,
    GL_PACK_ROW_LENGTH = 0x0D02,
    GL_PACK_SKIP_ROWS = 0x0D03,
    GL_PACK_SKIP_PIXELS = 0x0D04,
    GL_PACK_ALIGNMENT = 0x0D05,
    GL_PACK_IMAGE_HEIGHT = 0x806C,
    GL_PACK_SKIP_IMAGES = 0x806B,
} GLPixelStoreParameter;

// --- Buffer Targets ---
typedef enum GLBufferTarget : GLenum {
    GL_ARRAY_BUFFER = 0x8892,
    GL_ELEMENT_ARRAY_BUFFER = 0x8893,
    GL_PIXEL_PACK_BUFFER = 0x88EB,
    GL_PIXEL_UNPACK_BUFFER = 0x88EC,
    GL_UNIFORM_BUFFER = 0x8A11,
    GL_TRANSFORM_FEEDBACK_BUFFER = 0x8C8E,
    GL_COPY_READ_BUFFER = 0x8F36,
    GL_COPY_WRITE_BUFFER = 0x8F37,
    GL_DRAW_INDIRECT_BUFFER = 0x8F3F,
    GL_SHADER_STORAGE_BUFFER = 0x90D2,
    GL_DISPATCH_INDIRECT_BUFFER = 0x90EE,
    GL_QUERY_BUFFER = 0x9192,
    GL_ATOMIC_COUNTER_BUFFER = 0x92C0,
} GLBufferTarget;

typedef enum GLBufferUsage : GLenum {
    GL_STREAM_DRAW = 0x88E0,
    GL_STREAM_READ = 0x88E1,
    GL_STREAM_COPY = 0x88E2,
    GL_STATIC_DRAW = 0x88E4,
    GL_STATIC_READ = 0x88E5,
    GL_STATIC_COPY = 0x88E6,
    GL_DYNAMIC_DRAW = 0x88E8,
    GL_DYNAMIC_READ = 0x88E9,
    GL_DYNAMIC_COPY = 0x88EA,
} GLBufferUsage;

typedef enum GLBufferStorageMapMask : uint32_t {
    GL_MAP_READ_BIT = 0x0001,
    GL_MAP_WRITE_BIT = 0x0002,
    GL_MAP_INVALIDATE_RANGE_BIT = 0x0004,
    GL_MAP_INVALIDATE_BUFFER_BIT = 0x0008,
    GL_MAP_FLUSH_EXPLICIT_BIT = 0x0010,
    GL_MAP_UNSYNCHRONIZED_BIT = 0x0020,
    GL_MAP_PERSISTENT_BIT = 0x0040,
    GL_MAP_COHERENT_BIT = 0x0080,
    GL_DYNAMIC_STORAGE_BIT = 0x0100,
    GL_CLIENT_STORAGE_BIT = 0x0200,
} GLBufferStorageMapMask;

// CaravanGL composite helper (non-spec)
[[maybe_unused]] static constexpr GLbitfield GL_PERSISTENT_WRITE_FLAGS =
    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

typedef enum GLBufferParameter : GLenum {
    GL_BUFFER_SIZE = 0x8764,
    GL_BUFFER_USAGE = 0x8765,
    GL_BUFFER_ACCESS = 0x88BB,
    GL_BUFFER_MAPPED = 0x88BC,
    GL_BUFFER_MAP_POINTER = 0x88BD,
    GL_BUFFER_ACCESS_FLAGS = 0x911F,
    GL_BUFFER_MAP_LENGTH = 0x9120,
    GL_BUFFER_MAP_OFFSET = 0x9121,
    GL_BUFFER_IMMUTABLE_STORAGE = 0x821F,
    GL_BUFFER_STORAGE_FLAGS = 0x8220,
} GLBufferParameter;

// --- Framebuffers ---
typedef enum GLFramebufferTarget : GLenum {
    GL_FRAMEBUFFER = 0x8D40,
    GL_READ_FRAMEBUFFER = 0x8CA8,
    GL_DRAW_FRAMEBUFFER = 0x8CA9,
} GLFramebufferTarget;

typedef enum GLRenderbufferTarget : GLenum {
    GL_RENDERBUFFER = 0x8D41,
} GLRenderbufferTarget;

typedef enum GLFramebufferAttachment : GLenum {
    GL_COLOR_ATTACHMENT0 = 0x8CE0,
    GL_COLOR_ATTACHMENT1 = 0x8CE1,
    GL_COLOR_ATTACHMENT2 = 0x8CE2,
    GL_COLOR_ATTACHMENT3 = 0x8CE3,
    GL_COLOR_ATTACHMENT4 = 0x8CE4,
    GL_COLOR_ATTACHMENT5 = 0x8CE5,
    GL_COLOR_ATTACHMENT6 = 0x8CE6,
    GL_COLOR_ATTACHMENT7 = 0x8CE7,
    GL_COLOR_ATTACHMENT8 = 0x8CE8,
    GL_COLOR_ATTACHMENT9 = 0x8CE9,
    GL_COLOR_ATTACHMENT10 = 0x8CEA,
    GL_COLOR_ATTACHMENT11 = 0x8CEB,
    GL_COLOR_ATTACHMENT12 = 0x8CEC,
    GL_COLOR_ATTACHMENT13 = 0x8CED,
    GL_COLOR_ATTACHMENT14 = 0x8CEE,
    GL_COLOR_ATTACHMENT15 = 0x8CEF,
    GL_DEPTH_ATTACHMENT = 0x8D00,
    GL_STENCIL_ATTACHMENT = 0x8D20,
    GL_DEPTH_STENCIL_ATTACHMENT = 0x821A,
} GLFramebufferAttachment;

typedef enum GLFramebufferStatus : GLenum {
    GL_FRAMEBUFFER_COMPLETE = 0x8CD5,
    GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT = 0x8CD6,
    GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7,
    GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER = 0x8CDB,
    GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER = 0x8CDC,
    GL_FRAMEBUFFER_UNSUPPORTED = 0x8CDD,
    GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE = 0x8D56,
    GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS = 0x8DA8,
    GL_FRAMEBUFFER_DEFAULT = 0x8218,
    GL_FRAMEBUFFER_UNDEFINED = 0x8219,
} GLFramebufferStatus;

typedef enum GLFramebufferAttachmentParameter : GLenum {
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE = 0x8CD0,
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME = 0x8CD1,
    GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL = 0x8CD2,
    GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE = 0x8CD3,
    GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER = 0x8CD4,
    GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING = 0x8210,
    GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE = 0x8211,
    GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE = 0x8212,
    GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE = 0x8213,
    GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE = 0x8214,
    GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE = 0x8215,
    GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE = 0x8216,
    GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE = 0x8217,
} GLFramebufferAttachmentParameter;

typedef enum GLRenderbufferParameter : GLenum {
    GL_RENDERBUFFER_WIDTH = 0x8D42,
    GL_RENDERBUFFER_HEIGHT = 0x8D43,
    GL_RENDERBUFFER_INTERNAL_FORMAT = 0x8D44,
    GL_RENDERBUFFER_RED_SIZE = 0x8D50,
    GL_RENDERBUFFER_GREEN_SIZE = 0x8D51,
    GL_RENDERBUFFER_BLUE_SIZE = 0x8D52,
    GL_RENDERBUFFER_ALPHA_SIZE = 0x8D53,
    GL_RENDERBUFFER_DEPTH_SIZE = 0x8D54,
    GL_RENDERBUFFER_STENCIL_SIZE = 0x8D55,
    GL_RENDERBUFFER_SAMPLES = 0x8CAB,
} GLRenderbufferParameter;

// --- Shaders & Programs ---
typedef enum GLShaderType : GLenum {
    GL_FRAGMENT_SHADER = 0x8B30,
    GL_VERTEX_SHADER = 0x8B31,
    GL_GEOMETRY_SHADER = 0x8DD9,
    GL_TESS_EVALUATION_SHADER = 0x8E87,
    GL_TESS_CONTROL_SHADER = 0x8E88,
    GL_COMPUTE_SHADER = 0x91B9,
} GLShaderType;

typedef enum GLShaderParameter : GLenum {
    GL_SHADER_TYPE = 0x8B4F,
    GL_DELETE_STATUS = 0x8B80,
    GL_COMPILE_STATUS = 0x8B81,
    GL_INFO_LOG_LENGTH = 0x8B84,
    GL_SHADER_SOURCE_LENGTH = 0x8B88,
} GLShaderParameter;

typedef enum GLProgramParameter : GLenum {
    GL_LINK_STATUS = 0x8B82,
    GL_VALIDATE_STATUS = 0x8B83,
    GL_ATTACHED_SHADERS = 0x8B85,
    GL_ACTIVE_UNIFORMS = 0x8B86,
    GL_ACTIVE_UNIFORM_MAX_LENGTH = 0x8B87,
    GL_ACTIVE_ATTRIBUTES = 0x8B89,
    GL_ACTIVE_ATTRIBUTE_MAX_LENGTH = 0x8B8A,
} GLProgramParameter;

typedef enum GLUniformType : GLenum {
    GL_FLOAT_VEC2 = 0x8B50,
    GL_FLOAT_VEC3 = 0x8B51,
    GL_FLOAT_VEC4 = 0x8B52,
    GL_DOUBLE_VEC2 = 0x8FFC,
    GL_DOUBLE_VEC3 = 0x8FFD,
    GL_DOUBLE_VEC4 = 0x8FFE,
    GL_INT_VEC2 = 0x8B53,
    GL_INT_VEC3 = 0x8B54,
    GL_INT_VEC4 = 0x8B55,
    GL_UNSIGNED_INT_VEC2 = 0x8DC6,
    GL_UNSIGNED_INT_VEC3 = 0x8DC7,
    GL_UNSIGNED_INT_VEC4 = 0x8DC8,
    GL_BOOL = 0x8B56,
    GL_BOOL_VEC2 = 0x8B57,
    GL_BOOL_VEC3 = 0x8B58,
    GL_BOOL_VEC4 = 0x8B59,
    GL_FLOAT_MAT2 = 0x8B5A,
    GL_FLOAT_MAT3 = 0x8B5B,
    GL_FLOAT_MAT4 = 0x8B5C,
    GL_FLOAT_MAT2x3 = 0x8B65,
    GL_FLOAT_MAT2x4 = 0x8B66,
    GL_FLOAT_MAT3x2 = 0x8B67,
    GL_FLOAT_MAT3x4 = 0x8B68,
    GL_FLOAT_MAT4x2 = 0x8B69,
    GL_FLOAT_MAT4x3 = 0x8B6A,
    GL_DOUBLE_MAT2 = 0x8F46,
    GL_DOUBLE_MAT3 = 0x8F47,
    GL_DOUBLE_MAT4 = 0x8F48,
    GL_SAMPLER_1D = 0x8B5D,
    GL_SAMPLER_2D = 0x8B5E,
    GL_SAMPLER_3D = 0x8B5F,
    GL_SAMPLER_CUBE = 0x8B60,
    GL_SAMPLER_1D_SHADOW = 0x8B61,
    GL_SAMPLER_2D_SHADOW = 0x8B62,
} GLUniformType;

typedef enum GLProgramInterface : GLenum {
    GL_UNIFORM = 0x92E1,
    GL_UNIFORM_BLOCK = 0x92E2,
    GL_PROGRAM_INPUT = 0x92E3,
    GL_PROGRAM_OUTPUT = 0x92E4,
    GL_BUFFER_VARIABLE = 0x92E5,
    GL_SHADER_STORAGE_BLOCK = 0x92E6,
    GL_VERTEX_SUBROUTINE = 0x92E8,
    GL_TESS_CONTROL_SUBROUTINE = 0x92E9,
    GL_TESS_EVALUATION_SUBROUTINE = 0x92EA,
    GL_GEOMETRY_SUBROUTINE = 0x92EB,
    GL_FRAGMENT_SUBROUTINE = 0x92EC,
    GL_COMPUTE_SUBROUTINE = 0x92ED,
    GL_VERTEX_SUBROUTINE_UNIFORM = 0x92EE,
    GL_TESS_CONTROL_SUBROUTINE_UNIFORM = 0x92EF,
    GL_TESS_EVALUATION_SUBROUTINE_UNIFORM = 0x92F0,
    GL_GEOMETRY_SUBROUTINE_UNIFORM = 0x92F1,
    GL_FRAGMENT_SUBROUTINE_UNIFORM = 0x92F2,
    GL_COMPUTE_SUBROUTINE_UNIFORM = 0x92F3,
    GL_TRANSFORM_FEEDBACK_VARYING = 0x92F4,
} GLProgramInterface;

typedef enum GLProgramResourceProperty : GLenum {
    GL_ACTIVE_RESOURCES = 0x92F5,
    GL_MAX_NAME_LENGTH = 0x92F6,
    GL_MAX_NUM_ACTIVE_VARIABLES = 0x92F7,
    GL_NAME_LENGTH = 0x92F9,
    GL_TYPE = 0x92FA,
    GL_ARRAY_SIZE = 0x92FB,
    GL_OFFSET = 0x92FC,
    GL_BLOCK_INDEX = 0x92FD,
    GL_ARRAY_STRIDE = 0x92FE,
    GL_MATRIX_STRIDE = 0x92FF,
    GL_IS_ROW_MAJOR = 0x9300,
    GL_ATOMIC_COUNTER_BUFFER_INDEX = 0x9301,
    GL_BUFFER_BINDING = 0x9302,
    GL_BUFFER_DATA_SIZE = 0x9303,
    GL_NUM_ACTIVE_VARIABLES = 0x9304,
    GL_ACTIVE_VARIABLES = 0x9305,
    GL_REFERENCED_BY_VERTEX_SHADER = 0x9306,
    GL_REFERENCED_BY_TESS_CONTROL_SHADER = 0x9307,
    GL_REFERENCED_BY_TESS_EVALUATION_SHADER = 0x9308,
    GL_REFERENCED_BY_GEOMETRY_SHADER = 0x9309,
    GL_REFERENCED_BY_FRAGMENT_SHADER = 0x930A,
    GL_REFERENCED_BY_COMPUTE_SHADER = 0x930B,
    GL_TOP_LEVEL_ARRAY_SIZE = 0x930C,
    GL_TOP_LEVEL_ARRAY_STRIDE = 0x930D,
    GL_LOCATION = 0x930E,
    GL_LOCATION_INDEX = 0x930F,
    GL_IS_PER_PATCH = 0x92E7,
} GLProgramResourceProperty;

// --- Vertex Attributes ---
typedef enum GLVertexAttribParameter : GLenum {
    GL_VERTEX_ATTRIB_ARRAY_ENABLED = 0x8622,
    GL_VERTEX_ATTRIB_ARRAY_SIZE = 0x8623,
    GL_VERTEX_ATTRIB_ARRAY_STRIDE = 0x8624,
    GL_VERTEX_ATTRIB_ARRAY_TYPE = 0x8625,
    GL_VERTEX_ATTRIB_ARRAY_NORMALIZED = 0x886A,
    GL_VERTEX_ATTRIB_ARRAY_POINTER = 0x8645,
    GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING = 0x889F,
    GL_VERTEX_ATTRIB_ARRAY_INTEGER = 0x88FD,
    GL_VERTEX_ATTRIB_ARRAY_DIVISOR = 0x88FE,
    GL_VERTEX_ATTRIB_ARRAY_LONG = 0x874E,
    GL_VERTEX_ATTRIB_BINDING = 0x82D4,
    GL_VERTEX_ATTRIB_RELATIVE_OFFSET = 0x82D5,
    GL_VERTEX_BINDING_DIVISOR = 0x82D6,
    GL_VERTEX_BINDING_OFFSET = 0x82D7,
    GL_VERTEX_BINDING_STRIDE = 0x82D8,
    GL_VERTEX_BINDING_BUFFER = 0x8F4F,
} GLVertexAttribParameter;

// --- Compute & Image Access ---
typedef enum GLImageAccess : GLenum {
    GL_READ_ONLY = 0x88B8,
    GL_WRITE_ONLY = 0x88B9,
    GL_READ_WRITE = 0x88BA,
} GLImageAccess;

typedef enum GLMemoryBarrierMask : uint32_t {
    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT = 0x00000001,
    GL_ELEMENT_ARRAY_BARRIER_BIT = 0x00000002,
    GL_UNIFORM_BARRIER_BIT = 0x00000004,
    GL_TEXTURE_FETCH_BARRIER_BIT = 0x00000008,
    GL_SHADER_IMAGE_ACCESS_BARRIER_BIT = 0x00000020,
    GL_COMMAND_BARRIER_BIT = 0x00000040,
    GL_PIXEL_BUFFER_BARRIER_BIT = 0x00000080,
    GL_TEXTURE_UPDATE_BARRIER_BIT = 0x00000100,
    GL_BUFFER_UPDATE_BARRIER_BIT = 0x00000200,
    GL_FRAMEBUFFER_BARRIER_BIT = 0x00000400,
    GL_TRANSFORM_FEEDBACK_BARRIER_BIT = 0x00000800,
    GL_ATOMIC_COUNTER_BARRIER_BIT = 0x00001000,
    GL_SHADER_STORAGE_BARRIER_BIT = 0x00002000,
    GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT = 0x00004000,
    GL_QUERY_BUFFER_BARRIER_BIT = 0x00008000,
    GL_ALL_BARRIER_BITS = 0xFFFFFFFF,
} GLMemoryBarrierMask;

// --- Sync Objects ---
typedef enum GLSyncStatus : GLenum {
    GL_ALREADY_SIGNALED = 0x911A,
    GL_TIMEOUT_EXPIRED = 0x911B,
    GL_CONDITION_SATISFIED = 0x911C,
    GL_WAIT_FAILED = 0x911D,
} GLSyncStatus;

typedef enum GLSyncMask : uint32_t {
    GL_SYNC_FLUSH_COMMANDS_BIT = 0x00000001,
} GLSyncMask;

typedef enum GLSyncParameter : GLenum {
    GL_SYNC_GPU_COMMANDS_COMPLETE = 0x9117,
    GL_SYNC_STATUS = 0x9114,
    GL_SYNC_CONDITION = 0x9113,
    GL_SYNC_FLAGS = 0x9115,
    GL_SYNC_FENCE = 0x9116,
    GL_OBJECT_TYPE = 0x9112,
} GLSyncParameter;

// --- Queries ---
typedef enum GLQueryTarget : GLenum {
    GL_SAMPLES_PASSED = 0x8914,
    GL_ANY_SAMPLES_PASSED = 0x8C2F,
    GL_ANY_SAMPLES_PASSED_CONSERVATIVE = 0x8D6A,
    GL_PRIMITIVES_GENERATED = 0x8C87,
    GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN = 0x8C88,
    GL_TIME_ELAPSED = 0x88BF,
    GL_TIMESTAMP = 0x8E28,
} GLQueryTarget;

typedef enum GLQueryParameter : GLenum {
    GL_QUERY_RESULT = 0x8866,
    GL_QUERY_RESULT_AVAILABLE = 0x8867,
    GL_QUERY_RESULT_NO_WAIT = 0x9194,
} GLQueryParameter;

// --- Transform Feedback ---
typedef enum GLTransformFeedbackMode : GLenum {
    GL_INTERLEAVED_ATTRIBS = 0x8C8C,
    GL_SEPARATE_ATTRIBS = 0x8C8D,
    GL_TRANSFORM_FEEDBACK = 0x8E22,
} GLTransformFeedbackMode;

// --- Tessellation ---
typedef enum GLPatchParameter : GLenum {
    GL_PATCH_VERTICES = 0x8E72,
    GL_PATCH_DEFAULT_INNER_LEVEL = 0x8E73,
    GL_PATCH_DEFAULT_OUTER_LEVEL = 0x8E74,
    GL_TESS_GEN_MODE = 0x8E76,
    GL_TESS_GEN_SPACING = 0x8E77,
    GL_TESS_GEN_VERTEX_ORDER = 0x8E78,
    GL_TESS_GEN_POINT_MODE = 0x8E79,
} GLPatchParameter;

// --- Provoking Vertex ---
typedef enum GLProvokingVertexMode : GLenum {
    GL_FIRST_VERTEX_CONVENTION = 0x8E4D,
    GL_LAST_VERTEX_CONVENTION = 0x8E4E,
    GL_PROVOKING_VERTEX = 0x8E4F,
} GLProvokingVertexMode;

// --- Logic Operations ---
typedef enum GLLogicOp : GLenum {
    GL_CLEAR = 0x1500,
    GL_AND = 0x1501,
    GL_AND_REVERSE = 0x1502,
    GL_COPY = 0x1503,
    GL_AND_INVERTED = 0x1504,
    GL_NOOP = 0x1505,
    GL_XOR = 0x1506,
    GL_OR = 0x1507,
    GL_NOR = 0x1508,
    GL_EQUIV = 0x1509,
    GL_LOGIC_INVERT = 0x150A, // same value as GL_INVERT (0x150A); alias for logic op context
    GL_OR_REVERSE = 0x150B,
    GL_COPY_INVERTED = 0x150C,
    GL_OR_INVERTED = 0x150D,
    GL_NAND = 0x150E,
    GL_SET = 0x150F,
} GLLogicOp;

// --- Atomic Counters ---
typedef enum GLAtomicCounterParameter : GLenum {
    GL_ATOMIC_COUNTER_BUFFER_BINDING = 0x92C1,
    GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE = 0x92C4,
    GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS = 0x92C5,
    GL_UNSIGNED_INT_ATOMIC_COUNTER = 0x92DB,
} GLAtomicCounterParameter;

// --- Debug ---
typedef enum GLDebugSource : GLenum {
    GL_DEBUG_SOURCE_API = 0x8246,
    GL_DEBUG_SOURCE_WINDOW_SYSTEM = 0x8247,
    GL_DEBUG_SOURCE_SHADER_COMPILER = 0x8248,
    GL_DEBUG_SOURCE_THIRD_PARTY = 0x8249,
    GL_DEBUG_SOURCE_APPLICATION = 0x824A,
    GL_DEBUG_SOURCE_OTHER = 0x824B,
} GLDebugSource;

typedef enum GLDebugType : GLenum {
    GL_DEBUG_TYPE_ERROR = 0x824C,
    GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR = 0x824D,
    GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR = 0x824E,
    GL_DEBUG_TYPE_PORTABILITY = 0x824F,
    GL_DEBUG_TYPE_PERFORMANCE = 0x8250,
    GL_DEBUG_TYPE_OTHER = 0x8251,
    GL_DEBUG_TYPE_MARKER = 0x8268,
    GL_DEBUG_TYPE_PUSH_GROUP = 0x8269,
    GL_DEBUG_TYPE_POP_GROUP = 0x826A,
} GLDebugType;

typedef enum GLDebugSeverity : GLenum {
    GL_DEBUG_SEVERITY_HIGH = 0x9146,
    GL_DEBUG_SEVERITY_MEDIUM = 0x9147,
    GL_DEBUG_SEVERITY_LOW = 0x9148,
    GL_DEBUG_SEVERITY_NOTIFICATION = 0x826B,
} GLDebugSeverity;

typedef enum GLDebugParameter : GLenum {
    GL_DEBUG_LOGGED_MESSAGES = 0x9145,
    GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH = 0x8243,
    GL_DEBUG_GROUP_STACK_DEPTH = 0x826D,
} GLDebugParameter;

// --- Object Identifiers (for Debug Labelling) ---
typedef enum GLObjectIdentifier : GLenum {
    GL_BUFFER_OBJECT = 0x9151,
    GL_SHADER_OBJECT = 0x8B48,
    GL_PROGRAM_OBJECT = 0x8B40,
    GL_VERTEX_ARRAY_OBJECT = 0x9154,
    GL_QUERY_OBJECT = 0x9153,
    GL_PROGRAM_PIPELINE_OBJECT = 0x8A4F,
    GL_SAMPLER_OBJECT = 0x8C2E,
} GLObjectIdentifier;

// --- Draw / Read Buffers ---
typedef enum GLDrawBuffer : GLenum {
    GL_DRAW_BUFFER0 = 0x8825,
    GL_DRAW_BUFFER1 = 0x8826,
    GL_DRAW_BUFFER2 = 0x8827,
    GL_DRAW_BUFFER3 = 0x8828,
    GL_DRAW_BUFFER4 = 0x8829,
    GL_DRAW_BUFFER5 = 0x882A,
    GL_DRAW_BUFFER6 = 0x882B,
    GL_DRAW_BUFFER7 = 0x882C,
    GL_DRAW_BUFFER8 = 0x882D,
    GL_DRAW_BUFFER9 = 0x882E,
    GL_DRAW_BUFFER10 = 0x882F,
    GL_DRAW_BUFFER11 = 0x8830,
    GL_DRAW_BUFFER12 = 0x8831,
    GL_DRAW_BUFFER13 = 0x8832,
    GL_DRAW_BUFFER14 = 0x8833,
    GL_DRAW_BUFFER15 = 0x8834,
    GL_BACK_LEFT = 0x0402,
    GL_BACK_RIGHT = 0x0403,
    GL_FRONT_LEFT = 0x0400,
    GL_FRONT_RIGHT = 0x0401,
    GL_LEFT = 0x0406,
    GL_RIGHT = 0x0407,
    GL_NONE = 0,
} GLDrawBuffer;

// --- Spec-Guaranteed Minimum Implementation Limits ---
// These are the "basement" values. Every OpenGL 4.6 driver MUST support at least these.
// Use these for static array sizes or compile-time checks.

[[maybe_unused]] static constexpr GLint GL_SPEC_MIN_MAP_BUFFER_ALIGNMENT = 64;

// The range of offsets for textureOffset() in GLSL
[[maybe_unused]] static constexpr GLint GL_SPEC_MIN_TEXEL_OFFSET = -8;
[[maybe_unused]] static constexpr GLint GL_SPEC_MAX_TEXEL_OFFSET = 7;

// Resource Bindings
[[maybe_unused]] static constexpr GLint GL_SPEC_MIN_UBO_BINDINGS = 36;
[[maybe_unused]] static constexpr GLint GL_SPEC_MIN_TEXTURE_UNITS = 16;
[[maybe_unused]] static constexpr GLint GL_SPEC_MIN_COMBINED_TEXTURE_UNITS = 96;
[[maybe_unused]] static constexpr GLint GL_SPEC_MIN_COLOR_ATTACHMENTS = 8;

// -----------------------------------------------------------------------------
// Platform / Calling Convention
// -----------------------------------------------------------------------------
#ifndef APIENTRY
#ifdef _WIN32
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif
#endif

#ifdef _WIN32
#define GL_API __stdcall
#else
#define GL_API
#endif

// -----------------------------------------------------------------------------
// Debug Callback Type
// -----------------------------------------------------------------------------
typedef void(APIENTRY *GLDEBUGPROC)(
    GLenum source,         // Where the message came from (API, window system, etc.)
    GLenum type,           // Error, performance warning, portability, etc.
    GLuint id,             // Driver-specific message ID
    GLenum severity,       // High, medium, low, or notification
    GLsizei length,        // Length of the message string
    const GLchar *message, // The human-readable message
    const void *userParam  // User pointer passed to DebugMessageCallback
);

// -----------------------------------------------------------------------------
// Application-layer Enums
// -----------------------------------------------------------------------------
typedef enum UniformFunction : uint8_t {
    UF_1I,
    UF_2I,
    UF_3I,
    UF_4I,
    UF_1B,
    UF_2B,
    UF_3B,
    UF_4B,
    UF_1U,
    UF_2U,
    UF_3U,
    UF_4U,
    UF_1F,
    UF_2F,
    UF_3F,
    UF_4F,
    UF_MAT2,
    UF_MAT2x3,
    UF_MAT2x4,
    UF_MAT3x2,
    UF_MAT3,
    UF_MAT3x4,
    UF_MAT4x2,
    UF_MAT4x3,
    UF_MAT4,
    UF_MAT2_RM,
    UF_MAT3_RM,
    UF_MAT4_RM,
    UF_COUNT,
} UniformFunction;

typedef enum ImageFormatTupleIndex : uint8_t {
    IF_INTERNAL_FORMAT,
    IF_FORMAT,
    IF_TYPE,
    IF_BUFFER,
    IF_COMPONENTS,
    IF_PIXEL_SIZE,
    IF_COLOR,
    IF_FLAGS,
    IF_CLEAR_TYPE,
    IF_TUPLE_SIZE,
} ImageFormatTupleIndex;

// -----------------------------------------------------------------------------
// OpenGL Function Pointer X-Macros
// Format: X(ReturnType, FunctionName, Arguments...)
// -----------------------------------------------------------------------------

// ======================= OPENGL 3.3 CORE =======================
#define GL_FUNCTIONS_3_3_CORE(X)                                                                   \
    X(void, FrontFace, GLenum mode)                                                                \
    X(void, CullFace, GLenum mode)                                                                 \
    X(void, Clear, GLbitfield mask)                                                                \
    X(void, TexParameteri, GLenum target, GLenum pname, GLint param)                               \
    X(void, TexImage1D, GLenum target, GLint level, GLint internalformat, GLsizei width,           \
      GLint border, GLenum format, GLenum type, const void *data)                                  \
    X(void, TexImage2D, GLenum target, GLint level, GLint internalformat, GLsizei width,           \
      GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)                \
    X(void, DepthMask, GLboolean flag)                                                             \
    X(void, Disable, GLenum cap)                                                                   \
    X(void, Enable, GLenum cap)                                                                    \
    X(void, Flush, void)                                                                           \
    X(void, DepthFunc, GLenum func)                                                                \
    X(void, ReadBuffer, GLenum src)                                                                \
    X(void, ReadPixels, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,            \
      GLenum type, void *pixels)                                                                   \
    X(GLenum, GetError, void)                                                                      \
    X(void, GetIntegerv, GLenum pname, GLint *data)                                                \
    X(const GLchar *, GetString, GLenum name)                                                      \
    X(void, Viewport, GLint x, GLint y, GLsizei width, GLsizei height)                             \
    X(void, TexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset,               \
      GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)               \
    X(void, BindTexture, GLenum target, GLuint texture)                                            \
    X(void, DeleteTextures, GLsizei n, const GLuint *textures)                                     \
    X(void, GenTextures, GLsizei n, GLuint *textures)                                              \
    X(void, TexImage3D, GLenum target, GLint level, GLint internalformat, GLsizei width,           \
      GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) \
    X(void, TexSubImage3D, GLenum target, GLint level, GLint xoffset, GLint zoffset,               \
      GLint yoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,     \
      const void *pixels)                                                                          \
    X(void, ActiveTexture, GLenum texture)                                                         \
    X(void, BlendFuncSeparate, GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha,          \
      GLenum dfactorAlpha)                                                                         \
    X(void, BindBuffer, GLenum target, GLuint buffer)                                              \
    X(void, DeleteBuffers, GLsizei n, const GLuint *buffers)                                       \
    X(void, GenBuffers, GLsizei n, GLuint *buffers)                                                \
    X(void, BufferData, GLenum target, GLsizeiptr size, const void *data, GLenum usage)            \
    X(void, BufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const void *data)      \
    X(void, GetBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, void *data)         \
    X(void, BlendEquationSeparate, GLenum modeRGB, GLenum modeAlpha)                               \
    X(void, DrawBuffers, GLsizei n, const GLenum *bufs)                                            \
    X(void, StencilOp, GLenum fail, GLenum zfail, GLenum zpass)                                    \
    X(void, StencilFunc, GLenum func, GLint ref, GLuint mask)                                      \
    X(void, StencilMask, GLuint mask)                                                              \
    X(void, StencilOpSeparate, GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)            \
    X(void, StencilFuncSeparate, GLenum face, GLenum func, GLint ref, GLuint mask)                 \
    X(void, StencilMaskSeparate, GLenum face, GLuint mask)                                         \
    X(void, AttachShader, GLuint program, GLuint shader)                                           \
    X(void, CompileShader, GLuint shader)                                                          \
    X(GLuint, CreateProgram, void)                                                                 \
    X(GLuint, CreateShader, GLenum type)                                                           \
    X(void, DeleteProgram, GLuint program)                                                         \
    X(void, DeleteShader, GLuint shader)                                                           \
    X(void, DetachShader, GLuint program, GLuint shader)                                           \
    X(void, DisableVertexAttribArray, GLuint index)                                                \
    X(void, EnableVertexAttribArray, GLuint index)                                                 \
    X(void, GetActiveUniform, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length,      \
      GLint *size, GLenum *type, GLchar *name)                                                     \
    X(void, GetActiveAttrib, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length,       \
      GLint *size, GLenum *type, GLchar *name)                                                     \
    X(GLint, GetAttribLocation, GLuint program, const GLchar *name)                                \
    X(void, GetProgramiv, GLuint program, GLenum pname, GLint *params)                             \
    X(void, GetProgramInfoLog, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)  \
    X(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params)                               \
    X(void, GetShaderInfoLog, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog)    \
    X(GLint, GetUniformLocation, GLuint program, const GLchar *name)                               \
    X(void, LinkProgram, GLuint program)                                                           \
    X(void, ShaderSource, GLuint shader, GLsizei count, const GLchar *const *string,               \
      const GLint *length)                                                                         \
    X(void, UseProgram, GLuint program)                                                            \
    X(void, Uniform1i, GLint location, GLint v0)                                                   \
    X(void, Uniform1fv, GLint location, GLsizei count, const GLfloat *value)                       \
    X(void, Uniform2fv, GLint location, GLsizei count, const GLfloat *value)                       \
    X(void, Uniform3fv, GLint location, GLsizei count, const GLfloat *value)                       \
    X(void, Uniform4fv, GLint location, GLsizei count, const GLfloat *value)                       \
    X(void, Uniform1iv, GLint location, GLsizei count, const GLint *value)                         \
    X(void, Uniform2iv, GLint location, GLsizei count, const GLint *value)                         \
    X(void, Uniform3iv, GLint location, GLsizei count, const GLint *value)                         \
    X(void, Uniform4iv, GLint location, GLsizei count, const GLint *value)                         \
    X(void, UniformMatrix2fv, GLint location, GLsizei count, GLboolean transpose,                  \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix3fv, GLint location, GLsizei count, GLboolean transpose,                  \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose,                  \
      const GLfloat *value)                                                                        \
    X(void, Uniform1f, GLint location, GLfloat v0)                                                 \
    X(void, Uniform1ui, GLint location, GLuint v0)                                                 \
    X(void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized,      \
      GLsizei stride, const void *pointer)                                                         \
    X(void, UniformMatrix2x3fv, GLint location, GLsizei count, GLboolean transpose,                \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix3x2fv, GLint location, GLsizei count, GLboolean transpose,                \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix2x4fv, GLint location, GLsizei count, GLboolean transpose,                \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix4x2fv, GLint location, GLsizei count, GLboolean transpose,                \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix3x4fv, GLint location, GLsizei count, GLboolean transpose,                \
      const GLfloat *value)                                                                        \
    X(void, UniformMatrix4x3fv, GLint location, GLsizei count, GLboolean transpose,                \
      const GLfloat *value)                                                                        \
    X(void, BindBufferRange, GLenum target, GLuint index, GLuint buffer, GLintptr offset,          \
      GLsizeiptr size)                                                                             \
    X(void, VertexAttribIPointer, GLuint index, GLint size, GLenum type, GLsizei stride,           \
      const void *pointer)                                                                         \
    X(void, Uniform1uiv, GLint location, GLsizei count, const GLuint *value)                       \
    X(void, Uniform2uiv, GLint location, GLsizei count, const GLuint *value)                       \
    X(void, Uniform3uiv, GLint location, GLsizei count, const GLuint *value)                       \
    X(void, Uniform4uiv, GLint location, GLsizei count, const GLuint *value)                       \
    X(void, ClearBufferiv, GLenum buffer, GLint drawbuffer, const GLint *value)                    \
    X(void, ClearBufferuiv, GLenum buffer, GLint drawbuffer, const GLuint *value)                  \
    X(void, ClearBufferfv, GLenum buffer, GLint drawbuffer, const GLfloat *value)                  \
    X(void, ClearBufferfi, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)          \
    X(void, BindRenderbuffer, GLenum target, GLuint renderbuffer)                                  \
    X(void, DeleteRenderbuffers, GLsizei n, const GLuint *renderbuffers)                           \
    X(void, GenRenderbuffers, GLsizei n, GLuint *renderbuffers)                                    \
    X(void, BindFramebuffer, GLenum target, GLuint framebuffer)                                    \
    X(GLenum, CheckFramebufferStatus, GLenum target)                                               \
    X(void, DeleteFramebuffers, GLsizei n, const GLuint *framebuffers)                             \
    X(void, GenFramebuffers, GLsizei n, GLuint *framebuffers)                                      \
    X(void, FramebufferTexture2D, GLenum target, GLenum attachment, GLenum textarget,              \
      GLuint texture, GLint level)                                                                 \
    X(void, FramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget,  \
      GLuint renderbuffer)                                                                         \
    X(void, GenerateMipmap, GLenum target)                                                         \
    X(void, BlitFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,      \
      GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)                       \
    X(void, RenderbufferStorageMultisample, GLenum target, GLsizei samples, GLenum internalformat, \
      GLsizei width, GLsizei height)                                                               \
    X(void, FramebufferTextureLayer, GLenum target, GLenum attachment, GLuint texture,             \
      GLint level, GLint layer)                                                                    \
    X(void, BindVertexArray, GLuint array)                                                         \
    X(void, DeleteVertexArrays, GLsizei n, const GLuint *arrays)                                   \
    X(void, GenVertexArrays, GLsizei n, GLuint *arrays)                                            \
    X(void, DrawArrays, GLenum mode, GLint first, GLsizei count)                                   \
    X(void, DrawElements, GLenum mode, GLsizei count, GLenum type, const void *indices)            \
    X(void, DrawArraysInstanced, GLenum mode, GLint first, GLsizei count, GLsizei instancecount)   \
    X(void, DrawElementsInstanced, GLenum mode, GLsizei count, GLenum type, const void *indices,   \
      GLsizei instancecount)                                                                       \
    X(void, DrawRangeElements, GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type,  \
      const void *indices)                                                                         \
    X(void, CopyBufferSubData, GLenum readTarget, GLenum writeTarget, GLintptr readOffset,         \
      GLintptr writeOffset, GLsizeiptr size)                                                       \
    X(GLuint, GetUniformBlockIndex, GLuint program, const GLchar *uniformBlockName)                \
    X(void, GetActiveUniformBlockiv, GLuint program, GLuint uniformBlockIndex, GLenum pname,       \
      GLint *params)                                                                               \
    X(void, GetActiveUniformBlockName, GLuint program, GLuint uniformBlockIndex, GLsizei bufSize,  \
      GLsizei *length, GLchar *uniformBlockName)                                                   \
    X(void, UniformBlockBinding, GLuint program, GLuint uniformBlockIndex,                         \
      GLuint uniformBlockBinding)                                                                  \
    X(void, GenSamplers, GLsizei count, GLuint *samplers)                                          \
    X(void, DeleteSamplers, GLsizei count, const GLuint *samplers)                                 \
    X(void, BindSampler, GLuint unit, GLuint sampler)                                              \
    X(void, SamplerParameteri, GLuint sampler, GLenum pname, GLint param)                          \
    X(void, SamplerParameterf, GLuint sampler, GLenum pname, GLfloat param)                        \
    X(void, VertexAttribDivisor, GLuint index, GLuint divisor)                                     \
    X(void, BindBufferBase, GLenum target, GLuint index, GLuint buffer)                            \
    X(void *, MapBufferRange, GLenum target, GLintptr offset, GLsizeiptr length,                   \
      GLbitfield access)                                                                           \
    X(GLboolean, UnmapBuffer, GLenum target)                                                       \
    X(void, PixelStorei, GLenum pname, GLint param)                                                \
    X(void, GetBufferParameteriv, GLenum target, GLenum pname, GLint *params)                      \
    X(GLsync, FenceSync, GLenum condition, GLbitfield flags)                                       \
    X(void, DeleteSync, GLsync sync)                                                               \
    X(GLenum, ClientWaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout)                     \
    X(void, WaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout)                             \
    X(void, Finish, void)                                                                          \
    X(void, GenQueries, GLsizei n, GLuint *ids)                                                    \
    X(void, DeleteQueries, GLsizei n, const GLuint *ids)                                           \
    X(GLboolean, IsQuery, GLuint id)                                                               \
    X(void, BeginQuery, GLenum target, GLuint id)                                                  \
    X(void, EndQuery, GLenum target)                                                               \
    X(void, GetQueryiv, GLenum target, GLenum pname, GLint *params)                                \
    X(void, GetQueryObjectiv, GLuint id, GLenum pname, GLint *params)                              \
    X(void, GetQueryObjectuiv, GLuint id, GLenum pname, GLuint *params)                            \
    X(void, QueryCounter, GLuint id, GLenum target)                                                \
    X(void, GetQueryObjecti64v, GLuint id, GLenum pname, GLint64 *params)                          \
    X(void, GetQueryObjectui64v, GLuint id, GLenum pname, GLuint64 *params)

// ======================= OPENGL 4.2 CORE =======================
#define GL_FUNCTIONS_4_2_CORE(X)                                                                   \
    X(void, BindImageTexture, GLuint unit, GLuint texture, GLint level, GLboolean layered,         \
      GLint layer, GLenum access, GLenum format)                                                   \
    X(void, MemoryBarrier, GLbitfield barriers)

// ======================= OPENGL 4.3 CORE =======================
#define GL_FUNCTIONS_4_3_CORE(X)                                                                   \
    X(void, DispatchCompute, GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)        \
    X(void, GetProgramInterfaceiv, GLuint program, GLenum programInterface, GLenum pname,          \
      GLint *params)                                                                               \
    X(void, DebugMessageCallback, GLDEBUGPROC callback, const void *userParam)                     \
    X(void, DebugMessageControl, GLenum source, GLenum type, GLenum severity, GLsizei count,       \
      const GLuint *ids, GLboolean enabled)                                                        \
    X(void, GetProgramResourceiv, GLuint program, GLenum programInterface, GLuint index,           \
      GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params)     \
    X(void, GetProgramResourceName, GLuint program, GLenum programInterface, GLuint index,         \
      GLsizei bufSize, GLsizei *length, GLchar *name)

// ======================= OPENGL 4.4 CORE =======================
#define GL_FUNCTIONS_4_4_CORE(X)                                                                   \
    X(void, BufferStorage, GLenum target, GLsizeiptr size, const void *data, GLbitfield flags)

// ======================= OPTIONAL / EXPERIMENTAL =======================

// OpenGL 4.3 Core (optional in loader)
#define GL_FUNCTIONS_4_3_OPTIONAL(X)                                                               \
    X(void, MultiDrawArraysIndirect, GLenum mode, const void *indirect, GLsizei drawcount,         \
      GLsizei stride)                                                                              \
    X(void, MultiDrawElementsIndirect, GLenum mode, GLenum type, const void *indirect,             \
      GLsizei drawcount, GLsizei stride)

// OpenGL 4.6 Core
#define GL_FUNCTIONS_4_6_OPTIONAL(X)                                                               \
    X(void, MultiDrawArraysIndirectCount, GLenum mode, const void *indirect, GLintptr drawcount,   \
      GLsizei maxdrawcount, GLsizei stride)                                                        \
    X(void, MultiDrawElementsIndirectCount, GLenum mode, GLenum type, const void *indirect,        \
      GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride)

// ARB_bindless_texture
#define GL_FUNCTIONS_EXT_BINDLESS(X)                                                               \
    X(GLuint64, GetTextureHandleARB, GLuint texture)                                               \
    X(void, MakeTextureHandleResidentARB, GLuint64 handle)                                         \
    X(void, MakeTextureHandleNonResidentARB, GLuint64 handle)

// -----------------------------------------------------------------------------
// Function Pointer Declaration Macro
// Note: Each TU that includes this header gets its own static pointer.
// Call your loader in each TU, or move declarations to a .cpp with extern.
// -----------------------------------------------------------------------------
#define DECLARE_GL_FUNC(ret, name, ...)                                                            \
    typedef ret(GL_API *PFN##name##PROC)(__VA_ARGS__);                                             \
    [[maybe_unused]] static PFN##name##PROC name = nullptr;

GL_FUNCTIONS_3_3_CORE(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_2_CORE(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_3_CORE(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_4_CORE(DECLARE_GL_FUNC)

GL_FUNCTIONS_4_3_OPTIONAL(DECLARE_GL_FUNC)
GL_FUNCTIONS_4_6_OPTIONAL(DECLARE_GL_FUNC)
GL_FUNCTIONS_EXT_BINDLESS(DECLARE_GL_FUNC)