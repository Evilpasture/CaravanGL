import glfw
import ctypes
import struct
import caravangl

# 1. Setup GLFW Context
glfw.init()
glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE) # Needed for Mac
window = glfw.create_window(800, 600, "CaravanGL Triangle", None, None)
glfw.make_context_current(window)

# 2. Init Caravan
class GLLoader:
    def load_opengl_function(self, name: str) -> int:
        ptr = glfw.get_proc_address(name)
        return ctypes.cast(ptr, ctypes.c_void_p).value if ptr else 0

caravangl.init(loader=GLLoader())

# 3. Create the Shader Program
VS = """
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vertexColor;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    vertexColor = aColor;
}
"""

FS = """
#version 330 core
out vec4 FragColor;
in vec3 vertexColor;
void main() {
    FragColor = vec4(vertexColor, 1.0);
}
"""
program = caravangl.Program(vertex_shader=VS, fragment_shader=FS)

# 4. Create the Vertex Data (Interleaved: X, Y, R, G, B)
# 3 vertices * 5 floats = 15 floats (60 bytes)
vertices = struct.pack(
    "15f",
    -0.5, -0.5,  1.0, 0.0, 0.0, # Bottom Left (Red)
     0.5, -0.5,  0.0, 1.0, 0.0, # Bottom Right (Green)
     0.0,  0.5,  0.0, 0.0, 1.0  # Top Center (Blue)
)

vbo = caravangl.Buffer(size=60, data=vertices)

# 5. Create VAO and Map Attributes
vao = caravangl.VertexArray()

# Attribute 0: Position (2 floats)
vao.bind_attribute(
    location=0, buffer=vbo, size=2, type=caravangl.FLOAT, 
    stride=20, offset=0  # 20 bytes per vertex (5 floats * 4 bytes)
)

# Attribute 1: Color (3 floats)
vao.bind_attribute(
    location=1, buffer=vbo, size=3, type=caravangl.FLOAT, 
    stride=20, offset=8  # Starts after the 2 position floats (2 * 4 bytes)
)

# 6. Create the Draw Pipeline
pipeline = caravangl.Pipeline(
    program=program, 
    vao=vao,
    topology=caravangl.TRIANGLES
)


# Mutate the memoryview to set vertex count to 3
pipeline.params[0] = 3
GL_COLOR_BUFFER_BIT = 0x00004000
# 7. Main Render Loop
while not glfw.window_should_close(window):
    glfw.poll_events()
    
    caravangl.clear_color(0, 0, 0, 1)
    caravangl.clear(GL_COLOR_BUFFER_BIT)
    
    # THE DRAW CALL
    pipeline.draw()
    
    glfw.swap_buffers(window)

glfw.terminate()