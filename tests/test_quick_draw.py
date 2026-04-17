import glfw
import caravangl
import time
import ctypes
import sys
import threading

# --- CONFIGURATION ---
TOTAL_DRAW_CALLS = 1_000_000
THREAD_COUNT = 4
DRAWS_PER_THREAD = TOTAL_DRAW_CALLS // THREAD_COUNT
WINDOW_WIDTH = 640
WINDOW_HEIGHT = 480

# --- 1. SETUP GLFW & CORE PROFILE ---
def setup_gl():
    if not glfw.init():
        sys.exit(1)

    if sys.platform == "darwin":
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    window = glfw.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Parallel Bench", None, None)
    if not window:
        glfw.terminate()
        sys.exit(1)
    
    glfw.make_context_current(window)
    return window

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

# --- 2. PREPARE RESOURCES ---
window = setup_gl()
ctx = caravangl.Context(loader=GLLoader())
ctx.make_current()

# Minimal Shaders
vs = """
#version 330 core
void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }
"""
fs = """
#version 330 core
out vec4 f; void main() { f = vec4(1.0, 0.0, 0.0, 1.0); }
"""

prog = caravangl.Program(vs, fs)
vao = caravangl.VertexArray()
pipe = caravangl.Pipeline(program=prog, vao=vao)
pipe.params[0] = 3 # 1 Triangle

# --- 3. THE WORKER FUNCTION ---
def benchmark_worker(worker_id, iterations, pipeline, context):
    """
    Each thread must call make_current() to set its TLS pointer 
    to the shared context.
    """
    context.make_current()
    
    # Simple loop to hammer the draw call
    for _ in range(iterations):
        pipeline.draw()

# --- 4. EXECUTION ---
print(f"Platform: {sys.platform}")
print(f"Python Version: {sys.version.split()[0]} (Free-Threaded)")
print(f"Total Draws: {TOTAL_DRAW_CALLS:,}")
print(f"Threads: {THREAD_COUNT} ({DRAWS_PER_THREAD:,} calls per thread)")
print("-" * 40)

# Warmup (Single Threaded)
print("Warming up driver cache...")
for _ in range(5000):
    pipe.draw()

# Start Threads
threads = []
for i in range(THREAD_COUNT):
    t = threading.Thread(
        target=benchmark_worker, 
        args=(i, DRAWS_PER_THREAD, pipe, ctx)
    )
    threads.append(t)

start_time = time.perf_counter()

for t in threads:
    t.start()

for t in threads:
    t.join()

end_time = time.perf_counter()

# --- 5. RESULTS ---
duration = end_time - start_time
draws_per_sec = TOTAL_DRAW_CALLS / duration

print(f"Parallel Execution Finished.")
print(f"Total Time: {duration:.4f} seconds")
print(f"Throughput: {draws_per_sec:,.0f} draws/second")
print(f"Latency: {(duration / TOTAL_DRAW_CALLS) * 1e9:.2f} ns per call")

glfw.terminate()