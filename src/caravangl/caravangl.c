#define PY_SSIZE_T_CLEAN
#include "caravangl.h"
#include <Python.h>
#include <stdio.h>

/**
 * caravan.init(loader)
 * Initializes the per-interpreter OpenGL function table.
 */
static PyObject *caravan_init(PyObject *m, PyObject *loader) {
  WithCaravanGL(m, gl) {

    if (load_gl(state, loader) < 0) {
      return nullptr; // Error already set by load_gl
    }
  }

  printf("CaravanGL: Subinterpreter state initialized.\n");
  Py_RETURN_NONE;
}

/**
 * caravan.test_render()
 * Proof-of-concept using the inferred 'gl' table.
 */
static PyObject *caravan_test_render(PyObject *m,
                                     [[maybe_unused]] PyObject *args) {
  WithCaravanGL(m, gl) {
    const GLfloat clear_color[] = {0.1f, 0.2f, 0.3f, 1.0f};
    gl.ClearBufferfv(GL_COLOR, 0, clear_color);
    GLint v[4];
    gl.GetIntegerv(GL_VIEWPORT, v);
  
  printf("CaravanGL: Modern ClearBuffer. Viewport: %d %d %d %d\n", v[0], v[1],
         v[2], v[3]);
  }
  Py_RETURN_NONE;
}

// --- Module Method Table ---

static PyMethodDef caravan_methods[] = {
    {"init", (PyCFunction)caravan_init, METH_O, "Initialize OpenGL loader"},
    {"test_render", (PyCFunction)caravan_test_render, METH_NOARGS,
     "Test render call"},
    {nullptr, nullptr, 0, nullptr}};

// --- Multi-phase Initialization Slots (C23) ---

static int caravan_exec(PyObject *m) {
  auto *state = get_caravan_state(m);

  // Initialize context state
  state->ctx =
      (CaravanContext){.last_error = GL_NO_ERROR, .viewport = {0, 0, 0, 0}};

  return 0; // Success
}

static int caravan_traverse(PyObject *m, visitproc visit, void *arg) {
  auto *state = get_caravan_state(m);
  // Add visits here when you add heap types (state->BufferType)
  return 0;
}

static int caravan_clear(PyObject *m) {
  // Add clears here for heap types
  return 0;
}

// ... inside caravan_slots ...
static PyModuleDef_Slot caravan_slots[] = {
    {Py_mod_exec, caravan_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED}, // Tells 3.14 we are GIL-safe
#endif
    {0, nullptr}};

static struct PyModuleDef caravan_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "caravangl",
    .m_doc = "CaravanGL: Modern Isolated OpenGL Context",
    .m_size = sizeof(CaravanState),
    .m_methods = caravan_methods,
    .m_slots = caravan_slots,
    .m_traverse = caravan_traverse,
    .m_clear = caravan_clear,
    .m_free = nullptr,
};

// --- Module Entry Point ---

PyMODINIT_FUNC PyInit_caravangl(void) {
  return PyModuleDef_Init(&caravan_module);
}