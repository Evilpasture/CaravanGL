import glfw
import struct
import math
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
window = glfw.create_window(1280, 720, "CaravanGL 3D Solid Cube", None, None)
glfw.make_context_current(window)

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        if ptr is None: return 0
        return ctypes.cast(ptr, ctypes.c_void_p).value or 0

caravangl.init(loader=GLLoader())

# 2. ROW-MAJOR MATH HELPERS
def get_perspective(fovy, aspect, n, f):
    s = 1.0 / math.tan(fovy / 2.0)
    return np.array([
        [s/aspect, 0, 0, 0],
        [0, s, 0, 0],
        [0, 0, (f+n)/(n-f), (2*f*n)/(n-f)],
        [0, 0, -1, 0]
    ], dtype=np.float32)

def get_rotation(t):
    c, s = math.cos(t), math.sin(t)
    # Pitch/Yaw combination
    rx = np.array([[1,0,0,0],[0,c,-s,0],[0,s,c,0],[0,0,0,1]], dtype=np.float32)
    ry = np.array([[c,0,s,0],[0,1,0,0],[-s,0,c,0],[0,0,0,1]], dtype=np.float32)
    return rx @ ry

# 3. Standard Shaders
VS_3D = """#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
uniform mat4 u_mvp;
out vec2 vTex;
void main() {
    gl_Position = u_mvp * vec4(aPos, 1.0);
    vTex = aTex;
}"""

FS_3D = """#version 330 core
uniform sampler2D u_tex;
in vec2 vTex;
out vec4 fCol;
void main() {
    fCol = texture(u_tex, vTex);
}"""

prog = caravangl.Program(vertex_shader=VS_3D, fragment_shader=FS_3D)

# 4. Geometry (Standard CCW Winding)
cube_data = np.array([
    # X, Y, Z, U, V
    -0.5,-0.5, 0.5, 0,0,  0.5,-0.5, 0.5, 1,0,  0.5, 0.5, 0.5, 1,1, -0.5, 0.5, 0.5, 0,1, # Front
    -0.5,-0.5,-0.5, 0,0, -0.5, 0.5,-0.5, 0,1,  0.5, 0.5,-0.5, 1,1,  0.5,-0.5,-0.5, 1,0, # Back
    -0.5,-0.5,-0.5, 0,0, -0.5,-0.5, 0.5, 1,0, -0.5, 0.5, 0.5, 1,1, -0.5, 0.5,-0.5, 0,1, # Left
     0.5,-0.5,-0.5, 0,0,  0.5, 0.5,-0.5, 0,1,  0.5, 0.5, 0.5, 1,1,  0.5,-0.5, 0.5, 1,0, # Right
    -0.5, 0.5,-0.5, 0,1, -0.5, 0.5, 0.5, 0,0,  0.5, 0.5, 0.5, 1,0,  0.5, 0.5,-0.5, 1,1, # Top
    -0.5,-0.5,-0.5, 0,1,  0.5,-0.5,-0.5, 1,1,  0.5,-0.5, 0.5, 1,0, -0.5,-0.5, 0.5, 0,0, # Bottom
], dtype=np.float32)

idx = []
for i in range(0, 24, 4): idx.extend([i, i+1, i+2, i+2, i+3, i])
cube_indices = np.array(idx, dtype=np.uint32)

vbo = caravangl.Buffer(size=cube_data.nbytes, data=cube_data.tobytes())
ibo = caravangl.Buffer(size=cube_indices.nbytes, data=cube_indices.tobytes(), target=caravangl.ELEMENT_ARRAY_BUFFER)
vao = caravangl.VertexArray()
vao.bind_index_buffer(ibo)
vao.bind_attribute(location=0, buffer=vbo, size=3, type=caravangl.FLOAT, stride=20, offset=0)
vao.bind_attribute(location=1, buffer=vbo, size=2, type=caravangl.FLOAT, stride=20, offset=12)

# 5. Texture & Sampler
t_data = np.zeros((2, 2, 4), dtype=np.uint8)
t_data[0,0]=[255,0,0,255]; t_data[0,1]=[0,255,0,255]
t_data[1,0]=[0,0,255,255]; t_data[1,1]=[255,255,0,255]
tex = caravangl.Texture(); tex.upload(2, 2, caravangl.RGBA8, caravangl.RGBA, caravangl.UNSIGNED_BYTE, t_data.tobytes())
samp = caravangl.Sampler(min_filter=caravangl.NEAREST, mag_filter=caravangl.NEAREST)

# 6. Pipeline (SOLID 3D)
pipe = caravangl.Pipeline(
    program=prog, vao=vao, 
    depth_test=1, depth_write=1, depth_func=caravangl.LESS,
    cull=1, cull_mode=caravangl.BACK,
    index_type=caravangl.UNSIGNED_INT
)
pipe.params[0] = 36

# 7. Uniforms (ROW MAJOR MODE)
batch = caravangl.UniformBatch(max_bindings=1, max_bytes=64)
loc_mvp = prog.get_uniform_location("u_mvp")
off_mvp = batch.add(caravangl.UF_MAT4_RM, loc_mvp, 1, 64) # <-- Uses transpose internally

proj = get_perspective(math.radians(45), 1280/720, 0.1, 100.0)

# 8. Loop
while not glfw.window_should_close(window):
    glfw.poll_events()
    
    # CALCULATE MVP (Row Major order: P @ V @ M)
    model = get_rotation(time.time())
    
    # View Translation (Translate -3 on Z)
    view = np.eye(4, dtype=np.float32)
    view[2, 3] = -3.0 # Row-major translation
    
    # Total MVP
    mvp = proj @ view @ model
    
    # Zero-Copy Cast Upload
    batch.data[off_mvp : off_mvp + 64] = memoryview(mvp.astype(np.float32)).cast("B")

    w, h = glfw.get_framebuffer_size(window)
    caravangl.viewport(x=0, y=0, width=w, height=h)
    caravangl.clear_color(0.1, 0.1, 0.1, 1)
    caravangl.clear(caravangl.COLOR_BUFFER_BIT | caravangl.DEPTH_BUFFER_BIT)
    
    tex.bind(0, samp)
    pipe.upload_uniforms(batch)
    pipe.draw()
    
    glfw.swap_buffers(window)

glfw.terminate()