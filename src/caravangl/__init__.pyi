from typing import Any, Dict, Optional, Protocol, Tuple, Union
from typing_extensions import Self

# Type Alias for objects supporting the buffer protocol (bytes, numpy, etc.)
class BufferProtocol(Protocol):
    def __buffer__(self, flags: int) -> memoryview: ...

# --- Constants ---
# Using 'Literal' or just 'int' is fine for stubs
FLOAT: int
TRIANGLES: int
UNSIGNED_BYTE: int
UNSIGNED_SHORT: int
UNSIGNED_INT: int

# Uniform Function IDs
UF_1I: int
UF_1F: int
UF_2F: int
UF_3F: int
UF_4F: int
UF_MAT4: int

# Buffer Targets & Usage
ELEMENT_ARRAY_BUFFER: int
UNIFORM_BUFFER: int
STATIC_DRAW: int
DYNAMIC_DRAW: int

# Texture Constants
TEXTURE_2D: int
TEXTURE_3D: int
RGBA: int
RGB: int
RGBA8: int
DEPTH_COMPONENT: int
DEPTH_COMPONENT24: int

# Framebuffer Constants
FRAMEBUFFER: int
COLOR_ATTACHMENT0: int
DEPTH_ATTACHMENT: int
DEPTH_STENCIL_ATTACHMENT: int
COLOR_BUFFER_BIT: int
DEPTH_BUFFER_BIT: int

# Metadata
FREE_THREADED: int
DEBUG_BUILD: int

# --- Module Level Functions ---

def init(loader: Any) -> None: 
    """Initialize the OpenGL function table using a loader object."""
    ...

def enable_debug() -> None:
    """Enable OpenGL 4.3+ Debug Output callback."""
    ...

def context() -> Dict[str, Any]:
    """Returns a dictionary containing hardware capabilities and driver info."""
    ...

def inspect(obj: Any) -> Dict[str, Any]:
    """Returns the internal C/GPU state of a CaravanGL object."""
    ...

def clear(mask: int) -> None:
    """Clear buffers using a bitmask (e.g., COLOR_BUFFER_BIT)."""
    ...

def clear_color(r: float, g: float, b: float, a: float) -> None:
    """Set the clear color for the current framebuffer."""
    ...

def viewport(x: int, y: int, w: int, h: int) -> None:
    """Set the OpenGL viewport region."""
    ...

def bind_default_framebuffer() -> None:
    """Reverts rendering to the main window screen."""
    ...

# --- Classes ---

class Buffer:
    def __init__(self, size: int, data: Optional[BufferProtocol] = None, target: int = ..., usage: int = ...) -> None: ...
    def write(self, data: BufferProtocol, offset: int = 0) -> None: ...
    def bind_base(self, index: int) -> None: ...

class Texture:
    def __init__(self, target: int = ...) -> None: ...
    def upload(self, width: int, height: int, internal_format: int, format: int, type: int, 
               data: Optional[BufferProtocol] = None, level: int = 0, depth: int = 0) -> None: ...
    def bind(self, unit: int) -> None: ...
    def generate_mipmap(self) -> None: ...

class Program:
    def __init__(self, vertex_shader: str, fragment_shader: str) -> None: ...
    def get_uniform_location(self, name: str) -> int: ...

class VertexArray:
    def __init__(self) -> None: ...
    def bind_attribute(self, location: int, buffer: Buffer, size: int, type: int, 
                       normalized: int = 0, stride: int = 0, offset: int = 0) -> None: ...
    def bind_index_buffer(self, buffer: Buffer) -> None: ...

class UniformBatch:
    def __init__(self, max_bindings: int, max_bytes: int) -> None: ...
    def add(self, func_id: int, location: int, count: int, size: int) -> int: ...
    @property
    def data(self) -> memoryview: ...

class Pipeline:
    def __init__(self, program: Program, vao: VertexArray, 
                 topology: int = ..., index_type: int = ..., 
                 depth_test: int = 0, depth_write: int = 1, depth_func: int = ..., 
                 blend: int = 0, cull: int = 0) -> None: ...
    def upload_uniforms(self, batch: UniformBatch) -> None: ...
    def draw(self) -> None: ...
    @property
    def params(self) -> memoryview: ...

class Framebuffer:
    def __init__(self) -> None: ...
    def attach_texture(self, attachment: int, texture: Texture, level: int = 0) -> None: ...
    def check_status(self) -> bool: ...
    def bind(self) -> None: ...

# Explicitly export everything
__all__ = [
    "Buffer", "Texture", "Program", "VertexArray", "UniformBatch", "Pipeline", "Framebuffer",
    "init", "enable_debug", "context", "inspect", "clear", "clear_color", "viewport", 
    "bind_default_framebuffer",
    "FLOAT", "TRIANGLES", "UNSIGNED_BYTE", "UNSIGNED_SHORT", "UNSIGNED_INT",
    "UF_1I", "UF_1F", "UF_2F", "UF_3F", "UF_4F", "UF_MAT4",
    "ELEMENT_ARRAY_BUFFER", "UNIFORM_BUFFER", "STATIC_DRAW", "DYNAMIC_DRAW",
    "TEXTURE_2D", "TEXTURE_3D", "RGBA", "RGB", "RGBA8", 
    "DEPTH_COMPONENT", "DEPTH_COMPONENT24",
    "FRAMEBUFFER", "COLOR_ATTACHMENT0", "DEPTH_ATTACHMENT", "DEPTH_STENCIL_ATTACHMENT",
    "COLOR_BUFFER_BIT", "DEPTH_BUFFER_BIT",
    "FREE_THREADED", "DEBUG_BUILD"
]