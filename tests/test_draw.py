import glfw
import numpy as np
import caravangl
import ctypes
import math
import time

# --- CONFIGURATION ---
WINDOW_WIDTH = 1280
WINDOW_HEIGHT = 720

# --- 1. MATH HELPERS ---

def get_perspective(fovy, aspect, n, f):
    s = 1.0 / math.tan(fovy / 2.0)
    # Standard Row-Major Perspective
    return np.array([
        [s/aspect, 0, 0, 0],
        [0, s, 0, 0],
        [0, 0, (f+n)/(n-f), (2*f*n)/(n-f)],
        [0, 0, -1, 0]
    ], dtype=np.float32)

def get_rotation(t):
    c, s = np.cos(t), np.sin(t)
    # Ensure these are float32 to prevent matrix promotion to float64
    c, s = np.float32(c), np.float32(s)
    rx = np.array([[1,0,0,0],[0,c,-s,0],[0,s,c,0],[0,0,0,1]], dtype=np.float32)
    ry = np.array([[c,0,s,0],[0,1,0,0],[-s,0,c,0],[0,0,0,1]], dtype=np.float32)
    return rx @ ry

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

def run_cube():
    if not glfw.init(): return

    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)

    window = glfw.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "CaravanGL Solid Cube", None, None)
    glfw.make_context_current(window)
    
    ctx = caravangl.Context(loader=GLLoader())
    ctx.os_make_current_cb = lambda: glfw.make_context_current(window)
    ctx.make_current()

    # 2. SHADERS
    VS_3D = """#version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTex;
    uniform mat4 u_mvp;
    out vec2 vTex;
    void main() {
        gl_Position = u_mvp * vec4(aPos, 1.0);
        vTex = aTex;
    }""".strip()

    FS_3D = """#version 330 core
    uniform sampler2D u_tex;
    in vec2 vTex;
    out vec4 fCol;
    void main() {
        fCol = texture(u_tex, vTex);
    }""".strip()

    prog = caravangl.Program(vertex_shader=VS_3D, fragment_shader=FS_3D)

    # 3. GEOMETRY (Explicit CCW Winding)
    # Format: X, Y, Z, U, V
    cube_data = np.array([
        # Front face (Z=0.5)
        -0.5,-0.5, 0.5, 0,0,  0.5,-0.5, 0.5, 1,0,  0.5, 0.5, 0.5, 1,1, -0.5, 0.5, 0.5, 0,1,
        # Back face (Z=-0.5)
        -0.5,-0.5,-0.5, 0,0, -0.5, 0.5,-0.5, 0,1,  0.5, 0.5,-0.5, 1,1,  0.5,-0.5,-0.5, 1,0,
        # Left face (X=-0.5)
        -0.5,-0.5,-0.5, 0,0, -0.5,-0.5, 0.5, 1,0, -0.5, 0.5, 0.5, 1,1, -0.5, 0.5,-0.5, 0,1,
        # Right face (X=0.5)
         0.5,-0.5,-0.5, 0,0,  0.5, 0.5,-0.5, 0,1,  0.5, 0.5, 0.5, 1,1,  0.5,-0.5, 0.5, 1,0,
        # Top face (Y=0.5)
        -0.5, 0.5,-0.5, 0,1, -0.5, 0.5, 0.5, 0,0,  0.5, 0.5, 0.5, 1,0,  0.5, 0.5,-0.5, 1,1,
        # Bottom face (Y=-0.5)
        -0.5,-0.5,-0.5, 0,1,  0.5,-0.5,-0.5, 1,1,  0.5,-0.5, 0.5, 1,0, -0.5,-0.5, 0.5, 0,0,
    ], dtype=np.float32)

    indices = []
    for i in range(0, 24, 4):
        indices.extend([i, i+1, i+2, i+2, i+3, i])
    cube_indices = np.array(indices, dtype=np.uint32)

    vbo = caravangl.Buffer(size=cube_data.nbytes, data=cube_data.tobytes())
    ibo = caravangl.Buffer(size=cube_indices.nbytes, data=cube_indices.tobytes(), target=caravangl.ELEMENT_ARRAY_BUFFER)
    
    vao = caravangl.VertexArray()
    vao.bind_index_buffer(ibo)
    # Stride is 5 floats * 4 bytes = 20
    vao.bind_attribute(0, vbo, 3, caravangl.FLOAT, 20, 0)
    vao.bind_attribute(1, vbo, 2, caravangl.FLOAT, 20, 12)

    # 4. TEXTURE (2x2 Checkerboard)
    t_raw = np.array([
        [255, 0, 0, 255,   0, 255, 0, 255],
        [0, 0, 255, 255,   255, 255, 0, 255]
    ], dtype=np.uint8)
    tex = caravangl.Texture()
    tex.upload(2, 2, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, t_raw.tobytes())
    samp = caravangl.Sampler(min_filter=caravangl.NEAREST, mag_filter=caravangl.NEAREST)

    # 5. PIPELINE
    pipe = caravangl.Pipeline(
        program=prog, vao=vao, 
        depth_test=1, depth_write=1, cull=1,
        index_type=caravangl.UNSIGNED_INT
    )
    pipe.params[0] = 36 # 36 indices

    # 6. UNIFORMS
    batch = caravangl.UniformBatch(max_bindings=1, max_bytes=64)
    loc_mvp = prog.get_uniform_location("u_mvp")
    # Use UF_MAT4_RM to match our NumPy Row-Major construction
    off_mvp = batch.add(caravangl.UF_MAT4_RM, loc_mvp, 1, 64)

    proj = get_perspective(math.radians(45), WINDOW_WIDTH/WINDOW_HEIGHT, 0.1, 100.0)

    while not glfw.window_should_close(window):
        glfw.poll_events()
        
        # MATH (Row-Major: P @ V @ M)
        model = get_rotation(time.time())
        view = np.eye(4, dtype=np.float32)
        view[2, 3] = -3.0 # Translation in 4th Column (Row-Major style)
        
        mvp = (proj @ view @ model).astype(np.float32)
        
        # UPLOAD: We use .tobytes() for absolute safety of the memory layout
        batch.data[off_mvp : off_mvp + 64] = mvp.tobytes()

        # RENDER
        caravangl.viewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)
        caravangl.clear_color(0.1, 0.1, 0.1, 1.0)
        caravangl.clear(caravangl.COLOR_BUFFER_BIT | caravangl.DEPTH_BUFFER_BIT)
        
        tex.bind(0, samp)
        pipe.upload_uniforms(batch)
        pipe.draw()
        
        glfw.swap_buffers(window)

    glfw.terminate()

if __name__ == "__main__":
    run_cube()