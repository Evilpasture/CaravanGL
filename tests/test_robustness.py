import pytest
import threading
import numpy as np
import glfw
import caravangl
import ctypes
import sys

# --- Boilerplate Setup ---
class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

def apply_hints():
    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    if sys.platform == "darwin":
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

@pytest.fixture(scope="module")
def ctx():
    if not glfw.init(): 
        pytest.skip("GLFW init failed")
    
    apply_hints()
    win = glfw.create_window(1, 1, "Robustness Hidden", None, None)
    
    # MANDATORY FOR WINDOWS: Context must be current BEFORE Context() init
    glfw.make_context_current(win)
    
    c = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(win),
        os_release_cb=lambda: glfw.make_context_current(None)
    )
    c.make_current()
    yield c
    
    glfw.destroy_window(win)
    glfw.terminate()

# --- SUITE 1: ERROR HANDLING & TYPE SAFETY ---

def test_buffer_invalid_args(ctx):
    with ctx:
        with pytest.raises(TypeError):
            caravangl.Buffer(size="one gigabyte") # type: ignore
        buf = caravangl.Buffer(size=1024)
        with pytest.raises(TypeError):
            buf.write(data=12345) # type: ignore

def test_vao_isolation_enforcement(ctx):
    with ctx:
        vao_main = caravangl.VertexArray()
        vbo = caravangl.Buffer(size=1024)

    main_win = glfw.get_current_context()
    apply_hints()
    worker_win = glfw.create_window(1, 1, "Worker", None, main_win)
    
    # 1. Make OS context current FIRST
    glfw.make_context_current(worker_win)
    
    # 2. Now init the CaravanGL object (so loader finds functions)
    ctx_worker = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(worker_win),
        os_release_cb=lambda: glfw.make_context_current(None)
    )

    with ctx_worker:
        with pytest.raises(RuntimeError) as exc:
            vao_main.bind_attribute(0, vbo, 3, caravangl.FLOAT)
        msg = str(exc.value).lower()
        assert "creator" in msg or "mismatch" in msg
    
    glfw.destroy_window(worker_win)
    # Restore main
    ctx.make_current()

# --- SUITE 2: MEMORY & RESOURCE STRESS ---

def test_massive_uniform_batch(ctx):
    batch = caravangl.UniformBatch(max_bindings=1000, max_bytes=1024*1024)
    for i in range(1000):
        batch.add(caravangl.UF_1F, location=i, count=1, size=4)
    view = batch.data
    view[1024*1024 - 1] = 255
    assert view[1024*1024 - 1] == 255

def test_texture_reallocation_churn(ctx):
    with ctx:
        tex = caravangl.Texture()
        for w, h in [(64,64), (512, 512), (2, 2)]:
            tex.upload(w, h, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, None)
            info = caravangl.inspect(tex)
            assert info["width"] == w

# --- SUITE 3: ASYNC SHARED RESOURCE INTEGRITY ---

def test_shared_buffer_multithreaded_write(ctx):
    with ctx:
        shared_vbo = caravangl.Buffer(size=4096)
    
    main_win = glfw.get_current_context()
    
    def worker():
        apply_hints()
        win = glfw.create_window(1, 1, "W", None, main_win)
        
        # Windows Requirement: Bind OS context before CaravanGL loader runs
        glfw.make_context_current(win)
        
        c = caravangl.Context(
            loader=GLLoader(), 
            os_make_current_cb=lambda: glfw.make_context_current(win),
            os_release_cb=lambda: glfw.make_context_current(None)
        )
        with c:
            data = np.zeros(1024, dtype=np.uint8).tobytes()
            for _ in range(50):
                shared_vbo.write(data)
        glfw.destroy_window(win)

    threads = [threading.Thread(target=worker) for _ in range(4)]
    for t in threads: t.start()
    for t in threads: t.join()
    
    ctx.make_current()
    assert True

# --- SUITE 4: STATE SHADOWING VALIDATION ---

def test_viewport_shadowing(ctx):
    main_win = glfw.get_current_context()
    apply_hints()
    worker_win = glfw.create_window(1, 1, "W", None, main_win)
    
    glfw.make_context_current(worker_win)
    ctx_worker = caravangl.Context(
        loader=GLLoader(), 
        os_make_current_cb=lambda: glfw.make_context_current(worker_win),
        os_release_cb=lambda: glfw.make_context_current(None)
    )

    with ctx:
        caravangl.viewport(0, 0, 800, 600)
    
    with ctx_worker:
        caravangl.viewport(0, 0, 100, 100)
        assert caravangl.context()["viewport"] == (0, 0, 100, 100)

    with ctx:
        assert caravangl.context()["viewport"] == (0, 0, 800, 600)
    
    glfw.destroy_window(worker_win)