import glfw
import caravangl
import sys

def main():
    # 1. Initialize GLFW and create a window (context)
    if not glfw.init():
        return

    # Create a hidden window just for the context
    glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
    # Ensure these hints are set before window creation
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 4)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 1)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE) # Required for macOS

    window = glfw.create_window(640, 480, "CaravanGL Hidden", None, None)
    if not window:
        glfw.terminate()
        return

    glfw.make_context_current(window)

    # 2. Define the Loader Object your C code expects
    class GLLoader:
        def load_opengl_function(self, name):
            # Fetch the address from GLFW
            addr = glfw.get_proc_address(name)
            return addr if addr is not None else 0


    print("Initializing CaravanGL C-Extension...")
    # This calls your caravan_init -> load_gl logic
    caravangl.init(GLLoader())

    print("Triggering Test Render...")
    # This calls state->gl.ClearColor and state->gl.Clear
    caravangl.test_render()
    
    print("Success! The C-Extension is communicating with the GPU driver.")
    
    glfw.terminate()

if __name__ == "__main__":
    main()