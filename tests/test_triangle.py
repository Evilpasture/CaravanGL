import glfw
import struct
import math
import time
import ctypes
import caravangl

# 1. Setup GLFW for macOS Core Profile
if not glfw.init():
    raise RuntimeError("Failed to init GLFW")

glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)
window = glfw.create_window(800, 600, "CaravanGL Final Verification", None, None)
glfw.make_context_current(window)

class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

caravangl.init(loader=GLLoader())

# 2. Shaders
VS_SRC = """#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vCol;
void main() { gl_Position = vec4(aPos, 0.0, 1.0); vCol = aColor; }""".strip()

FS_SRC = """#version 330 core
in vec3 vCol;
out vec4 fCol;
void main() { fCol = vec4(vCol, 1.0); }""".strip()

VS_POST = """#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;
out vec2 vTex;
void main() { gl_Position = vec4(aPos, 0.0, 1.0); vTex = aTex; }""".strip()

FS_POST = """#version 330 core
uniform sampler2D u_screen;
in vec2 vTex;
out vec4 fCol;
void main() {
    vec4 scene = texture(u_screen, vTex);
    fCol = vec4(1.0 - scene.rgb, 1.0); 
}""".strip()

prog_scene = caravangl.Program(vertex_shader=VS_SRC, fragment_shader=FS_SRC)
prog_post = caravangl.Program(vertex_shader=VS_POST, fragment_shader=FS_POST)

# 3. Geometry (Using Keyword Args for bind_attribute)

# Rainbow Triangle Data (Pos: 2f, Color: 3f)
tri_data = struct.pack("15f", 
    -0.8, -0.8, 1,0,0,  # Red
     0.8, -0.8, 0,1,0,  # Green
     0.0,  0.8, 0,0,1   # Blue
)
vbo_tri = caravangl.Buffer(size=len(tri_data), data=tri_data)
vao_tri = caravangl.VertexArray()
vao_tri.bind_attribute(location=0, buffer=vbo_tri, size=2, type=caravangl.FLOAT, stride=20, offset=0)
vao_tri.bind_attribute(location=1, buffer=vbo_tri, size=3, type=caravangl.FLOAT, stride=20, offset=8)

# Center Square Mask (Pos: 2f, Color: 3f - Color added for shader compatibility)
mask_data = struct.pack("30f", 
    -0.4, -0.4, 1,0,0,   0.4, -0.4, 1,0,0,   0.4,  0.4, 1,0,0,
    -0.4, -0.4, 1,0,0,   0.4,  0.4, 1,0,0,  -0.4,  0.4, 1,0,0
)
vbo_mask = caravangl.Buffer(size=len(mask_data), data=mask_data)
vao_mask = caravangl.VertexArray()
vao_mask.bind_attribute(location=0, buffer=vbo_mask, size=2, type=caravangl.FLOAT, stride=20, offset=0)
vao_mask.bind_attribute(location=1, buffer=vbo_mask, size=3, type=caravangl.FLOAT, stride=20, offset=8)

# Full-Screen Quad (Pos: 2f, UV: 2f)
quad_data = struct.pack("24f",
    -1, -1, 0, 0,   1, -1, 1, 0,   1,  1, 1, 1,
    -1, -1, 0, 0,   1,  1, 1, 1,  -1,  1, 0, 1
)
vbo_quad = caravangl.Buffer(size=len(quad_data), data=quad_data)
vao_quad = caravangl.VertexArray()
vao_quad.bind_attribute(location=0, buffer=vbo_quad, size=2, type=caravangl.FLOAT, stride=16, offset=0)
vao_quad.bind_attribute(location=1, buffer=vbo_quad, size=2, type=caravangl.FLOAT, stride=16, offset=8)

# 4. Framebuffer with Color and packed Depth+Stencil
target_tex = caravangl.Texture(target=caravangl.TEXTURE_2D)
target_tex.upload(width=800, height=600, internal_format=caravangl.RGBA8, format=caravangl.RGBA, type=caravangl.UNSIGNED_BYTE, data=None)

# Packing Depth (24 bit) and Stencil (8 bit) into one texture
depth_stencil_tex = caravangl.Texture(target=caravangl.TEXTURE_2D)
depth_stencil_tex.upload(
    width=800, height=600, 
    internal_format=caravangl.DEPTH24_STENCIL8, 
    format=caravangl.DEPTH_STENCIL, 
    type=caravangl.UNSIGNED_INT_24_8, 
    data=None
)

fbo = caravangl.Framebuffer()
fbo.attach_texture(attachment=caravangl.COLOR_ATTACHMENT0, texture=target_tex)
fbo.attach_texture(attachment=caravangl.DEPTH_STENCIL_ATTACHMENT, texture=depth_stencil_tex)
fbo.check_status()

# 5. Pipelines (Exercising Render State Logic)

# Pipeline 1: Write '1' to stencil where the square is. 
# We disable color/depth write so the square itself is invisible, but the mask is set.
pip_mask = caravangl.Pipeline(
    program=prog_scene, vao=vao_mask,
    depth_test=0,
    depth_write=0, 
    stencil_test=1,
    stencil_func=caravangl.ALWAYS, stencil_ref=1, 
    stencil_zpass=caravangl.REPLACE
)
pip_mask.params[0] = 6

# Pipeline 2: Only draw where stencil is NOT 1
pip_tri = caravangl.Pipeline(
    program=prog_scene, vao=vao_tri,
    stencil_test=1,
    stencil_func=caravangl.NOTEQUAL, stencil_ref=1
)
pip_tri.params[0] = 3

# Pipeline 3: Post-process sampling
pip_post = caravangl.Pipeline(program=prog_post, vao=vao_quad)
pip_post.params[0] = 6

# 6. Render Loop
while not glfw.window_should_close(window):
    glfw.poll_events()

    # --- PASS 1: Scene (Draw to 800x600 FBO) ---
    fbo.bind()
    caravangl.viewport(x=0, y=0, w=800, h=600)
    
    # Clear Color (Grey), Depth, and Stencil
    caravangl.clear_color(0.1, 0.1, 0.1, 1.0)
    caravangl.clear(caravangl.COLOR_BUFFER_BIT | caravangl.DEPTH_BUFFER_BIT | caravangl.STENCIL_BUFFER_BIT)
    
    # Draw Mask (Invisible, but sets stencil bits)
    pip_mask.draw()
    
    # Draw Triangle (Only visible outside the square hole)
    pip_tri.draw()

    # --- PASS 2: Post-Process (Draw to Retina Screen) ---
    caravangl.bind_default_framebuffer()
    px_w, px_h = glfw.get_framebuffer_size(window)
    caravangl.viewport(x=0, y=0, w=px_w, h=px_h)
    
    caravangl.clear_color(0, 0, 0, 1)
    caravangl.clear(caravangl.COLOR_BUFFER_BIT)
    
    target_tex.bind(unit=0)
    pip_post.draw()

    glfw.swap_buffers(window)

glfw.terminate()