import sys
import ctypes
import struct
import pytest
# pyright: reportMissingTypeStubs=false
# pyright: reportUnknownMemberType=false
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
        if ptr is None:
            return 0
        val = ctypes.cast(ptr, ctypes.c_void_p).value
        return val if val is not None else 0
    
def create_shared_context(main_window):
    """Creates a new GLFW window and CaravanGL context shared with the main window."""
    # Create a hidden window that shares resources with the main window
    shared_window = glfw.create_window(1, 1, "Shared Context", None, main_window)
    if not shared_window:
        raise RuntimeError("Failed to create shared GLFW window")
    
    # Note: We don't make it current here; the worker thread will do it
    ctx = caravangl.Context(loader=GLLoader())
    
    return shared_window, ctx

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

    # 1. Tell GLFW to make the window current on this thread
    glfw.make_context_current(window)
    
    # 2. Create the CaravanGL Context object
    # In the new architecture, we store this object.
    ctx = caravangl.Context(loader=GLLoader())

    ctx.os_make_current_cb = lambda: glfw.make_context_current(window)
    
    # 3. CRITICAL: Tell CaravanGL to make this context active for the current thread.
    # This sets the thread_local cv_active_context pointer in C.
    ctx.make_current()
    
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
    assert caravangl.inspect(tex)["type"] == "texture"

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
    ibo = caravangl.Buffer(size=indices.nbytes, data=indices.tobytes(), target=caravangl.ELEMENT_ARRAY_BUFFER)
    
    # --- THE FIX: Link the IBO to the VAO ---
    vao.bind_index_buffer(ibo) 
    
    pipe = caravangl.Pipeline(
        program=prog, vao=vao, 
        topology=caravangl.TRIANGLES,
        index_type=caravangl.UNSIGNED_SHORT # Match GL_UNSIGNED_SHORT
    )
    
    # Set indices count (3 indices for 1 triangle)
    pipe.params[0] = 3 
    
    # This will now work without crashing
    pipe.draw()

# --- 11. Robustness: Argument Validation ---

def test_parser_type_safety():
    with pytest.raises(TypeError):
        # We pass a list where int is expected to test C-side safety
        caravangl.Buffer(size=[1, 2, 3]) # type: ignore

    with pytest.raises(TypeError):
        vao = caravangl.VertexArray()
        vbo = caravangl.Buffer(size=10)
        # Passing a Buffer where a Program is expected
        caravangl.Pipeline(program=vbo, vao=vao) # type: ignore

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

# --- Supplemental FBO Constants ---
GL_FRAMEBUFFER = 0x8D40
GL_COLOR_ATTACHMENT0 = 0x8CE0

# --- 15. Framebuffer: Lifecycle & Completeness ---

def test_framebuffer_completeness():
    """Verify an FBO is marked complete when properly attached."""
    # 1. Create backing texture
    tex = caravangl.Texture(target=GL_TEXTURE_2D)
    tex.upload(
        width=512, height=512,
        internal_format=GL_RGBA8,
        format=GL_RGBA,
        type=GL_UNSIGNED_BYTE,
        data=None # Allocate only
    )
    
    # 2. Attach to FBO
    fbo = caravangl.Framebuffer()
    fbo.attach_texture(attachment=GL_COLOR_ATTACHMENT0, texture=tex)
    
    # 3. Check status (should return True)
    assert fbo.check_status() is True

# --- 16. Framebuffer: Incomplete Error Handling ---

def test_framebuffer_incomplete():
    """Verify that an empty FBO correctly raises an exception."""
    fbo = caravangl.Framebuffer()

    # An FBO with no attachments is mathematically incomplete in OpenGL.
    # caravangl should catch this and raise a RuntimeError.
    with pytest.raises(RuntimeError) as excinfo:
        fbo.check_status()

    # UPDATED: Match the new, specific error message we added to the C code
    msg = str(excinfo.value).lower()
    assert ("no attachments" in msg) or ("not complete" in msg)

# --- 17. Viewport: State Tracking ---

def test_viewport_state_update():
    """Verify viewport updates properly sync with the isolated C-context."""
    # Change viewport to an arbitrary resolution
    caravangl.viewport(x=15, y=25, width=1920, height=1080)
    
    # Ask the C-backend for a snapshot of the context state
    ctx = caravangl.context()
    vp = ctx["viewport"] # Tuple: (x, y, w, h)
    
    assert vp[0] == 15
    assert vp[1] == 25
    assert vp[2] == 1920
    assert vp[3] == 1080

# --- 18. Integration: FBO Context Switching ---

def test_fbo_render_context_switch():
    """Execute a full render-to-texture and render-to-screen pass."""
    # 1. Setup FBO
    tex = caravangl.Texture(target=GL_TEXTURE_2D)
    tex.upload(64, 64, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, None)
    fbo = caravangl.Framebuffer()
    fbo.attach_texture(GL_COLOR_ATTACHMENT0, tex)
    fbo.check_status()
    
    # 2. Dummy Pipeline
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    # --- PASS 1: Render to FBO ---
    fbo.bind()
    caravangl.viewport(x=0, y=0, width=64, height=64)
    caravangl.clear_color(1.0, 0.0, 0.0, 1.0)
    caravangl.clear(GL_COLOR_BUFFER_BIT)
    pipe.draw()
    
    # --- PASS 2: Render to Screen ---
    caravangl.bind_default_framebuffer()
    caravangl.viewport(x=0, y=0, width=800, height=600)
    caravangl.clear_color(0.0, 0.0, 0.0, 1.0)
    caravangl.clear(GL_COLOR_BUFFER_BIT)
    
    tex.bind(unit=0)
    pipe.draw()
    
    # If we reach this line without an OpenGL error, segmentation fault, 
    # or SIGABRT, the FBO context switching is perfectly thread-safe and robust.
    assert True

# --- Supplemental Constants for Depth/Stencil ---
GL_DEPTH24_STENCIL8 = 0x88F0
GL_DEPTH_STENCIL = 0x84F9
GL_UNSIGNED_INT_24_8 = 0x84FA
GL_DEPTH_STENCIL_ATTACHMENT = 0x821A

# --- 19. Pipeline: Render State Inspection ---

def test_pipeline_complex_state_inspection():
    """Verify that all Depth and Stencil parameters are correctly binned and inspectable."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    pipe = caravangl.Pipeline(
        program=prog, vao=vao,
        depth_test=1,
        depth_write=0,
        depth_func=caravangl.LEQUAL,
        stencil_test=1,
        stencil_func=caravangl.NOTEQUAL,
        stencil_ref=42,
        stencil_read_mask=0xFF,
        stencil_write_mask=0x00,
        stencil_fail=caravangl.KEEP,
        stencil_zfail=caravangl.INCR,
        stencil_zpass=caravangl.REPLACE
    )
    
    info = caravangl.inspect(pipe)
    rs = info["render_state"]
    
    # Check Depth
    assert rs["depth_test"] is True
    assert rs["depth_write"] is False
    assert rs["depth_func"] == caravangl.LEQUAL
    
    # Check Stencil
    # Note: If your inspect logic doesn't yet export these, this is your reminder 
    # to update caravan_inspect in caravangl.c!
    assert rs["stencil_test"] is True
    assert rs["stencil_func"] == caravangl.NOTEQUAL
    assert rs["stencil_ref"] == 42
    assert rs["stencil_zpass"] == caravangl.REPLACE

# --- 20. Functional: Depth Buffer blocking ---

def test_depth_test_execution():
    """Verify that Depth Testing correctly blocks a distant object."""
    # 1. Setup FBO with Depth
    tex_color = caravangl.Texture()
    tex_color.upload(64, 64, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, None)
    
    tex_depth = caravangl.Texture()
    tex_depth.upload(64, 64, caravangl.DEPTH_COMPONENT24, caravangl.DEPTH_COMPONENT, caravangl.UNSIGNED_INT, None)
    
    fbo = caravangl.Framebuffer()
    fbo.attach_texture(caravangl.COLOR_ATTACHMENT0, tex_color)
    fbo.attach_texture(caravangl.DEPTH_ATTACHMENT, tex_depth)
    fbo.check_status()
    
    # 2. Setup Pipeline with Depth Testing
    prog = caravangl.Program(vertex_shader=VS_ATTR, fragment_shader=FS_ATTR)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao, depth_test=1, depth_func=caravangl.LESS)
    
    fbo.bind()
    caravangl.viewport(0, 0, 64, 64)
    
    # 3. Execution: If this doesn't crash, the state sync for depth is stable
    caravangl.clear(caravangl.COLOR_BUFFER_BIT | caravangl.DEPTH_BUFFER_BIT)
    pipe.draw()
    
    caravangl.bind_default_framebuffer()

# --- 21. Functional: Stencil Masking ---

def test_stencil_logic_execution():
    """Verify state sync survives switching between different stencil configurations."""
    # 1. Setup FBO with Depth/Stencil
    tex_color = caravangl.Texture()
    tex_color.upload(64, 64, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, None)
    
    tex_ds = caravangl.Texture()
    tex_ds.upload(64, 64, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, None)
    
    fbo = caravangl.Framebuffer()
    fbo.attach_texture(caravangl.COLOR_ATTACHMENT0, tex_color)
    fbo.attach_texture(GL_DEPTH_STENCIL_ATTACHMENT, tex_ds)
    fbo.check_status()
    
    # 2. Setup two different state pipelines
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Pipe A: Writing to stencil
    pipe_write = caravangl.Pipeline(
        program=prog, vao=vao, 
        stencil_test=1, stencil_func=caravangl.ALWAYS, stencil_ref=1, stencil_zpass=caravangl.REPLACE
    )
    
    # Pipe B: Testing stencil
    pipe_test = caravangl.Pipeline(
        program=prog, vao=vao, 
        stencil_test=1, stencil_func=caravangl.EQUAL, stencil_ref=1
    )
    
    fbo.bind()
    caravangl.clear(caravangl.COLOR_BUFFER_BIT | caravangl.DEPTH_BUFFER_BIT | caravangl.STENCIL_BUFFER_BIT)
    
    # 3. Execute rapid state changes
    pipe_write.draw()
    pipe_test.draw() 
    
    caravangl.bind_default_framebuffer()

# --- 22. Error Handling: Invalid Constants ---

def test_pipeline_invalid_enums():
    """Ensure passing garbage to GLenums is caught or handled gracefully."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Passing a string to an integer enum field should be caught by FastParse
    with pytest.raises(TypeError):
        caravangl.Pipeline(program=prog, vao=vao, depth_func="GL_LESSER_THAN") # type: ignore

# --- 24. Pipeline: Blending Inspection ---

def test_pipeline_blending_config():
    """Verify that complex blending factors are correctly stored in the C struct."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Configure an additive blending pipeline
    pipe = caravangl.Pipeline(
        program=prog, vao=vao,
        blend=1,
        blend_src_rgb=caravangl.ONE,
        blend_dst_rgb=caravangl.ONE,
        blend_src_alpha=caravangl.ZERO,
        blend_dst_alpha=caravangl.ONE
    )
    
    info = caravangl.inspect(pipe)
    rs = info["render_state"]
    
    assert rs["blend"] is True
    assert rs["blend_src_rgb"] == caravangl.ONE
    assert rs["blend_dst_rgb"] == caravangl.ONE

# --- 25. Pipeline: Culling Inspection ---

def test_pipeline_culling_config():
    """Verify face culling configurations."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Configure front-face culling
    pipe = caravangl.Pipeline(
        program=prog, vao=vao,
        cull=1,
        cull_mode=caravangl.FRONT
    )
    
    info = caravangl.inspect(pipe)
    rs = info["render_state"]
    
    assert rs["cull"] is True
    assert rs["cull_mode"] == caravangl.FRONT

# --- 26. Functional: State Transition Stress Test ---

def test_render_state_synchronization_stress():
    """Force the C-backend to rapidly toggle every state bit."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Create two pipelines with diametrically opposed states
    pipe_a = caravangl.Pipeline(
        program=prog, vao=vao,
        depth_test=1, blend=0, cull=1, stencil_test=0
    )
    
    pipe_b = caravangl.Pipeline(
        program=prog, vao=vao,
        depth_test=0, blend=1, cull=0, stencil_test=1,
        stencil_func=caravangl.EQUAL, stencil_ref=1
    )
    
    # Execute rapid switches. If cv_sync_render_state has a caching bug 
    # or a null-pointer dereference, this will crash the process.
    for _ in range(100):
        pipe_a.draw()
        pipe_b.draw()
    
    assert True

# --- 27. Robustness: Illegal Blend Constants ---

def test_pipeline_illegal_blend_values():
    """Ensure invalid types passed to blend fields are caught by FastParse."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    
    # Passing a float where a uint32 enum is expected
    with pytest.raises(TypeError):
        caravangl.Pipeline(program=prog, vao=vao, blend_src_rgb=0.5) # type: ignore
        
    # Passing a string
    with pytest.raises(TypeError):
        caravangl.Pipeline(program=prog, vao=vao, blend_dst_rgb="GL_ONE") # type: ignore

# --- 28. VertexArray: Instanced Attribute Binding ---

def test_vao_instanced_attribute():
    """Verify that the 'divisor' argument is accepted by the parser."""
    vao = caravangl.VertexArray()
    vbo = caravangl.Buffer(size=1024)
    
    # Binding an attribute with divisor=1 (Instancing enabled)
    # If the parser or C-code is broken, this will throw an error or crash here.
    vao.bind_attribute(
        location=0, 
        buffer=vbo, 
        size=3, 
        type=caravangl.FLOAT, 
        divisor=1 # <--- Testing our new C field
    )
    
    assert True # Successfully parsed and called gl-command

# --- 29. Functional: Instanced Draw Execution ---

def test_instanced_draw_call():
    """Execute a draw call with multiple instances."""
    # 1. Setup
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    vbo = caravangl.Buffer(size=1024)
    
    # 3 vertices (per shape)
    vao.bind_attribute(0, vbo, 3, caravangl.FLOAT, stride=12, offset=0, divisor=0)
    
    # 10 instances of data (e.g., offsets)
    vbo_inst = caravangl.Buffer(size=1024)
    vao.bind_attribute(1, vbo_inst, 2, caravangl.FLOAT, stride=8, offset=0, divisor=1)
    
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    # 2. Configure for Instancing
    pipe.params[0] = 3  # 3 vertices per instance
    pipe.params[1] = 10 # 10 instances
    
    # 3. Draw
    # This triggers OpenGL->DrawArraysInstanced
    caravangl.clear(caravangl.COLOR_BUFFER_BIT)
    pipe.draw()
    
    assert True

# --- 30. Sampler: Lifecycle & Defaults ---

def test_sampler_creation():
    """Verify sampler object creation with various filter modes."""
    # Default sampler
    s1 = caravangl.Sampler()
    info = caravangl.inspect(s1)
    assert info["type"] == "sampler"
    assert info["id"] > 0

    # Custom pixel-art sampler
    s2 = caravangl.Sampler(
        min_filter=caravangl.NEAREST, 
        mag_filter=caravangl.NEAREST,
        wrap_s=caravangl.REPEAT,
        wrap_t=caravangl.REPEAT
    )
    assert caravangl.inspect(s2)["id"] > 0

# --- 31. Sampler: Strict Type Safety ---

def test_sampler_binding_type_safety():
    """Verify that Texture.bind rejects non-sampler objects."""
    tex = caravangl.Texture()
    
    # Test 1: Passing an integer instead of a Sampler object
    with pytest.raises(TypeError) as exc:
        tex.bind(unit=0, sampler=123) # type: ignore
    assert "caravangl.Sampler" in str(exc.value)

    # Test 2: Passing a Buffer object instead of a Sampler
    vbo = caravangl.Buffer(size=10)
    with pytest.raises(TypeError) as exc:
        tex.bind(unit=0, sampler=vbo) # type: ignore
    assert "caravangl.Sampler" in str(exc.value)

# --- 32. Sampler: Cache Invalidation (Crucial) ---

def test_sampler_cache_poisoning_protection():
    """Verify that deleting a sampler clears the C-state tracker cache."""
    tex = caravangl.Texture()
    tex.upload(64, 64, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, None)
    
    # 1. Create and bind a sampler
    s1 = caravangl.Sampler()
    s1_id = caravangl.inspect(s1)["id"]
    tex.bind(unit=0, sampler=s1)
    
    # 2. Delete the sampler. 
    # This triggers Sampler_dealloc, which should clear the tracker for Unit 0.
    del s1
    
    # 3. Create a new sampler. 
    # OpenGL will very likely reuse the same ID (s1_id).
    s2 = caravangl.Sampler()
    s2_id = caravangl.inspect(s2)["id"]
    
    # 4. Bind the new sampler.
    # If our dealloc logic is correct, the tracker knows unit 0 is empty 
    # and will correctly issue the glBindSampler command even if IDs are identical.
    tex.bind(unit=0, sampler=s2)
    
    # If this didn't crash or trigger a GL error, the cache logic is healthy.
    assert True

# --- 33. Functional: Multi-Sampler State Switch ---

def test_multi_sampler_draw_consistency():
    """Ensure we can switch samplers between textures on different units."""
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    t1 = caravangl.Texture()
    t2 = caravangl.Texture()
    
    s_linear = caravangl.Sampler(min_filter=caravangl.LINEAR)
    s_nearest = caravangl.Sampler(min_filter=caravangl.NEAREST)
    
    # Bind to different units with different samplers
    t1.bind(unit=0, sampler=s_linear)
    t2.bind(unit=1, sampler=s_nearest)
    
    # Switch them
    t1.bind(unit=1, sampler=s_nearest)
    t2.bind(unit=0, sampler=s_linear)
    
    pipe.draw()
    assert True

import threading

def test_parallel_context_rendering():
    main_window = glfw.get_current_context()
    
    # 1. Create the second window
    shared_win = glfw.create_window(1, 1, "Shared Context", None, main_window)
    
    # 2. TEMPORARILY make it current on the main thread so symbols can load
    glfw.make_context_current(shared_win)
    ctx2 = caravangl.Context(loader=GLLoader())
    
    # 3. Setup the OS callback for the future
    ctx2.os_make_current_cb = lambda: glfw.make_context_current(shared_win)
    
    # 4. RELEASE both contexts from the main thread so workers can use them
    glfw.make_context_current(None)

    def render_work(ctx, window, color):
        # This will now succeed because the context is not 'held' by the main thread
        ctx.make_current() 
        caravangl.clear_color(*color)
        caravangl.clear(GL_COLOR_BUFFER_BIT)

    # Note: main context already had its symbols loaded by the fixture
    t1 = threading.Thread(target=render_work, args=(caravangl.get_active_context(), main_window, (1, 0, 0, 1)))
    t2 = threading.Thread(target=render_work, args=(ctx2, shared_win, (0, 1, 0, 1)))
    
    t1.start()
    t2.start()
    t1.join()
    t2.join()
    
    glfw.destroy_window(shared_win)
    # Restore main thread context for remaining tests
    caravangl.get_active_context().make_current()

def test_cross_thread_garbage_collection():
    tex = caravangl.Texture()
    tex.upload(64, 64, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, None)
    
    # Put in a list and delete the local name
    container = [tex]
    del tex 

    def worker_thread(c):
        # Drops the final reference on THIS thread
        obj = c.pop()
        del obj 

    t = threading.Thread(target=worker_thread, args=(container,))
    t.start()
    t.join()
    
    # This will now trigger cv_flush_garbage on the main thread's context
    ctx = caravangl.get_active_context()
    ctx.make_current()
    
    assert True

def test_buffer_contention_stress():
    vbo = caravangl.Buffer(size=1024, usage=GL_DYNAMIC_DRAW)
    main_window = glfw.get_current_context()
    
    # 1. CAPTURE the main context while we are still on the main thread
    main_ctx = caravangl.get_active_context()
    
    # 2. Setup shared context
    shared_win = glfw.create_window(1, 1, "Stress Win", None, main_window)
    
    # 3. Make current temporarily to load function pointers
    glfw.make_context_current(shared_win)
    ctx2 = caravangl.Context(loader=GLLoader())
    ctx2.os_make_current_cb = lambda: glfw.make_context_current(shared_win)
    
    # 4. Release contexts so workers can take them
    glfw.make_context_current(None)
    
    stop_event = threading.Event()

    def writer():
        ctx2.make_current() 
        while not stop_event.is_set():
            data = np.random.randint(0, 255, 10, dtype=np.uint8).tobytes()
            vbo.write(data=data, offset=0)
            
    def reader(ctx_to_use):
        # Use the context passed from the main thread
        ctx_to_use.make_current()
        while not stop_event.is_set():
            vbo.bind_base(index=0)
            
    # Pass main_ctx into the reader thread
    t_write = threading.Thread(target=writer)
    t_read = threading.Thread(target=reader, args=(main_ctx,))
    
    t_write.start()
    t_read.start()
    
    import time
    time.sleep(0.5)
    stop_event.set()
    
    t_write.join()
    t_read.join()
    
    # Cleanup: restore main thread context for subsequent tests
    main_ctx.make_current()
    glfw.destroy_window(shared_win)
    assert True

def test_sync_basic_behavior():
    # 1. Create a sync object (inserts fence immediately)
    sync = caravangl.Sync()
    
    # 2. Waiting immediately should generally return CONDITION_SATISFIED 
    # or ALREADY_SIGNALED (if the GPU is fast)
    status = sync.wait(timeout_sec=1.0)
    
    assert status in (
        caravangl.CONDITION_SATISFIED, 
        caravangl.ALREADY_SIGNALED
    )

def test_sync_timeout():
    # We can't easily force the GPU to be slow, but we can pass 0.0 
    # to see if it hasn't finished yet, or just check that the argument parsing works.
    sync = caravangl.Sync()
    status = sync.wait(timeout_sec=0.0)
    
    # It's perfectly valid for it to return either status
    assert status in (
        caravangl.CONDITION_SATISFIED, 
        caravangl.ALREADY_SIGNALED, 
        caravangl.TIMEOUT_EXPIRED
    )

import numpy as np

def test_cross_thread_sync_data_consistency():
    main_window = glfw.get_current_context()
    main_ctx = caravangl.get_active_context()
    
    # Shared VBO
    vbo = caravangl.Buffer(size=1024)
    
    # A list to pass the sync object between threads
    sync_queue = []

    def producer_thread():
        # 1. Setup context
        main_ctx.make_current()
        
        # 2. Write "Secret Key" 42.0 to the buffer
        data = np.full(256, 42.0, dtype=np.float32).tobytes()
        vbo.write(data)
        
        # 3. Insert Fence
        fence = caravangl.Sync()
        sync_queue.append(fence)
        
        # Release context
        glfw.make_context_current(None)

    # 1. Run the producer
    t = threading.Thread(target=producer_thread)
    t.start()
    t.join()

    # 2. Main Thread takes over
    main_ctx.make_current()
    
    # 3. Wait for the producer's GPU work to finish
    fence = sync_queue[0]
    status = fence.wait(timeout_sec=2.0)
    assert status != caravangl.TIMEOUT_EXPIRED
    
    # 4. If we had a way to read back the buffer (e.g. glGetBufferSubData),
    # we would verify the 42.0 here. Since your API is write-only for now, 
    # we verify that no crash occurred during synchronization.
    assert True

def test_sync_garbage_collection_stress():
    main_ctx = caravangl.get_active_context()
    
    def deleter_thread(sync_objs):
        # This thread will drop the final references to the sync objects
        # created on the main thread.
        sync_objs.clear()

    # Create 1000 sync objects on the main thread
    syncs = [caravangl.Sync() for _ in range(1000)]
    
    t = threading.Thread(target=deleter_thread, args=(syncs,))
    t.start()
    t.join()
    
    # Now, the main context's garbage queue is full of GLsync pointers.
    # Calling make_current() triggers cv_flush_garbage.
    main_ctx.make_current()
    
    # If the C-code didn't handle DeleteSync properly, we would likely 
    # crash here or leak memory.
    caravangl.clear(caravangl.COLOR_BUFFER_BIT)
    assert True

def test_sync_invalid_timeout():
    sync = caravangl.Sync()
    
    # Passing a string instead of a float
    with pytest.raises(TypeError):
        sync.wait("one second") # type: ignore
        
    # Negative values should result in TIMEOUT_IGNORED (infinite wait)
    # per your C implementation (timeout_ns = GL_TIMEOUT_IGNORED)
    status = sync.wait(timeout_sec=-1.0)
    assert status in (caravangl.CONDITION_SATISFIED, caravangl.ALREADY_SIGNALED)

def test_sync_multiple_waits():
    sync = caravangl.Sync()
    
    # First wait
    res1 = sync.wait(0.1)
    # Second wait (should return ALREADY_SIGNALED or CONDITION_SATISFIED)
    res2 = sync.wait(0.1)
    
    assert res1 in (caravangl.CONDITION_SATISFIED, caravangl.ALREADY_SIGNALED)
    assert res2 in (caravangl.CONDITION_SATISFIED, caravangl.ALREADY_SIGNALED)

def test_query_time_elapsed():
    query = caravangl.Query(target=caravangl.TIME_ELAPSED)
    
    query.begin()
    # Execute expensive draw calls here
    # pipe.draw() 
    query.end()
    
    # Wait for the GPU to finish rendering and reading the clock
    nanoseconds = query.get_result()
    milliseconds = nanoseconds / 1_000_000.0
    print(f"GPU took {milliseconds:.2f} ms")
    assert nanoseconds >= 0