import ctypes
import gc
import sys
import threading
import time
from typing import TYPE_CHECKING

# pyright: reportMissingTypeStubs=false
# pyright: reportUnknownMemberType=false
import glfw
import numpy as np
import pytest

import caravangl


if TYPE_CHECKING:
    import glfw

    WindowHandle = glfw._GLFWwindow  # type: ignore
    from caravangl import Context

# --- Boilerplate Setup ---


class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        if not (ptr := glfw.get_proc_address(name)):
            return 0

        addr = ctypes.cast(ptr, ctypes.c_void_p).value
        return addr if addr is not None else 0


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
    win = glfw.create_window(1, 1, "Robustness Main", None, None)
    if not win:
        glfw.terminate()
        pytest.skip("Window creation failed")

    glfw.make_context_current(win)

    c = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(win),
        os_release_cb=lambda: glfw.make_context_current(None),
    )
    c.make_current()

    yield c

    # --- NUCLEAR FIXTURE CLEANUP ---
    glfw.make_context_current(None)

    # 1. Break the closure links inside the Context object
    c.os_make_current_cb = None
    c.os_release_cb = None

    # 2. Explicitly destroy the window while GLFW is still active
    if win:
        glfw.destroy_window(win)
        win = None

    # 3. Explicitly delete the context reference
    del c

    # 4. Force GC and process OS events to ensure all objects are dead
    gc.collect()
    gc.collect()  # Handle potential cycles
    glfw.poll_events()
    time.sleep(0.05)  # Give Cocoa/WGL a moment to finish window cleanup

    # 5. Only now is it safe to terminate the library
    glfw.terminate()


# --- SUITE 1: ERROR HANDLING & TYPE SAFETY ---


def test_buffer_invalid_args(ctx: Context):
    """Verify C-side FastParse rejects garbage input."""
    with ctx:
        with pytest.raises(TypeError):
            caravangl.Buffer(size="one gigabyte")  # type: ignore

        buf = caravangl.Buffer(size=1024)
        with pytest.raises(TypeError):
            buf.write(data=12345)  # type: ignore


# --- SUITE 2: ISOLATION ENFORCEMENT ---


def test_vao_isolation_enforcement(ctx: Context):
    """Verify that C code blocks using a VAO in the wrong context."""
    with ctx:
        vao_main = caravangl.VertexArray()
        vbo = caravangl.Buffer(size=1024)

    # 1. Create worker window on main thread (macOS requirement)
    main_win = glfw.get_current_context()
    apply_hints()
    worker_win = glfw.create_window(1, 1, "Worker", None, main_win)

    glfw.make_context_current(worker_win)
    ctx_worker = caravangl.Context(
        loader=GLLoader(),
        os_make_current_cb=lambda: glfw.make_context_current(worker_win),
        os_release_cb=lambda: glfw.make_context_current(None),
    )
    glfw.make_context_current(None)

    try:
        # 2. Try to use vao_main inside ctx_worker
        with ctx_worker:
            with pytest.raises(RuntimeError) as exc:
                # This should trigger the C check
                vao_main.bind_attribute(0, vbo, 3, caravangl.FLOAT)

            msg = str(exc.value).lower()
            assert "creator" in msg or "mismatch" in msg

    finally:
        ctx_worker.os_make_current_cb = None
        ctx_worker.os_release_cb = None
        glfw.make_context_current(None)
        del ctx_worker
        if worker_win:
            glfw.destroy_window(worker_win)
        gc.collect()
        gc.collect()
        glfw.poll_events()
        ctx.make_current()


# --- SUITE 3: MEMORY & RESOURCE STRESS ---


def test_massive_uniform_batch(ctx: Context):
    """Test the limits of the zero-copy UniformBatch."""
    batch = caravangl.UniformBatch(max_bindings=1000, max_bytes=1024 * 1024)
    for i in range(1000):
        batch.add(caravangl.UF_1F, location=i, count=1, size=4)

    view = batch.data
    view[1024 * 1024 - 1] = 255
    assert view[1024 * 1024 - 1] == 255


def test_texture_reallocation_churn(ctx: Context):
    """Verify that repeatedly uploading different sizes to one texture is stable."""
    with ctx:
        tex = caravangl.Texture()
        sizes = [(64, 64), (1024, 1024), (128, 128), (2, 2)]

        for w, h in sizes:
            tex.upload(
                w, h, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, None
            )
            info = caravangl.inspect(tex)
            assert info is not None
            assert info["width"] == w
            assert info["height"] == h


# --- SUITE 4: ASYNC SHARED RESOURCE INTEGRITY ---


def test_shared_buffer_multithreaded_write(ctx: Context):
    """Verify thread-safety of writing to a shared buffer from multiple threads."""
    with ctx:
        shared_vbo = caravangl.Buffer(size=4096)

    main_win = glfw.get_current_context()
    worker_resources: list[tuple[WindowHandle, caravangl.Context]] = []

    # 1. Pre-create on main thread
    for i in range(4):
        apply_hints()
        win = glfw.create_window(1, 1, f"W{i}", None, main_win)
        glfw.make_context_current(win)
        c = caravangl.Context(
            loader=GLLoader(),
            os_make_current_cb=lambda w=win: glfw.make_context_current(w),
            os_release_cb=lambda: glfw.make_context_current(None),
        )
        worker_resources.append((win, c))

    glfw.make_context_current(None)

    def worker_thread_entry(context: Context):
        # The 'with context' handles the make_current and release automatically
        with context:
            data = np.zeros(1024, dtype=np.uint8).tobytes()
            for _ in range(50):
                shared_vbo.write(data)

    threads = [
        threading.Thread(target=worker_thread_entry, args=(res[1],))
        for res in worker_resources
    ]

    for t in threads:
        t.start()
    for t in threads:
        t.join()

    # 2. Clean up resources
    for win, c in worker_resources:
        c.os_make_current_cb = None
        c.os_release_cb = None
        del c
        glfw.destroy_window(win)

    gc.collect()
    gc.collect()
    glfw.poll_events()

    ctx.make_current()
    assert True


# --- SUITE 5: STATE SHADOWING VALIDATION ---
def test_viewport_shadowing(ctx: Context):
    """Verify isolated shadow states."""
    main_win = glfw.get_current_context()

    def exec_isolated():
        # This nested scope ensures that locals like 'cw' and 'worker_win'
        # are eligible for garbage collection as soon as this function exits.
        apply_hints()
        worker_win = glfw.create_window(1, 1, "W", None, main_win)

        glfw.make_context_current(worker_win)
        cw = caravangl.Context(
            loader=GLLoader(),
            os_make_current_cb=lambda: glfw.make_context_current(worker_win),
            os_release_cb=lambda: glfw.make_context_current(None),
        )
        glfw.make_context_current(None)

        try:
            # 1. Set main viewport
            with ctx:
                caravangl.viewport(0, 0, 800, 600)

            # 2. Switch context and set different viewport
            with cw:
                caravangl.viewport(0, 0, 100, 100)
                vp_worker = caravangl.context()["viewport"]
                assert vp_worker == (0, 0, 100, 100)

            # 3. Check main context again (shadow state should be preserved)
            with ctx:
                vp_main = caravangl.context()["viewport"]
                assert vp_main == (0, 0, 800, 600)

        finally:
            # --- INTERNAL CLEANUP ---
            # Release callbacks to break closure links to 'worker_win' and 'glfw'
            cw.os_make_current_cb = None
            cw.os_release_cb = None

            # Ensure the baton is put down
            glfw.make_context_current(None)

            if worker_win:
                glfw.destroy_window(worker_win)

            # Function returns, 'cw' local variable is now out of scope

    # Run the logic
    exec_isolated()

    # --- FINAL EXTERNAL CLEANUP ---
    # Now that 'cw' is gone, force multiple GC passes to trigger the C deallocator
    for _ in range(3):
        gc.collect()

    # Flush OS events and let Cocoa breathe before the fixture terminates
    glfw.poll_events()
    time.sleep(0.02)

    # Restore main context baton for subsequent tests
    ctx.make_current()
