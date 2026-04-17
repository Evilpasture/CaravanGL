import glfw
import numpy as np
import caravangl
import ctypes
import time

# --- Configuration ---
TRIANGLE_COUNT = 15000  # Increased count to show off performance
WINDOW_WIDTH = 1280
WINDOW_HEIGHT = 720

VS_SOURCE = """
#version 330 core
layout (location = 0) in vec2 aPos;    
layout (location = 1) in vec2 aOffset; 
layout (location = 2) in vec3 aColor;  

out vec3 vColor;

void main() {
    // Scale and rotate logic can be added here, but keeping it simple
    gl_Position = vec4(aPos * 0.01 + aOffset, 0.0, 1.0);
    vColor = aColor;
}
"""

FS_SOURCE = """
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
"""

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

def run_swarm():
    if not glfw.init():
        return

    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)
    glfw.window_hint(glfw.DOUBLEBUFFER, glfw.TRUE)

    window = glfw.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "CaravanGL Swarm", None, None)
    if not window:
        glfw.terminate()
        return

    glfw.make_context_current(window)
    glfw.swap_interval(0) # Disable VSync to see true FPS
    
    ctx = caravangl.Context(loader=GLLoader())
    ctx.os_make_current_cb = lambda: glfw.make_context_current(window)
    ctx.make_current()

    # 1. Base Triangle Shape
    shape = np.array([0.0, 0.6, -0.4, -0.4, 0.4, -0.4], dtype=np.float32)
    vbo_shape = caravangl.Buffer(size=shape.nbytes, data=shape.tobytes())

    # 2. Instance Data
    # Structure: [x, y (pos), r, g, b (color)]
    instance_data = np.zeros(TRIANGLE_COUNT, dtype=[('pos', 'f4', 2), ('color', 'f4', 3)])
    instance_data['pos'] = np.random.uniform(-1, 1, (TRIANGLE_COUNT, 2))
    
    # Give them a gradient color based on their ID
    colors = np.zeros((TRIANGLE_COUNT, 3), dtype=np.float32)
    colors[:, 0] = np.linspace(0.2, 1.0, TRIANGLE_COUNT) # Red sweep
    colors[:, 1] = np.linspace(1.0, 0.2, TRIANGLE_COUNT) # Green sweep
    colors[:, 2] = 0.6 # Constant Blue
    instance_data['color'] = colors

    # 3. Individual "Personalities" (for Dispersion)
    # Each triangle wants to be at a specific offset from the cursor
    personal_offsets = np.random.normal(0, 0.15, (TRIANGLE_COUNT, 2)).astype(np.float32)
    
    # Also give them individual speeds
    personal_speeds = np.random.uniform(0.0002, 0.0015, (TRIANGLE_COUNT, 1)).astype(np.float32)

    vbo_instances = caravangl.Buffer(size=instance_data.nbytes, usage=caravangl.DYNAMIC_DRAW)

    # 4. VAO Setup
    vao = caravangl.VertexArray()
    vao.bind_attribute(0, vbo_shape, 2, caravangl.FLOAT)
    vao.bind_attribute(1, vbo_instances, 2, caravangl.FLOAT, stride=20, offset=0, divisor=1)
    vao.bind_attribute(2, vbo_instances, 3, caravangl.FLOAT, stride=20, offset=8, divisor=1)

    # 5. Pipeline
    prog = caravangl.Program(vertex_shader=VS_SOURCE, fragment_shader=FS_SOURCE)
    pipe = caravangl.Pipeline(program=prog, vao=vao)
    pipe.params[0] = 3
    pipe.params[1] = TRIANGLE_COUNT

    velocities = np.zeros((TRIANGLE_COUNT, 2), dtype=np.float32)

    # FPS Tracking
    frame_count = 0
    last_fps_update = time.time()
    last_time = time.time()

    while not glfw.window_should_close(window):
        # Calculate Delta Time
        current_time = time.time()
        dt = current_time - last_time
        last_time = current_time
        frame_count += 1

        # Update FPS in title every second
        if current_time - last_fps_update >= 1.0:
            fps = frame_count / (current_time - last_fps_update)
            glfw.set_window_title(window, f"CaravanGL Swarm | {TRIANGLE_COUNT} Triangles | FPS: {fps:.1f}")
            frame_count = 0
            last_fps_update = current_time

        # Get Mouse Target
        mx, my = glfw.get_cursor_pos(window)
        target_x = (mx / WINDOW_WIDTH) * 2 - 1
        target_y = (my / WINDOW_HEIGHT) * -2 + 1
        global_target = np.array([target_x, target_y], dtype=np.float32)

        # Vectorized Physics with Dispersion
        # Each triangle targets (Cursor + Personal Offset)
        targets = global_target + personal_offsets
        
        diff = targets - instance_data['pos']
        dist = np.linalg.norm(diff, axis=1, keepdims=True)
        
        # Attraction force (proportional to distance, but capped)
        force = (diff / (dist + 0.01)) * personal_speeds
        velocities += force
        
        # Damping (Friction)
        velocities *= 0.94
        
        # Update Positions
        instance_data['pos'] += velocities

        # Rendering
        caravangl.clear_color(0.02, 0.02, 0.05, 1.0)
        caravangl.clear(caravangl.COLOR_BUFFER_BIT)

        # Fast upload and draw
        vbo_instances.write(data=instance_data.tobytes())
        pipe.draw()

        glfw.swap_buffers(window)
        glfw.poll_events()

    glfw.terminate()

if __name__ == "__main__":
    run_swarm()