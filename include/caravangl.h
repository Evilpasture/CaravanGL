#pragma once

/**
 * CaravanGL - Developer Guide (C Side)
 * ===================================
 *
 * 1. ARCHITECTURE
 *    CaravanGL uses isolated state per Python subinterpreter.
 *    Global OpenGL function pointers are strictly forbidden. All pointers
 *    live within 'CaravanState->gl'.
 *
 * 2. THE 'gl' TABLE PATTERN (C23)
 *    To call OpenGL, fetch the function table from the module object 'm'.
 *    This copies the pointer table (~1.6KB) to the stack for ultra-fast access.
 *
 *    Example:
 *      static PyObject* my_func(PyObject* m, PyObject* args) {
 *          auto gl = gl_table(m); // C23: auto infers 'CaravanGLTable'
 *          OpenGL->ClearColor(1, 0, 0, 1);
 *          OpenGL->Clear(GL_COLOR_BUFFER_BIT);
 *          Py_RETURN_NONE;
 *      }
 *
 * 3. ENGINE CONTEXT (STATE TRACKING)
 *    Use 'state->ctx' to track bound VAOs, Programs, and FBOs to minimize
 *    redundant driver calls.
 *
 *    Example:
 *      auto* state = get_caravan_state(m);
 *      if (state->ctx.bound.vao != vao_id) {
 *          state->OpenGL->BindVertexArray(vao_id);
 *          state->ctx.bound.vao = vao_id;
 *      }
 *
 * 4. OPTIONAL FUNCTIONS (CORE 4.2+ / EXTENSIONS)
 *    OpenGL 3.3 is the required baseline (macOS compatible).
 *    Always check if optional function pointers are non-null before calling.
 *
 *    Example:
 *      auto gl = gl_table(m);
 *      if (OpenGL->DispatchCompute) {
 *          OpenGL->DispatchCompute(x, y, z);
 *      }
 *
 * 5. UNIFORM DISPATCH
 *    Use 'uniform_upload_table' to handle different GLSL types generically.
 *    Requires passing the 'state' pointer to the wrapper.
 *
 *    Example:
 *      uniform_upload_table[UF_MAT4](state, location, count, matrix_ptr);
 *
 * 6. DEPENDENCY ORDER
 *    core -> specs -> module -> loader -> uniforms
 */

#include "caravangl_core.h"   // Basic GL Types, Constants, and X-Macros
#include "caravangl_loader.h" // Loader implementation (load_gl)
#include "caravangl_module.h" // Python State (CaravanState & CaravanGLTable)
#if defined(CARAVANGL_LOADER_ONLY)

#else
#include "caravangl_specs.h" // Engine Structs (Buffers, Context, Textures)
#endif
#include "caravangl_uniforms.h" // Uniform dispatch wrappers