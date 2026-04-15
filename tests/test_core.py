import sys
import ctypes
import struct
import pytest
import glfw

# Import your compiled extension
import caravangl

# --- Basic OpenGL Constants ---
GL_ARRAY_BUFFER = 0x8892
GL_UNIFORM_BUFFER = 0x8A11
GL_STATIC_DRAW = 0x88E4
GL_TRIANGLES = 0x0004
GL_UNSIGNED_INT = 0x1405
GL_FLOAT = 0x1406

# --- Shaders for Testing ---
VS_DUMMY = """
#version 330 core
void main() {
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}
"""

FS_DUMMY = """
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0);
}
"""

VS_UNIFORMS = """
#version 330 core
uniform mat4 uProjection;
uniform vec4 uColor;
void main() {
    gl_Position = uProjection * vec4(1.0);
}
"""

FS_UNIFORMS = """
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() {
    FragColor = uColor;
}
"""

class GLLoader:
    """
    A compatible loader class that caravangl.init() expects.
    """
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        if ptr:
            return ctypes.cast(ptr, ctypes.c_void_p).value
        return 0


@pytest.fixture(scope="session", autouse=True)
def gl_context():
    """
    Creates a headless GLFW window and initializes CaravanGL.
    """
    if not glfw.init():
        pytest.skip("Failed to initialize GLFW.")

    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    
    if sys.platform == "darwin":
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

    window = glfw.create_window(800, 600, "CaravanGL Test", None, None)
    if not window:
        glfw.terminate()
        pytest.skip("Failed to create GLFW context.")

    glfw.make_context_current(window)

    loader = GLLoader()
    caravangl.init(loader=loader)

    yield window
    glfw.destroy_window(window)
    glfw.terminate()


def test_context_capabilities(gl_context): 
    """Test that capabilities were correctly queried and populated."""
    ctx = caravangl.context()
    assert isinstance(ctx, dict)
    expected_w, expected_h = glfw.get_framebuffer_size(gl_context)
    assert ctx["viewport"][2] == expected_w
    assert ctx["viewport"][3] == expected_h
    assert ctx["caps"]["max_texture_size"] >= 1024


def test_buffer_creation_and_inspect():
    """Test Buffer C-state."""
    buf = caravangl.Buffer(size=1024, target=GL_ARRAY_BUFFER, usage=GL_STATIC_DRAW)
    info = caravangl.inspect(buf)
    assert info["type"] == "buffer"
    assert info["size"] == 1024


def test_buffer_data_upload():
    """Test Buffer initial data upload."""
    data = struct.pack("4f", 1.0, 2.0, 3.0, 4.0)
    buf = caravangl.Buffer(size=16, data=data)
    info = caravangl.inspect(buf)
    assert info["size"] == 16


def test_buffer_write_bounds():
    """Test Buffer safety checks."""
    buf = caravangl.Buffer(size=16)
    buf.write(b"A" * 16, offset=0)
    with pytest.raises(ValueError, match="exceeds buffer size"):
        buf.write(b"C" * 4, offset=14)


def test_program_and_vao():
    """Test shader compilation and VAO generation."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    p_info = caravangl.inspect(prog)
    v_info = caravangl.inspect(vao)
    assert p_info["type"] == "program"
    assert v_info["type"] == "vertex_array"


def test_pipeline_creation_and_inspect():
    """Test pipeline initialization using true Objects."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(
        program=prog, vao=vao, 
        topology=caravangl.TRIANGLES, 
        index_type=GL_UNSIGNED_INT
    )
    info = caravangl.inspect(pipe)
    assert info["program"] == caravangl.inspect(prog)["id"]
    assert info["vao"] == caravangl.inspect(vao)["id"]


def test_pipeline_memoryview_mutation():
    """Test zero-overhead memoryview mutation of draw parameters."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    pipe.params[0] = 36 # vertex_count
    pipe.params[2] = 12 # first_vertex
    
    info = caravangl.inspect(pipe)
    assert info["draw_params"]["vertex_count"] == 36
    assert info["draw_params"]["first_vertex"] == 12


def test_pipeline_draw_predictability():
    """Test the predictability early-out logic."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    pipe.draw() # vertex_count is 0, should do nothing safely
    
    pipe.params[0] = 6
    pipe.params[1] = 0 # instance_count is 0, should do nothing safely
    pipe.draw()


def test_uniform_batch_creation():
    """Test UniformBatch creation and memory exposure."""
    # 2 bindings, 128 bytes of payload
    batch = caravangl.UniformBatch(max_bindings=2, max_bytes=128)
    
    assert isinstance(batch.data, memoryview)
    assert len(batch.data) == 128
    
    # Register a dummy binding
    offset = batch.add(func_id=caravangl.UF_4F, location=0, count=1, size=16)
    assert offset == 0
    
    # Second binding should start after the first
    offset2 = batch.add(func_id=caravangl.UF_1F, location=1, count=1, size=4)
    assert offset2 == 16


def test_pipeline_uniform_dispatch():
    """Test zero-copy uniform upload through the pipeline."""
    prog = caravangl.Program(vertex_shader=VS_UNIFORMS, fragment_shader=FS_UNIFORMS)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    loc_proj = prog.get_uniform_location("uProjection")
    loc_color = prog.get_uniform_location("uColor")
    
    # Setup batch
    batch = caravangl.UniformBatch(max_bindings=2, max_bytes=80)
    proj_off = batch.add(func_id=caravangl.UF_MAT4, location=loc_proj, count=1, size=64)
    col_off  = batch.add(func_id=caravangl.UF_4F, location=loc_color, count=1, size=16)
    
    # Map to Float32 view for zero-copy update
    f32 = batch.data.cast('f')
    
    # Set Identity Matrix (Individual assignment is fast and zero-copy)
    proj_idx = proj_off // 4
    for i in range(16): 
        f32[proj_idx + i] = 0.0
    f32[proj_idx] = f32[proj_idx + 5] = f32[proj_idx + 10] = f32[proj_idx + 15] = 1.0
    
    # Set Red Color
    col_idx = col_off // 4
    # FIX: Assign individually to avoid "bytes-like object" error
    f32[col_idx]     = 1.0
    f32[col_idx + 1] = 0.0
    f32[col_idx + 2] = 0.0
    f32[col_idx + 3] = 1.0
    
    # Dispatch - if memory layout is wrong, this will likely crash C-side
    pipe.upload_uniforms(batch=batch)