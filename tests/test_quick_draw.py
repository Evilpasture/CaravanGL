import os
print(f"PID: {os.getpid()}")
input("Press Enter to continue and crash...")

import faulthandler

faulthandler.enable()
import glfw
import caravangl
import threading
import time
import ctypes
import sys

# --- CONFIG ---
DRAWS_PER_THREAD = 250_000
THREAD_COUNT = 4


# --- 1. SETUP ---
def setup_main():
    glfw.init()
    if sys.platform == "darwin":
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    window = glfw.create_window(640, 480, "Main", None, None)
    glfw.make_context_current(window)
    return window


class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0


# --- 2. WORKER LOGIC ---
def worker_task(main_window, shared_vbo, shared_prog, iters):
    # A. Create a LOCAL window shared with the main one
    # Passing 'main_window' as the last arg enables resource sharing
    local_win = glfw.create_window(1, 1, "Worker", None, main_window)

    # B. Create a LOCAL CaravanGL context for this thread
    ctx = caravangl.Context(loader=GLLoader())
    ctx.os_make_current_cb = lambda: glfw.make_context_current(local_win)
    ctx.make_current()

    # C. VAOs are NOT shared. We must create one for this thread
    # but we point it to the SHARED VBO.
    vao = caravangl.VertexArray()
    vao.bind_attribute(0, shared_vbo, 3, caravangl.FLOAT)

    # D. Create a Pipeline using the shared Program and local VAO
    pipe = caravangl.Pipeline(program=shared_prog, vao=vao)
    pipe.params[0] = 3

    # E. Parallel Execution!
    # Because this thread has its own context, there is ZERO lock contention.
    for _ in range(iters):
        pipe.draw()


# --- 3. MAIN EXECUTION ---
main_window = setup_main()
main_ctx = caravangl.Context(loader=GLLoader())
main_ctx.os_make_current_cb = lambda: glfw.make_context_current(main_window)
main_ctx.make_current()

# Create SHARED resources
vs = "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }"
fs = "#version 330 core\nout vec4 f; void main() { f = vec4(1.0); }"
shared_prog = caravangl.Program(vs, fs)
shared_vbo = caravangl.Buffer(size=1024)

print(f"Starting {THREAD_COUNT} threads with shared resources...")
threads = [
    threading.Thread(
        target=worker_task,
        args=(main_window, shared_vbo, shared_prog, DRAWS_PER_THREAD),
    )
    for _ in range(THREAD_COUNT)
]

start = time.perf_counter()
for t in threads:
    t.start()
for t in threads:
    t.join()
end = time.perf_counter()

total_draws = THREAD_COUNT * DRAWS_PER_THREAD
duration = end - start
print(f"Finished {total_draws:,} draws in {duration:.4f}s")
print(f"Combined Throughput: {total_draws / duration:,.0f} draws/sec")

glfw.terminate()
