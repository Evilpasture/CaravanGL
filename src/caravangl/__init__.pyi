from collections.abc import Buffer as _BufferProtocol
from typing import Any

# --- Constants ---
FLOAT: int
UNSIGNED_BYTE: int
UNSIGNED_SHORT: int
UNSIGNED_INT: int
UNSIGNED_INT_24_8: int

# Primitives
TRIANGLES: int
LINES: int
POINTS: int

# Uniform Function IDs
UF_1I: int
UF_3I: int
UF_1F: int
UF_2F: int
UF_3F: int
UF_4F: int
UF_MAT4: int
UF_MAT4_RM: int

# Buffer Targets & Usage
ARRAY_BUFFER: int
ELEMENT_ARRAY_BUFFER: int
UNIFORM_BUFFER: int
STATIC_DRAW: int
DYNAMIC_DRAW: int
STREAM_DRAW: int

# Texture Constants
TEXTURE_2D: int
TEXTURE_3D: int
RGBA: int
RGB: int
RGBA8: int
DEPTH_COMPONENT: int
DEPTH_COMPONENT24: int
DEPTH24_STENCIL8: int
DEPTH_STENCIL: int

# Framebuffer Constants
FRAMEBUFFER: int
COLOR_ATTACHMENT0: int
DEPTH_ATTACHMENT: int
DEPTH_STENCIL_ATTACHMENT: int
COLOR_BUFFER_BIT: int
DEPTH_BUFFER_BIT: int
STENCIL_BUFFER_BIT: int

# Depth/Stencil Compare Functions
NEVER: int
LESS: int
EQUAL: int
LEQUAL: int
GREATER: int
NOTEQUAL: int
GEQUAL: int
ALWAYS: int

# Stencil Operations
KEEP: int
ZERO: int
REPLACE: int
INCR: int
INCR_WRAP: int
DECR: int
DECR_WRAP: int
INVERT: int

# Culling
FRONT: int
BACK: int
FRONT_AND_BACK: int

# Blending
SRC_ALPHA: int
ONE_MINUS_SRC_ALPHA: int
ONE: int
FUNC_ADD: int

# Sampler Constants
NEAREST: int
LINEAR: int
REPEAT: int
CLAMP_TO_EDGE: int

# Faces
CW: int
CCW: int

# Metadata
FREE_THREADED: int
DEBUG_BUILD: int

TIMEOUT_IGNORED: int
ALREADY_SIGNALED: int
TIMEOUT_EXPIRED: int
CONDITION_SATISFIED: int
WAIT_FAILED: int

# --- Module Level Functions ---

def enable_debug() -> None: ...
def context() -> dict[str, Any]: ...
def inspect(obj: object) -> dict[str, Any] | None: ...
def clear(mask: int) -> None: ...
def clear_color(r: float, g: float, b: float, a: float) -> None: ...
def viewport(x: int, y: int, width: int, height: int) -> None: ...
def bind_default_framebuffer() -> None: ...
def get_active_context() -> "Context | None": ...

# --- Classes ---

class Context:
    os_make_current_cb: Any
    def __init__(self, loader: Any, callback: Any = None) -> None: ...
    def make_current(self) -> None: ...

class Buffer:
    def __init__(self, size: int, data: _BufferProtocol | None = None, target: int = ..., usage: int = ...) -> None: ...
    def write(self, data: _BufferProtocol, offset: int = 0) -> None: ...
    def bind_base(self, index: int) -> None: ...

class Sampler:
    def __init__(self, min_filter: int = ..., mag_filter: int = ..., wrap_s: int = ..., wrap_t: int = ...) -> None: ...

class Texture:
    def __init__(self, target: int = ...) -> None: ...
    def upload(self, width: int, height: int, internal_format: int, format: int, type: int, 
               data: _BufferProtocol | None = None, level: int = 0, depth: int = 0) -> None: ...
    def bind(self, unit: int, sampler: Sampler | None = None) -> None: ...
    def generate_mipmap(self) -> None: ...

class Program:
    def __init__(self, vertex_shader: str, fragment_shader: str) -> None: ...
    def get_uniform_location(self, name: str) -> int: ...

class VertexArray:
    def __init__(self) -> None: ...
    def bind_attribute(self, location: int, buffer: Buffer, size: int, type: int, 
                       normalized: int = 0, stride: int = 0, offset: int = 0, divisor: int = 0) -> None: ...
    def bind_index_buffer(self, buffer: Buffer) -> None: ...

class UniformBatch:
    def __init__(self, max_bindings: int, max_bytes: int) -> None: ...
    def add(self, func_id: int, location: int, count: int, size: int) -> int: ...
    @property
    def data(self) -> memoryview: ...

class Pipeline:
    def __init__(
        self, 
        program: Program, 
        vao: VertexArray, 
        topology: int = ..., 
        index_type: int = ..., 
        depth_test: int = 0, 
        depth_write: int = 1, 
        depth_func: int = ..., 
        cull: int = 0,
        cull_mode: int = ...,
        front_face: int = CCW,
        stencil_test: int = 0,
        stencil_func: int = ...,
        stencil_ref: int = 0,
        stencil_read_mask: int = 0xFFFFFFFF,
        stencil_write_mask: int = 0xFFFFFFFF,
        stencil_fail: int = ...,
        stencil_zfail: int = ...,
        stencil_zpass: int = ...,
        blend: int = 0, 
        blend_src_rgb: int = ...,
        blend_dst_rgb: int = ...,
        blend_src_alpha: int = ...,
        blend_dst_alpha: int = ...,
        blend_eq_rgb: int = ...,
        blend_eq_alpha: int = ...
    ) -> None: ...
    
    def upload_uniforms(self, batch: UniformBatch) -> None: ...
    def draw(self) -> None: ...
    
    @property
    def params(self) -> memoryview: ...

class Framebuffer:
    def __init__(self) -> None: ...
    def attach_texture(self, attachment: int, texture: Texture, level: int = 0) -> None: ...
    def check_status(self) -> bool: ...
    def bind(self) -> None: ...

class Sync:
    def __init__(self) -> None: ...
    def wait(self, timeout_sec: float | None = None) -> int: ...

__all__ = [
    "Context", "Buffer", "Sampler", "Texture", "Program", "VertexArray", "UniformBatch", "Pipeline", "Framebuffer", "Sync",
    "enable_debug", "context", "inspect", "clear", "clear_color", "viewport", 
    "bind_default_framebuffer", "get_active_context",
    "FLOAT", "UNSIGNED_BYTE", "UNSIGNED_SHORT", "UNSIGNED_INT", "UNSIGNED_INT_24_8",
    "TRIANGLES", "LINES", "POINTS",
    "UF_1I", "UF_3I", "UF_1F", "UF_2F", "UF_3F", "UF_4F", "UF_MAT4", "UF_MAT4_RM",
    "ARRAY_BUFFER", "ELEMENT_ARRAY_BUFFER", "UNIFORM_BUFFER", "STATIC_DRAW", "DYNAMIC_DRAW", "STREAM_DRAW",
    "TEXTURE_2D", "TEXTURE_3D", "RGBA", "RGB", "RGBA8", 
    "DEPTH_COMPONENT", "DEPTH_COMPONENT24", "DEPTH24_STENCIL8", "DEPTH_STENCIL",
    "FRAMEBUFFER", "COLOR_ATTACHMENT0", "DEPTH_ATTACHMENT", "DEPTH_STENCIL_ATTACHMENT",
    "COLOR_BUFFER_BIT", "DEPTH_BUFFER_BIT", "STENCIL_BUFFER_BIT",
    "NEVER", "LESS", "EQUAL", "LEQUAL", "GREATER", "NOTEQUAL", "GEQUAL", "ALWAYS",
    "KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT",
    "FRONT", "BACK", "FRONT_AND_BACK",
    "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "ONE", "FUNC_ADD",
    "NEAREST", "LINEAR", "REPEAT", "CLAMP_TO_EDGE",
    "CW", "CCW", "TIMEOUT_IGNORED", "ALREADY_SIGNALED", "TIMEOUT_EXPIRED", "CONDITION_SATISFIED", "WAIT_FAILED",
    "FREE_THREADED", "DEBUG_BUILD"
]