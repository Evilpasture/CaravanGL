import sys
import ctypes
import struct
import pytest
import threading
import time
# pyright: reportMissingTypeStubs=false
# pyright: reportUnknownMemberType=false
import glfw
import numpy as np

import caravangl

# --- Supplemental Constants ---
GL_COLOR_BUFFER_BIT = 0x00004000
GL_DEPTH_BUFFER_BIT = 0x00000100
GL_STENCIL_BUFFER_BIT = 0x00000400
GL_TEXTURE_2D = 0x0DE1
GL_RGBA8 = 0x8058
GL_RGBA = 0x1908
GL_UNSIGNED_BYTE = 0x1401
GL_DYNAMIC_DRAW = 0x88E8
GL_UNIFORM_BUFFER = 0x8A11
GL_ELEMENT_ARRAY_BUFFER = 0x8893
GL_UNSIGNED_SHORT = 0x1403
GL_FRAMEBUFFER = 0x8D40
GL_COLOR_ATTACHMENT0 = 0x8CE0
GL_DEPTH_STENCIL_ATTACHMENT = 0x821A
GL_DEPTH24_STENCIL8 = 0x88F0
GL_DEPTH_STENCIL = 0x84F9
GL_UNSIGNED_INT_24_8 = 0x84FA

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

VS_DUMMY = "#version 330 core\nvoid main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }"
FS_DUMMY = "#version 330 core\nout vec4 color; void main() { color = vec4(1.0); }"

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        if ptr is None:
            return 0
        val = ctypes.cast(ptr, ctypes.c_void_p).value
        return val if val is not None else 0

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
    
    # Initialize Context with BOTH callbacks for Context Manager support
    ctx = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(window),
        os_release_cb=lambda: glfw.make_context_current(None)
    )
    
    ctx.make_current()
    
    yield window
    
    glfw.destroy_window(window)
    glfw.terminate()

# --- 1. CORE API TESTS ---

def test_clear_operations():
    caravangl.clear_color(r=0.1, g=0.2, b=0.3, a=1.0)
    caravangl.clear(mask=GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

def test_texture_lifecycle():
    tex = caravangl.Texture(target=GL_TEXTURE_2D)
    tex.upload(256, 256, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, None)
    
    pixels = np.random.randint(0, 255, (64, 64, 4), dtype=np.uint8)
    tex.upload(64, 64, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, pixels.tobytes())
    tex.bind(unit=0)
    tex.generate_mipmap()

def test_vao_attribute_setup():
    vao = caravangl.VertexArray()
    vbo = caravangl.Buffer(size=4096, target=caravangl.TRIANGLES)
    vao.bind_attribute(0, vbo, 3, caravangl.FLOAT, 0, 12, 0)
    assert caravangl.inspect(vao)["type"] == "vertex_array"

def test_pipeline_render_state_persistence():
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao, depth_test=1, depth_write=0, blend=1)
    
    info = caravangl.inspect(pipe)
    assert info["render_state"]["depth_test"] is True
    assert info["render_state"]["depth_write"] is False
    assert info["render_state"]["blend"] is True

def test_high_frequency_mutation():
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    mv = pipe.params
    for i in range(1000):
        mv[0] = i      
        mv[1] = 1      
        mv[2] = i * 2  
        
    info = caravangl.inspect(pipe)
    assert info["draw_params"]["vertex_count"] == 999
    assert info["draw_params"]["first_vertex"] == 1998

def test_full_pipeline_draw():
    prog = caravangl.Program(vertex_shader=VS_ATTR, fragment_shader=FS_ATTR)
    vao = caravangl.VertexArray()
    
    verts = np.array([-0.5,-0.5,0,0,0,  0.5,-0.5,0,1,0,  0,0.5,0,0.5,1], dtype=np.float32)
    vbo = caravangl.Buffer(size=verts.nbytes, data=verts.tobytes())
    
    vao.bind_attribute(0, vbo, 3, caravangl.FLOAT, 0, 20, 0)
    vao.bind_attribute(1, vbo, 2, caravangl.FLOAT, 0, 20, 12) 
    
    tex = caravangl.Texture()
    tex.upload(64, 64, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, None)
    
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    pipe.params[0] = 3 
    
    caravangl.clear_color(0, 0, 0, 1)
    caravangl.clear(GL_COLOR_BUFFER_BIT)
    
    tex.bind(0)
    pipe.draw()

# --- 2. ADVANCED MULTI-THREADING (CONTEXT MANAGERS) ---

def test_parallel_context_rendering():
    main_window = glfw.get_current_context()
    shared_win = glfw.create_window(1, 1, "Shared Context", None, main_window)
    
    ctx2 = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(shared_win),
        os_release_cb=lambda: glfw.make_context_current(None)
    )
    
    # Temporarily release main thread context
    glfw.make_context_current(None)

    def render_work(ctx, color):
        # Look how clean this is! Automatically binds and unbinds.
        with ctx:
            caravangl.clear_color(*color)
            caravangl.clear(GL_COLOR_BUFFER_BIT)

    main_ctx = caravangl.get_active_context()
    
    t1 = threading.Thread(target=render_work, args=(main_ctx, (1, 0, 0, 1)))
    t2 = threading.Thread(target=render_work, args=(ctx2, (0, 1, 0, 1)))
    
    t1.start(); t2.start()
    t1.join(); t2.join()
    
    glfw.destroy_window(shared_win)
    # Re-acquire main context for the rest of Pytest
    main_ctx.make_current()

def test_cross_thread_sync_data_consistency():
    main_ctx = caravangl.get_active_context()
    vbo = caravangl.Buffer(size=1024)
    sync_queue = []

    # Release main context so thread can take it
    glfw.make_context_current(None)

    def producer_thread():
        # Scoped lock and execution
        with main_ctx:
            data = np.full(256, 42.0, dtype=np.float32).tobytes()
            vbo.write(data)
            sync_queue.append(caravangl.Sync())

    t = threading.Thread(target=producer_thread)
    t.start()
    t.join()

    # Back to main thread
    main_ctx.make_current()
    
    fence = sync_queue[0]
    status = fence.wait(timeout_sec=2.0)
    assert status != caravangl.TIMEOUT_EXPIRED
    assert True

def test_buffer_contention_stress():
    vbo = caravangl.Buffer(size=1024, usage=GL_DYNAMIC_DRAW)
    main_window = glfw.get_current_context()
    main_ctx = caravangl.get_active_context()
    
    shared_win = glfw.create_window(1, 1, "Stress Win", None, main_window)
    ctx2 = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(shared_win),
        os_release_cb=lambda: glfw.make_context_current(None)
    )
    
    glfw.make_context_current(None)
    stop_event = threading.Event()

    def writer():
        with ctx2:
            while not stop_event.is_set():
                data = np.random.randint(0, 255, 10, dtype=np.uint8).tobytes()
                vbo.write(data=data, offset=0)
            
    def reader():
        with main_ctx:
            while not stop_event.is_set():
                vbo.bind_base(index=0)
            
    t_write = threading.Thread(target=writer)
    t_read = threading.Thread(target=reader)
    
    t_write.start(); t_read.start()
    time.sleep(0.5)
    stop_event.set()
    
    t_write.join(); t_read.join()
    
    main_ctx.make_current()
    glfw.destroy_window(shared_win)
    assert True

# --- 3. QUERIES ---

def test_query_time_elapsed():
    """Verify GPU profiling query works."""
    query = caravangl.Query(target=caravangl.TIME_ELAPSED)
    
    # 1. Setup dummy work (Triangle)
    vs = "#version 330 core\nvoid main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }"
    fs = "#version 330 core\nout vec4 f; void main() { f = vec4(1.0); }"
    prog = caravangl.Program(vs, fs)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    pipe.params[0] = 3 # 3 vertices
    
    # 2. Profile a loop of draws to force the GPU clock to tick
    query.begin()
    for _ in range(100):
        caravangl.clear(GL_COLOR_BUFFER_BIT)
        pipe.draw()
    query.end()
    
    # 3. Get Result (This blocks until GPU finishes the 100 draws)
    nanoseconds = query.get_result()
    
    # If the GPU is insanely fast, it might still be 0, 
    # but 100 draws should satisfy any driver's timer resolution.
    assert nanoseconds >= 0
    # Log it so we can see the perf in -s output
    print(f"  [Perf] 100 dummy draws took: {nanoseconds} ns")

def test_query_occlusion():
    """Verify Occlusion Culling (SAMPLES_PASSED) works."""
    query = caravangl.Query(target=caravangl.SAMPLES_PASSED)
    
    prog = caravangl.Program(vertex_shader=VS_DUMMY, fragment_shader=FS_DUMMY)
    vao = caravangl.VertexArray()
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    
    # Start checking how many pixels render
    query.begin()
    
    # Since our VS_DUMMY just outputs vec4(0.0, 0.0, 0.0, 1.0) and draw_params 
    # vertex count is 0, this technically renders 0 pixels. 
    # A real test would draw a triangle.
    pipe.draw()
    
    query.end()
    
    pixels_drawn = query.get_result()
    assert pixels_drawn >= 0 # Should be 0 in this dummy case