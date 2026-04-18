# CaravanGL
**Yet another OpenGL wrapper.**

CaravanGL is, well, just an OpenGL wrapper written in C23. The experience is in the source code itself and very easy to maintain.

Furthermore it bypasses the historical bottlenecks of Python graphics by implementing a multi-context, thread-isolated architecture that achieves driver-bound execution speeds.

## Performance
*   **Throughput:** around 4,000,000 or more draw calls per second (Windows/WGL).
*   **Latency:** around 250-350ns per API call.
*   **Scaling:** Linear CPU scaling across multiple cores using Python 3.14t workers.
*   **Zero-Copy:** DMA data paths via Persistent Mapping (OpenGL 4.4+) and zero-copy uniform uploads.

## Key Features
*   **Isolated State Management:** Each thread operates its own `Context`, eliminating the global state-machine hazards of traditional OpenGL.
*   **Lazy State Evaluation:** Program, VAO, and FBO changes are tracked via "Dirty Bits" and resolved into a single batch of driver calls only when necessary.
*   **SIMD State Validation:** Pipeline render states (Depth, Blend, Stencil) are memoized and compared using SIMD-optimized integer blocks.
*   **GPGPU:** Full support for **Compute Shaders** and Shader Storage Buffer Objects (SSBOs). Windows and Linux only because Apple is silly.
*   **Memory Safety:** Cross-thread deferred garbage collection prevents resource leaks when objects are deleted on background threads.
*   **Pythonic Scoping:** Built-in context managers (`with ctx:`) handle the context dancing of GPU ownership automatically.
*   **MacOS Compatibility:** Suddenly I cared about MacOS this time. Handles gracefully when you use OpenGL 4.2+ features.

## Technologies
*   **Language:** C23 (using `_Generic`, bitfields, `[[gnu::hot]]` attributes and manual RAII).
*   **Python:** 3.14t (utilizing Python's `mimalloc` and GIL-disabled concurrency, and other Python builtins).
*   **Platforms:** Windows (WGL), macOS (CGL/Core Profile), Linux (GLX).

## Quick Start

You need a modern C compiler. And a graphics driver. And uv and CMake and Ninja(or Make). Let's just assume you have them.

For your sanity, don't use MSVC and MSBuild. It won't compile.

One, clone the repo.
```bash
git clone https://github.com/Evilpasture/CaravanGL.git
git submodule update --init --recursive
```

Two, build.

* With uv
```bash
uv pip install .
```

or

```bash
uv build
uv pip install dist/*.whl
```

* With CMake. Drag and drop your .pyd or .so, because you used CMake.
```bash
cmake -GNinja -B build
cmake --build build --config Release
```

Then you can run! If you have other problems, try using the x64 Native Tools for VS(DOS) or Developer PowerShell for VS, it happens often on Windows.

```python
import caravangl as cv
import glfw

# Setup window and context.
win = glfw.create_window(1280, 720, "CaravanGL", None, None)
ctx = cv.Context(
    loader=GLLoader(), 
    os_make_current_cb=lambda: glfw.make_context_current(win),
    os_release_cb=lambda: glfw.make_context_current(None)
)

# Define a pipeline once.
pipe = cv.Pipeline(
    program=cv.Program(vs_src, fs_src),
    vao=cv.VertexArray(),
    depth_test=1, 
    cull=1
)

# Hot loop.
while not glfw.window_should_close(win):
    with ctx:
        cv.clear(cv.COLOR_BUFFER_BIT | cv.DEPTH_BUFFER_BIT)
        pipe.draw()
    glfw.swap_buffers(win)
```

## High-End Architectures Supported
1.  **Asynchronous Streaming:** Load 4K textures on a background worker thread and hand them to the renderer with a `cv.Sync()` fence—zero frame stutter.
2.  **GPU Particle Physics:** Calculate 1,000,000+ positions using `cv.ComputePipeline` and draw them directly from an SSBO without ever touching CPU memory.
3.  **Sort-Based Rendering:** Leverage the Lazy State Resolver to draw thousands of objects; CaravanGL will automatically skip redundant shader/FBO switches.

## License
MIT License.