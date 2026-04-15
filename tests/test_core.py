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


def test_context_capabilities(gl_context): # Pass the fixture to access the window
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


def test_buffer_bind_base():
    """Test binding to an indexed target."""
    buf = caravangl.Buffer(size=256, target=GL_UNIFORM_BUFFER)
    # Shouldn't crash. (Testing actual GL state binding would require feedback shaders)
    buf.bind_base(index=0)


def test_pipeline_creation_and_inspect():
    """Test pipeline initialization and internal representation."""
    # Assuming Program ID 1 and VAO ID 1 exist (OpenGL won't strictly validate IDs 
    # until draw time or state compilation, so this is safe to test)
    pipe = caravangl.Pipeline(
        program=1, 
        vao=1, 
        topology=GL_TRIANGLES, 
        index_type=GL_UNSIGNED_INT,
        depth_test=1
    )
    
    info = caravangl.inspect(pipe)
    
    assert info["type"] == "pipeline"
    assert info["program"] == 1
    assert info["vao"] == 1
    assert info["topology"] == GL_TRIANGLES
    assert info["index_type"] == GL_UNSIGNED_INT
    assert info["render_state"]["depth_test"] is True


def test_pipeline_memoryview_mutation():
    """Test the zero-overhead memoryview mutation of draw parameters."""
    pipe = caravangl.Pipeline(program=1, vao=1)
    
    # Get the memory view
    params = pipe.params
    assert isinstance(params, memoryview)
    assert len(params) == 4
    
    # Layout: [vertex_count, instance_count, first_vertex, base_instance]
    # Default is [0, 1, 0, 0]
    assert params[0] == 0
    assert params[1] == 1
    
    # Mutate parameters directly through the view
    params[0] = 36  # Set vertex_c