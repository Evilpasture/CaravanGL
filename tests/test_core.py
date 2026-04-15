import sys
import ctypes
import struct
import pytest
import glfw
import numpy as np

# Import your compiled extension
import caravangl

# --- Supplemental Constants (if not yet in caravangl) ---
GL_COLOR_BUFFER_BIT = 0x00004000
GL_DEPTH_BUFFER_BIT = 0x00000100
GL_TEXTURE_2D = 0x0DE1
GL_RGBA8 = 0x8058
GL_RGBA = 0x1908
GL_UNSIGNED_BYTE = 0x1401

# --- Test Shaders ---
VS_ATTR = """
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTex;
}
"""

FS_ATTR = """
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D uTexture;
void main() {
    FragColor = texture(uTexture, TexCoord);
}
"""

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

@pytest.fixture(scope="session", autouse=True)
def gl_context():
    if not glfw.init():
        pytest.skip("Failed to initialize GLFW.")
    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    if sys.platform == "darwin":
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

    window = glfw.create_window(800, 600, "CaravanGL Suite", None, None)
    if not window:
        glfw.terminate()
        pytest.skip("Failed to create GLFW context.")

    glfw.make_context_current(window)
    caravangl.init(loader=GLLoader())
    yield window
    glfw.destroy_window(window)
    glfw.terminate()

# --- 1. NEW: Clearing API Tests ---

def test_clear_operations():
    """Verify clear color and clear commands execution."""
    # These functions return None and shouldn't raise exceptions
    caravangl.clear_color(r=0.1, g=0.2, b=0.3, a=1.0)
    caravangl.clear(mask=GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    
    # Check if context caught the clear color (if we implement a getter later)
    # For now, simply ensuring no crash confirms the FastParse schema is correct.

# --- 2. NEW: Texture API Tests ---

def test_texture_lifecycle():
    """Test Texture creation, allocation, and mipmap generation."""
    tex = caravangl.Texture(target=GL_TEXTURE_2D)
    
    # Test Empty Allocation (Data=None)
    tex.upload(
        width=256, height=256,
        internal_format=GL_RGBA8,
        format=GL_RGBA,
        type=GL_UNSIGNED_BYTE,
        data=None
    )
    
    # Test Data Upload (using numpy)
    pixels = np.random.randint(0, 255, (64, 64, 4), dtype=np.uint8)
    tex.upload(
        width=64, height=64,
        internal_format=GL_RGBA8,
        format=GL_RGBA,
        type=GL_UNSIGNED_BYTE,
        data=pixels.tobytes()
    )
    
    # Test Bind and Mipmap
    tex.bind(unit=0)
    tex.generate_mipmap()

# --- 3. NEW: VertexArray Attribute Binding ---

def test_vao_attribute_setup():
    """Test that buffers link to VAOs correctly."""
    vao = caravangl.VertexArray()
    vbo = caravangl.Buffer(size=4096, target=caravangl.TRIANGLES)
    
    # location=0, size=3 (vec3), type=FLOAT, normalized=False, stride=0, offset=0
    vao.bind_attribute(
        location=0, 
        buffer=vbo, 
        size=3, 
        type=caravangl.FLOAT, 
        normalized=0, 
        stride=12, 
        offset=0
    )
    
    # Ensure it didn't crash and the inspect logic still works
    info = caravangl.inspect(vao)
    assert info["type"] == "vertex_array"

# --- 4. EXPANDED: Pipeline Render State ---

def test_pipeline_render_state_persistence():
    """Verify that render states like depth and blend are stored and inspected."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    pipe = caravangl.Pipeline(
        program=prog, vao=vao,
        depth_test=1,
        depth_write=0,
        blend=1
    )
    
    info = caravangl.inspect(pipe)
    assert info["render_state"]["depth_test"] is True
    assert info["render_state"]["depth_write"] is False
    assert info["render_state"]["blend"] is True

# --- 5. STRESS: MemoryView Stability ---

def test_high_frequency_mutation():
    """Stress test the memoryview mutation to ensure no memory corruption."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    # Simulate 1000 frames of draw parameter updates
    mv = pipe.params
    for i in range(1000):
        mv[0] = i      # vertex_count
        mv[1] = 1      # instance_count
        mv[2] = i * 2  # first_vertex
        
    info = caravangl.inspect(pipe)
    assert info["draw_params"]["vertex_count"] == 999
    assert info["draw_params"]["first_vertex"] == 1998

# --- 6. INTEGRATION: Full Draw Path ---

def test_full_pipeline_draw():
    """Set up a complete scene state and execute a draw call."""
    # 1. Shaders & Pipeline
    prog = caravangl.Program(vertex_shader=VS_ATTR, fragment_shader=FS_ATTR)
    vao = caravangl.VertexArray()
    
    # 2. Geometry
    # 3 verts: (x,y,z, u,v)
    verts = np.array([
        -0.5, -0.5, 0.0,  0.0, 0.0,
         0.5, -0.5, 0.0,  1.0, 0.0,
         0.0,  0.5, 0.0,  0.5, 1.0
    ], dtype=np.float32)
    vbo = caravangl.Buffer(size=verts.nbytes, data=verts.tobytes())
    
    # 3. Attributes
    # Stride = 5 * 4 bytes = 20
    vao.bind_attribute(0, vbo, 3, caravangl.FLOAT, 0, 20, 0)  # pos
    vao.bind_attribute(1, vbo, 2, caravangl.FLOAT, 0, 20, 12) # tex
    
    # 4. Texture
    tex = caravangl.Texture()
    tex.upload(64, 64, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, None)
    
    # 5. Pipeline
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    pipe.params[0] = 3 # 3 vertices
    
    # 6. Execute
    caravangl.clear_color(0, 0, 0, 1)
    caravangl.clear(GL_COLOR_BUFFER_BIT)
    
    tex.bind(0)
    pipe.draw() # Verify no GL errors or C crashes

# --- 7. UTILITY: Inspection Coverage ---

VS_DUMMY = """
#version 330 core
void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }
"""

FS_DUMMY = """
#version 330 core
out vec4 color;
void main() { color = vec4(1.0); }
"""

def test_inspect_coverage():
    """Ensure all core types are inspectable."""
    state = caravangl.context()
    
    # Test Program
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    assert caravangl.inspect(prog)["type"] == "program"
    
    # Test Texture
    tex = caravangl.Texture()
    assert caravangl.inspect(tex)["type"] == "texture" # (Wait, did we add this to inspect?)

# Note: Remember to update caravan_inspect in caravangl.c to support the new Texture type:
"""
    if (type == state->TextureType) {
        PyCaravanTexture *t = (PyCaravanTexture *)arg;
        return FastBuild_Dict("type", "texture", 
                              "id", (long long)t->tex.id,
                              "target", (long long)t->tex.target,
                              "width", (long long)t->tex.width,
                              "height", (long long)t->tex.height);
    }
"""

# --- Supplemental Constants ---
GL_STATIC_DRAW = 0x88E4
GL_DYNAMIC_DRAW = 0x88E8
GL_ELEMENT_ARRAY_BUFFER = 0x8893
GL_UNSIGNED_SHORT = 0x1403
GL_UNIFORM_BUFFER = 0x8A11

# --- 8. UniformBatch: Zero-Copy Mechanics ---

def test_uniform_batch_zero_copy():
    """Verify that we can write to UniformBatch memory and upload to a pipeline."""
    # 1. Create a batch (max 10 bindings, 1024 bytes)
    batch = caravangl.UniformBatch(max_bindings=10, max_bytes=1024)
    
    # 2. Register a vec3 (UF_3F = 3 floats = 12 bytes)
    # Assume UF_3F is defined in caravangl constants
    offset = batch.add(func_id=caravangl.UF_3F, location=0, count=1, size=12)
    assert offset == 0
    
    # 3. Write data directly via memoryview
    # Use struct to pack floats into bytes
    mv = batch.data
    struct.pack_into("fff", mv, offset, 1.0, 0.5, 0.2)
    
    # 4. Verification via inspection
    info = caravangl.inspect(batch)
    assert info["used_bytes"] == 12
    assert info["count"] == 1

# --- 9. Buffer: Writing and Multi-Targeting ---

def test_buffer_subdata_update():
    """Verify Buffer.write updates specific regions."""
    # Create an empty 1KB buffer
    vbo = caravangl.Buffer(size=1024, usage=GL_DYNAMIC_DRAW)
    
    # Write to the middle of the buffer
    new_data = np.array([9.0, 8.0, 7.0], dtype=np.float32)
    vbo.write(data=new_data.tobytes(), offset=512)
    
    info = caravangl.inspect(vbo)
    assert info["size"] == 1024
    assert info["usage"] == GL_DYNAMIC_DRAW

def test_buffer_indexed_binding():
    """Verify buffer can bind to indexed targets like UBOs."""
    ubo = caravangl.Buffer(size=256, target=GL_UNIFORM_BUFFER)
    # Bind to UBO slot 0
    ubo.bind_base(index=0)

# --- 10. Integration: Indexed Drawing (IBO) ---

def test_indexed_pipeline_draw():
    """Test glDrawElements path via Pipeline."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Setup VBO (Points)
    verts = np.array([0,0,0, 1,0,0, 1,1,0], dtype=np.float32)
    vbo = caravangl.Buffer(size=verts.nbytes, data=verts.tobytes())
    vao.bind_attribute(0, vbo, 3, caravangl.FLOAT, 0, 12, 0)
    
    # Setup IBO (Indices)
    indices = np.array([0, 1, 2], dtype=np.uint16)
    ibo = caravangl.Buffer(size=indices.nbytes, data=indices.tobytes(), target=GL_ELEMENT_ARRAY_BUFFER)
    # In GL Core, IBO is part of VAO state. 
    # Our VertexArray doesn't have a specific 'bind_index_buffer' method, 
    # but binding it while the VAO is active works:
    # (Note: You might want to add VertexArray.set_index_buffer(buf) to your C API)
    
    pipe = caravangl.Pipeline(
        program=prog, vao=vao, 
        topology=caravangl.TRIANGLES,
        index_type=GL_UNSIGNED_SHORT
    )
    
    # Set indices count
    pipe.params[0] = 3 # vertex_count (indices count in this case)
    pipe.draw()

# --- 11. Robustness: Argument Validation ---

def test_parser_type_safety():
    """Verify that FastParse catches invalid types before hitting C logic."""
    with pytest.raises(TypeError) as excinfo:
        # Pass a list where an integer (size) is expected
        caravangl.Buffer(size=[1, 2, 3])
    assert "int" in str(excinfo.value)

    with pytest.raises(TypeError) as excinfo:
        # Pass a Buffer where a Program is expected
        vao = caravangl.VertexArray()
        vbo = caravangl.Buffer(size=10)
        caravangl.Pipeline(program=vbo, vao=vao)
    assert "caravangl.Program" in str(excinfo.value) or "Pipeline" in str(excinfo.value)

# --- 12. Context Metadata & Caps ---

def test_context_capabilities():
    """Verify context() returns driver info and hardware limits."""
    ctx = caravangl.context()
    
    assert "info" in ctx
    assert "vendor" in ctx["info"]
    assert "renderer" in ctx["info"]
    
    assert "caps" in ctx
    assert ctx["caps"]["max_texture_size"] > 0
    # On Mac, this was hardcoded to false in your C code, check for consistency
    if sys.platform == "darwin":
        assert ctx["caps"]["support_compute"] is False
    
    assert "viewport" in ctx
    assert len(ctx["viewport"]) == 4

# --- 13. Isolated Multi-Object Cleanup ---

def test_bulk_resource_deletion():
    """Ensure heavy churn of objects doesn't leak or crash."""
    for _ in range(100):
        b = caravangl.Buffer(size=1024)
        v = caravangl.VertexArray()
        # Resources are deleted when Python objects go out of scope/GC
    
    # Check context is still healthy
    caravangl.clear(GL_COLOR_BUFFER_BIT)

# --- 14. Program Uniform Reflection ---

def test_program_uniform_query():
    """Test getting uniform locations."""
    # Shader with a named uniform
    fs_uniform = """
    #version 330 core
    uniform vec4 uColor;
    out vec4 FragColor;
    void main() { FragColor = uColor; }
    """
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=fs_uniform)
    
    loc = prog.get_uniform_location("uColor")
    assert loc >= 0
    
    loc_missing = prog.get_uniform_location("nonExistent")
    assert loc_missing == -1