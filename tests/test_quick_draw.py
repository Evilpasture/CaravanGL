import glfw
import caravangl
import threading
import time
import ctypes
import sys

# --- CONFIG ---
DRAWS_PER_THREAD = 250_000
THREAD_COUNT = 4

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        if ptr is None: return 0
        return ctypes.cast(ptr, ctypes.c_void_p).value or 0

def worker_task(ctx, shared_vbo, shared_prog, iters):
    try:
        # 1. Take ownership of the context we were handed
        ctx.make_current()

        # 2. VAOs are NOT shared, so create it here on the worker thread
        vao = caravangl.VertexArray()
        vao.bind_attribute(0, shared_vbo, 3, caravangl.FLOAT)
        
        # 3. Setup local pipeline
        pipe = caravangl.Pipeline(program=shared_prog, vao=vao)
        pipe.params[0] = 3
        
        # 4. Performance Loop
        for _ in range(iters):
            pipe.draw()
            
    except Exception as e:
        print(f"\nWorker Error: {e}")

def run_bench():
    if not glfw.init():
        return

    # Windows Global Hints
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    
    # 1. Create Main Window
    main_window = glfw.create_window(640, 480, "Main", None, None)
    glfw.make_context_current(main_window)
    main_ctx = caravangl.Context(loader=GLLoader())
    main_ctx.make_current()
    
    # Init Shared Resources
    vs = "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }"
    fs = "#version 330 core\nout vec4 f; void main() { f = vec4(1.0); }"
    shared_prog = caravangl.Program(vs, fs)
    shared_vbo = caravangl.Buffer(size=1024)

    # 2. Pre-create all Worker Windows/Contexts on the MAIN thread
    worker_contexts = []
    for i in range(THREAD_COUNT):
        # Share with main_window
        win = glfw.create_window(1, 1, f"Worker {i}", None, main_window)
        if not win:
            print(f"Failed to create window {i}")
            continue
            
        # Temporarily make current to init the Context object
        glfw.make_context_current(win)
        c = caravangl.Context(loader=GLLoader())
        # Set the callback so the worker thread can call it later
        c.os_make_current_cb = lambda w=win: glfw.make_context_current(w)
        worker_contexts.append(c)

    # 3. Release context from Main Thread so workers can take them
    glfw.make_context_current(None)

    print(f"Starting {len(worker_contexts)} threads with pre-initialized contexts...")
    threads = [threading.Thread(target=worker_task, 
                                args=(worker_contexts[i], shared_vbo, shared_prog, DRAWS_PER_THREAD)) 
               for i in range(len(worker_contexts))]

    start = time.perf_counter()
    for t in threads: t.start()
    for t in threads: t.join()
    end = time.perf_counter()

    # Restore main context for cleanup
    glfw.make_context_current(main_window)
    
    total_draws = len(worker_contexts) * DRAWS_PER_THREAD
    duration = end - start
    print(f"\nFinished {total_draws:,} draws in {duration:.4f}s")
    print(f"Combined Throughput: {total_draws / duration:,.0f} draws/sec")

    glfw.terminate()

if __name__ == "__main__":
    run_bench()