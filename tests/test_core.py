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

# --- Dummy Shaders for Testing ---
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


class GLLoader:
    """
    A compatible loader class that caravangl.init() expects.
    It takes a string and returns a raw C pointer as an integer.
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
    This runs once for the entire test session.
    """
    if not glfw.init():
        pytest.skip("Failed to initialize GLFW. Cannot run OpenGL tests.")

    # Request an invisible, modern OpenGL 3.3 Core profile context
    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    
    # On macOS, forward compatibility is required
    if sys.platform == "darwin":
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

    window = glfw.create_window(800, 600, "CaravanGL Test", None, None)
    if not window:
        glfw.terminate()
        pytest.skip("Failed to create GLFW window/context.")

    glfw.make_context_current(window)

    # Initialize CaravanGL with our loader
    loader = GLLoader()
    caravangl.init(loader=loader)

    yield window

    # Teardown
    glfw.destroy_window(window)
    glfw.terminate()


def test_context_capabilities(gl_context): 
    """Test that capabilities were correctly queried and populated."""
    ctx = caravangl.context()
    
    assert isinstance(ctx, dict)
    
    # Get the actual pixel dimensions from GLFW to compare against GL Viewport
    expected_w, expected_h = glfw.get_framebuffer_size(gl_context)
    
    # Assert against real hardware measurements
    assert ctx["viewport"][2] == expected_w
    assert ctx["viewport"][3] == expected_h
    
    assert ctx["caps"]["max_texture_size"] >= 1024
    assert isinstance(ctx["info"]["vendor"], str)


def test_buffer_creation_and_inspect():
    """Test creating an empty buffer and inspecting its internal C state."""
    buf = caravangl.Buffer(size=1024, target=GL_ARRAY_BUFFER, usage=GL_STATIC_DRAW)
    info = caravangl.inspect(buf)
    
    assert info["type"] == "buffer"
    assert info["id"] > 0
    assert info["size"] == 1024
    assert info["target"] == GL_ARRAY_BUFFER


def test_buffer_data_upload():
    """Test creating a buffer with initial data (tests the PyObject_GetBuffer fix)."""
    # 4 floats = 16 bytes
    data = struct.pack("4f", 1.0, 2.0, 3.0, 4.0)
    
    buf = caravangl.Buffer(size=16, data=data)
    info = caravangl.inspect(buf)
    
    assert info["size"] == 16


def test_buffer_write_bounds():
    """Test buffer writing with python bytearrays/memoryviews and bounds checking."""
    buf = caravangl.Buffer(size=16)
    
    # Write exactly 16 bytes
    buf.write(b"A" * 16, offset=0)
    
    # Write 4 bytes with offset 12 (exact edge)
    buf.write(bytearray(b"B" * 4), offset=12)

    # Expect ValueError if writing past the buffer boundary
    with pytest.raises(ValueError, match="exceeds buffer size"):
        buf.write(b"C" * 4, offset=14)

    # Expect TypeError if passing non-buffer data (e.g. integer)
    with pytest.raises(TypeError):
        buf.write(12345, offset=0)


def test_program_and_vao():
    """Test shader compilation and VAO generation."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    p_info = caravangl.inspect(prog)
    v_info = caravangl.inspect(vao)
    
    assert p_info["type"] == "program"
    assert p_info["id"] > 0
    
    assert v_info["type"] == "vertex_array"
    assert v_info["id"] > 0


def test_pipeline_creation_and_inspect():
    """Test pipeline initialization using true Objects."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()

    pipe = caravangl.Pipeline(
        program=prog, 
        vao=vao, 
        topology=caravangl.TRIANGLES, 
        index_type=GL_UNSIGNED_INT,
        depth_test=1
    )
    
    info = caravangl.inspect(pipe)
    
    assert info["type"] == "pipeline"
    # The pipeline should have extracted the internal IDs
    assert info["program"] == caravangl.inspect(prog)["id"]
    assert info["vao"] == caravangl.inspect(vao)["id"]
    assert info["topology"] == caravangl.TRIANGLES
    assert info["index_type"] == GL_UNSIGNED_INT
    assert info["render_state"]["depth_test"] is True


def test_pipeline_memoryview_mutation():
    """Test the zero-overhead memoryview mutation of draw parameters."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    # Get the memory view
    params = pipe.params
    assert isinstance(params, memoryview)
    assert len(params) == 4
    
    # Layout: [vertex_count, instance_count, first_vertex, base_instance]
    # Default is [0, 1, 0, 0]
    assert params[0] == 0
    assert params[1] == 1
    
    # Mutate parameters directly through the view
    params[0] = 36  # Set vertex_count to 36
    params[2] = 12  # Set first_vertex to 12
    
    # Inspect the internal C struct to prove the memoryview actually modified it
    info = caravangl.inspect(pipe)
    assert info["draw_params"]["vertex_count"] == 36
    assert info["draw_params"]["first_vertex"] == 12


def test_pipeline_draw_predictability():
    """
    Test the predictability early-out logic.
    If vertex_count or instance_count is 0, it should safely return without
    doing any work.
    """
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    # vertex_count is 0 by default. This should early-out safely.
    pipe.draw()
    
    # Change vertex count to >0, but instance_count to 0. Should also early-out.
    pipe.params[0] = 6
    pipe.params[1] = 0
    pipe.draw()