import glfw
import struct
import math
import time
import caravangl
import ctypes

# 1. Standard Setup
glfw.init()
glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE) 
window = glfw.create_window(800, 600, "CaravanGL FBO Test", None, None)
glfw.make_context_current(window)

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

caravangl.init(loader=GLLoader())

# 2. SHADERS
# Shader 1: Normal Triangle
VS_TRI = """#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vCol;
void main() { gl_Position = vec4(aPos, 0.0, 1.0); vCol = aColor; }"""

FS_TRI = """#version 330 core
in vec3 vCol;
out vec4 fCol;
void main() { fCol = vec4(vCol, 1.0); }"""

# Shader 2: Post-Process (Inversion)
VS_POST = """#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;
out vec2 vTex;
void main() { gl_Position = vec4(aPos, 0.0, 1.0); vTex = aTex; }"""

FS_POST = """#version 330 core
uniform sampler2D u_screen;
in vec2 vTex;
out vec4 fCol;
void main() {
    vec4 scene = texture(u_screen, vTex);
    fCol = vec4(1.0 - scene.rgb, 1.0); // INVERT COLORS
}"""

prog_tri = caravangl.Program(vertex_shader=VS_TRI, fragment_shader=FS_TRI)
prog_post = caravangl.Program(vertex_shader=VS_POST, fragment_shader=FS_POST)

# 3. GEOMETRY
# Triangle VBO
tri_data = struct.pack("15f", -0.5, -0.5, 1,0,0,  0.5, -0.5, 0,1,0,  0.0, 0.5, 0,0,1)
vbo_tri = caravangl.Buffer(size=len(tri_data), data=tri_data)
vao_tri = caravangl.VertexArray()
vao_tri.bind_attribute(location=0, buffer=vbo_tri, size=2, type=caravangl.FLOAT, stride=20, offset=0)
vao_tri.bind_attribute(location=1, buffer=vbo_tri, size=3, type=caravangl.FLOAT, stride=20, offset=8)

# Full-screen Quad (Post-process target)
# Pos(X,Y), UV(U,V)
quad_data = struct.pack("24f",
    -1, -1, 0, 0,   1, -1, 1, 0,   1,  1, 1, 1,
    -1, -1, 0, 0,   1,  1, 1, 1,  -1,  1, 0, 1
)
vbo_quad = caravangl.Buffer(size=len(quad_data), data=quad_data)
vao_quad = caravangl.VertexArray()
vao_quad.bind_attribute(location=0, buffer=vbo_quad, size=2, type=caravangl.FLOAT, stride=16, offset=0)
vao_quad.bind_attribute(location=1, buffer=vbo_quad, size=2, type=caravangl.FLOAT, stride=16, offset=8)

# 4. FRAMEBUFFER SETUP
# Create the texture we will draw into
target_tex = caravangl.Texture(target=caravangl.TEXTURE_2D)
target_tex.upload(
    width=800, height=600,
    internal_format=caravangl.RGBA8,
    format=caravangl.RGBA,
    type=caravangl.UNSIGNED_BYTE,
    data=None # Allocate memory on GPU, don't upload from CPU
)

fbo = caravangl.Framebuffer()
fbo.attach_texture(attachment=caravangl.COLOR_ATTACHMENT0, texture=target_tex)
fbo.check_status()

# 5. PIPELINES
pip_tri = caravangl.Pipeline(program=prog_tri, vao=vao_tri)
pip_tri.params[0] = 3

pip_post = caravangl.Pipeline(program=prog_post, vao=vao_quad)
pip_post.params[0] = 6

# 6. LOOP
while not glfw.window_should_close(window):
    glfw.poll_events()

    # --- PASS 1: Scene (FBO is 800x600) ---
    fbo.bind()
    caravangl.viewport(x=0, y=0, w=800, h=600)
    caravangl.clear_color(0.1, 0.1, 0.1, 1.0)
    caravangl.clear(caravangl.COLOR_BUFFER_BIT)
    pip_tri.draw()

    # --- PASS 2: Post-Process (Screen is Retina size) ---
    caravangl.bind_default_framebuffer()
    # CRITICAL: Ask GLFW for the actual PIXEL size, not the window size
    px_w, px_h = glfw.get_framebuffer_size(window)
    caravangl.viewport(x=0, y=0, w=px_w, h=px_h)
    
    caravangl.clear_color(0, 0, 0, 1)
    caravangl.clear(caravangl.COLOR_BUFFER_BIT)
    
    target_tex.bind(unit=0)
    pip_post.draw()

    glfw.swap_buffers(window)

glfw.terminate()