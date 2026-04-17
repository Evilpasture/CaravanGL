import glfw
import struct
import time
import numpy as np
import caravangl
import ctypes

# 1. Setup
glfw.init()
glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE) 
window = glfw.create_window(800, 600, "Instancing Diagnosis", None, None)
glfw.make_context_current(window)

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        if ptr is None: return 0
        val = ctypes.cast(ptr, ctypes.c_void_p).value
        return val if val is not None else 0

caravangl.init(loader=GLLoader())

# 2. Simple Shaders (No complexity)
VS = """#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aOffset; // Instanced
void main() {
    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);
}""".strip()

FS = """#version 330 core
out vec4 fCol;
void main() { fCol = vec4(1.0, 0.5, 0.0, 1.0); }""".strip()

prog = caravangl.Program(vertex_shader=VS, fragment_shader=FS)

# 3. Simple Geometry (1 Triangle)
tri = np.array([-0.05, -0.05, 0.05, -0.05, 0.0, 0.05], dtype=np.float32)
vbo_tri = caravangl.Buffer(size=tri.nbytes, data=tri.tobytes())

# 4. Instance Data (Max 10,000)
# Just a grid of offsets
MAX_INSTANCES = 10_000
offsets = np.random.uniform(-0.9, 0.9, (MAX_INSTANCES, 2)).astype(np.float32)
vbo_inst = caravangl.Buffer(size=offsets.nbytes, data=offsets.tobytes())

# 5. VAO
vao = caravangl.VertexArray()
# Shape (Divisor 0)
vao.bind_attribute(location=0, buffer=vbo_tri, size=2, type=caravangl.FLOAT, stride=8, offset=0, divisor=0)
# Offsets (Divisor 1)
vao.bind_attribute(location=1, buffer=vbo_inst, size=2, type=caravangl.FLOAT, stride=8, offset=0, divisor=1)

# 6. Pipeline
pipe = caravangl.Pipeline(program=prog, vao=vao)

# 7. Diagnostic Loop
current_instances = 1
last_report = time.time()

print("Starting Diagnosis...")

while not glfw.window_should_close(window) and current_instances <= MAX_INSTANCES:
    glfw.poll_events()
    
    # Increase count slowly
    if time.time() - last_report > 0.5:
        current_instances = min(MAX_INSTANCES, int(current_instances * 2) + 1)
        print(f"Drawing {current_instances} instances...")
        last_report = time.time()

    # Update draw params
    pipe.params[0] = 3                 # vertex_count
    pipe.params[1] = current_instances # instance_count
    
    caravangl.clear_color(0.1, 0.1, 0.1, 1.0)
    caravangl.clear(caravangl.COLOR_BUFFER_BIT)
    
    pipe.draw()
    
    glfw.swap_buffers(window)

glfw.terminate()