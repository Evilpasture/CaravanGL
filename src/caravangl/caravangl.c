/**
 * ============================================================================
 * CaravanGL - Final Implementation
 * ============================================================================
 * Architecture: C23 + Python 3.14t (Free-Threaded)
 * Features: Isolated State, Fast-Path Parsing, Register-Speed Build, No-GIL.
 * ============================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <string.h>
#include "pycaravangl.h"
#include "caravangl_arg_indices.h"
#include "fast_build.h"
#include "caravangl_uniform_upload.h"
#include "caravangl_state.h"

// -----------------------------------------------------------------------------
// Internal Helpers
// -----------------------------------------------------------------------------

/**
 * Queries GPU hardware limits and populates the state context.
 * Called once during caravan.init().
 */
static void query_capabilities(PyObject *m) {
    WithCaravanGL(m, gl) {
        if (gl.GetIntegerv != nullptr) {
            // Textures
            gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &state->ctx.caps.max_texture_size);
            gl.GetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &state->ctx.caps.max_3d_texture_size);
            gl.GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &state->ctx.caps.max_array_texture_layers);
            gl.GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &state->ctx.caps.max_texture_units);
            
            // FBOs & Buffers
            gl.GetIntegerv(GL_MAX_SAMPLES, &state->ctx.caps.max_samples);
            gl.GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &state->ctx.caps.max_color_attachments);
            gl.GetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &state->ctx.caps.max_uniform_block_size);
            gl.GetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &state->ctx.caps.max_ubo_bindings);
            
            // Viewport
            GLint v[4] = {0};
            gl.GetIntegerv(GL_VIEWPORT, v);
            state->ctx.viewport = (CaravanRect){v[0], v[1], v[2], v[3]};
        }

#ifndef __APPLE__
        state->ctx.caps.support_compute = (gl.DispatchCompute != nullptr);
        state->ctx.caps.support_bindless = (gl.GetTextureHandleARB != nullptr);
        
        if (state->ctx.caps.support_compute && gl.GetIntegerv != nullptr) {
            gl.GetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &state->ctx.caps.max_compute_work_group_invocations);
            gl.GetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &state->ctx.caps.max_shader_storage_block_size);
        }
#else
        // Hardcode to false for Mac (OpenGL 4.1 limit)
        state->ctx.caps.support_compute = false;
        state->ctx.caps.support_bindless = false;
        state->ctx.caps.max_compute_work_group_invocations = 0;
        state->ctx.caps.max_shader_storage_block_size = 0;
#endif
    }
}

// -----------------------------------------------------------------------------
// Buffer Object Implementation (Heap Type)
// -----------------------------------------------------------------------------

/**
 * Buffer Deallocator: Cleans up GPU resources before the Python object is freed.
 */
static void Buffer_dealloc(PyCaravanBuffer *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    
    WithCaravanGL(m, gl) {
        if (self->buf.id != 0) {
            gl.DeleteBuffers(1, &self->buf.id);
            self->buf.id = 0;
        }
    }

    if (self->weakreflist != nullptr) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }

    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

/**
 * Buffer Init: Handles creation and initial data upload.
 * Signature: (self, args, kwds) -> Standard tp_init
 */
static int Buffer_init(PyCaravanBuffer *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        int size = 0;
        PyObject *data = nullptr;
        uint32_t target = GL_ARRAY_BUFFER;
        uint32_t usage = GL_STATIC_DRAW;

        void *targets[BufInit_COUNT] = {
            [IDX_BUF_SIZE]   = &size,
            [IDX_BUF_DATA]   = &data,
            [IDX_BUF_TARGET] = &target,
            [IDX_BUF_USAGE]  = &usage
        };

        // tp_init call: (args, kwds, kwnames=nullptr, parser, targets)
        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.BufInitParser, targets)) {
            return -1;
        }

        gl.GenBuffers(1, &self->buf.id);
        gl.BindBuffer(target, self->buf.id);

        const void *ptr = nullptr;
        Py_buffer view;
        if (data && data != Py_None) {
            if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) == 0) {
                ptr = view.buf;
            } else {
                return -1; // Fail initialization if buffer extraction fails
            }
        }

        gl.BufferData(target, (GLsizeiptr)size, ptr, usage);
        if (ptr) PyBuffer_Release(&view);

        self->buf.target = target;
        self->buf.size = (GLsizeiptr)size;
        self->buf.usage = usage;
    }
    return 0;
}

/**
 * Buffer Write: Updates a region of the buffer.
 * Signature: (self, args, nargs, kwnames) -> FastCall
 */
static PyObject* Buffer_write(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        PyObject *data = nullptr;
        int offset = 0;
        void *targets[BufWrite_COUNT] = {
            [IDX_BUF_WRITE_DATA]   = &data,
            [IDX_BUF_WRITE_OFFSET] = &offset
        };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufWriteParser, targets)) {
            return nullptr;
        }

        Py_buffer view;
        if (data == nullptr || PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) != 0) {
            PyErr_SetString(PyExc_TypeError, "data must support the buffer protocol");
            return nullptr;
        }

        if (offset + view.len > self->buf.size) {
            PyErr_SetString(PyExc_ValueError, "Data exceeds buffer size");
            PyBuffer_Release(&view);
            return nullptr;
        }

        gl.BindBuffer(self->buf.target, self->buf.id);
        gl.BufferSubData(self->buf.target, (GLintptr)offset, (GLsizeiptr)view.len, view.buf);
        PyBuffer_Release(&view);
    }
    Py_RETURN_NONE;
}

/**
 * Buffer Bind Base: Binds buffer to an indexed target (UBO/SSBO).
 * Signature: (self, args, nargs, kwnames) -> FastCall
 */
static PyObject* Buffer_bind_base(PyCaravanBuffer *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        uint32_t index = 0;
        void *targets[BufBind_COUNT] = { [IDX_BUF_BIND_IDX] = &index };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.BufBindParser, targets)) {
            return nullptr;
        }
        gl.BindBufferBase(self->buf.target, index, self->buf.id);
    }
    Py_RETURN_NONE;
}

static PyMethodDef Buffer_methods[] = {
    {"write", (PyCFunction)(void(*)(void))Buffer_write, METH_FASTCALL | METH_KEYWORDS, "Write data to buffer."},
    {"bind_base", (PyCFunction)(void(*)(void))Buffer_bind_base, METH_FASTCALL | METH_KEYWORDS, "Bind as indexed resource."},
    {nullptr}
};

static PyType_Slot Buffer_slots[] = {
    {Py_tp_init, Buffer_init},
    {Py_tp_dealloc, Buffer_dealloc},
    {Py_tp_methods, Buffer_methods},
    {Py_tp_doc, "CaravanGL Buffer Object (VBO, IBO, UBO)"},
    {0, nullptr}
};

static PyType_Spec Buffer_spec = {
    .name = "caravangl.Buffer",
    .basicsize = sizeof(PyCaravanBuffer),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Buffer_slots,
};

// -----------------------------------------------------------------------------
// Module-Level API
// -----------------------------------------------------------------------------

/**
 * caravan.init(loader)
 * Loads the function table and discovers GPU capabilities.
 */
static PyObject *caravan_init(PyObject *m, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames) {
    WithCaravanGL(m, gl) {
        PyObject *loader = nullptr;
        void *targets[Init_COUNT] = { [IDX_INIT_LOADER] = &loader };

        if (!FastParse_Unified(args, PyVectorcall_NARGS(nargsf), kwnames, &state->parsers.InitParser, targets)) {
            return nullptr;
        }

        if (load_gl(state, loader) < 0) return nullptr;
        query_capabilities(m);
    }
    Py_RETURN_NONE;
}

/**
 * caravangl.context() -> dict
 * Returns a snapshot of capabilities and driver info using FastBuild.
 */
static PyObject *caravan_context(PyObject *m, [[maybe_unused]] PyObject *const *args, [[maybe_unused]] Py_ssize_t nargs, [[maybe_unused]] PyObject *kwnames) {
    WithCaravanGL(m, gl) {
        if (gl.GetString == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "OpenGL not loaded. Call init() first.");
            return nullptr;
        }

        return FastBuild_Dict(
            "caps", FastBuild_Dict(
                "max_texture_size", state->ctx.caps.max_texture_size,
                "max_samples",      state->ctx.caps.max_samples,
                "support_compute",  state->ctx.caps.support_compute,
                "support_bindless", state->ctx.caps.support_bindless
            ),
            "info", FastBuild_Dict(
                "vendor",   (const char*)gl.GetString(GL_VENDOR),
                "renderer", (const char*)gl.GetString(GL_RENDERER),
                "version",  (const char*)gl.GetString(GL_VERSION)
            ),
            "viewport", FastBuild_Tuple(
                state->ctx.viewport.x, state->ctx.viewport.y,
                state->ctx.viewport.w, state->ctx.viewport.h
            )
        );
    }
    return nullptr;
}

static int Pipeline_init(PyCaravanPipeline *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        // Variables to parse into
        uint32_t program_id = 0, vao_id = 0;
        uint32_t topology = GL_TRIANGLES;
        uint32_t index_type = 0; // 0 means draw arrays (no index buffer)
        
        // Render State defaults
        int depth_test = 0, depth_write = 1;
        uint32_t depth_func = GL_LESS;
        int blend_enabled = 0;
        
        void *targets[PipelineInit_COUNT] = {
            [IDX_PL_PROGRAM] = &program_id,
            [IDX_PL_VAO]     = &vao_id,
            [IDX_PL_TOPO]    = &topology,
            [IDX_PL_IDX_TYP] = &index_type,
            [IDX_PL_DEPTH]   = &depth_test,
            [IDX_PL_DWRITE]  = &depth_write,
            [IDX_PL_DFUNC]   = &depth_func,
            [IDX_PL_BLEND]   = &blend_enabled
        };

        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.PipelineInitParser, targets)) {
            return -1;
        }

        // 1. Assign GPU Objects
        self->program = program_id;
        self->vao = vao_id;
        self->topology = topology;
        self->index_type = index_type;

        // 2. Pack the Render State
        self->render_state.depth_test_enabled = (bool)depth_test;
        self->render_state.depth_write_mask = (bool)depth_write;
        self->render_state.depth_func = depth_func;
        self->render_state.blend_enabled = (bool)blend_enabled;

        // 3. Initialize default draw params
        self->params = (CaravanDrawParams){
            .vertex_count = 0,
            .instance_count = 1,
            .first_vertex = 0,
            .base_instance = 0
        };

        // 4. Expose params as a writable Python memoryview (like HyperGL did)
        // This lets users do `pipeline.params[0] = 6` to change vertex count instantly!
        self->params_buffer.buf = &self->params;
        self->params_buffer.len = sizeof(CaravanDrawParams);
        self->params_buffer.readonly = 0;
        self->params_buffer.itemsize = sizeof(GLuint);
        self->params_buffer.format = "I"; // Unsigned Int
        self->params_buffer.ndim = 1;
        
        static Py_ssize_t shape = sizeof(CaravanDrawParams) / sizeof(GLuint);
        static Py_ssize_t stride = sizeof(GLuint);
        self->params_buffer.shape = &shape;
        self->params_buffer.strides = &stride;
        
        // FIX: Tie the buffer lifetime to the Pipeline object
        self->params_buffer.obj = (PyObject *)self;
        Py_INCREF(self);
        
        self->params_view = PyMemoryView_FromBuffer(&self->params_buffer);
    }
    return 0;
}

static PyObject* Pipeline_draw(PyCaravanPipeline *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    
    WithCaravanGL(m, gl) {
        // 0. Predictable Early-Out: If vertex or instance count is 0, do nothing.
        if (self->params.vertex_count == 0 || self->params.instance_count == 0) [[unlikely]] {
            Py_RETURN_NONE;
        }

        // 1. Lock the context state
        MagMutex_Lock(&state->ctx.state_lock);

        // 2. Bind core objects (These compile to near-zero cycles if already bound)
        cv_bind_program(state, self->program);
        cv_bind_vao(state, self->vao);

        // 3. Apply render states
        cv_set_depth_state(state, 
                           self->render_state.depth_test_enabled, 
                           self->render_state.depth_func, 
                           self->render_state.depth_write_mask);

        // ... Apply blend state, cull state, etc. ...

        // 4. Dispatch the Draw Call
        if (self->index_type != 0) {
            // Indexed Drawing (glDrawElements)
            
            // PREDICTABILITY FIX: Convert 'first_vertex' (element index) into a byte offset
            uintptr_t byte_offset = 0;
            switch (self->index_type) {
                case GL_UNSIGNED_INT:   byte_offset = self->params.first_vertex * sizeof(GLuint);   break;
                case GL_UNSIGNED_SHORT: byte_offset = self->params.first_vertex * sizeof(GLushort); break;
                case GL_UNSIGNED_BYTE:  byte_offset = self->params.first_vertex * sizeof(GLubyte);  break;
            }
            void *index_ptr = (void *)byte_offset;

            if (self->params.instance_count > 1) {
                gl.DrawElementsInstanced(self->topology, 
                                         self->params.vertex_count, 
                                         self->index_type, 
                                         index_ptr, 
                                         self->params.instance_count);
            } else {
                gl.DrawElements(self->topology, 
                                self->params.vertex_count, 
                                self->index_type, 
                                index_ptr);
            }
        } else {
            // Array Drawing (glDrawArrays)
            
            if (self->params.instance_count > 1) {
                gl.DrawArraysInstanced(self->topology, 
                                       self->params.first_vertex, 
                                       self->params.vertex_count, 
                                       self->params.instance_count);
            } else {
                gl.DrawArrays(self->topology, 
                              self->params.first_vertex, 
                              self->params.vertex_count);
            }
        }

        // 5. Unlock the context
        MagMutex_Unlock(&state->ctx.state_lock);
    }
    Py_RETURN_NONE;
}

/**
 * caravangl.inspect(obj) -> dict | None
 * Returns a dictionary containing the internal OpenGL IDs and state of a Caravan object.
 */
static PyObject *caravan_inspect(PyObject *m, PyObject *arg) {
    CaravanState *state = get_caravan_state(m);
    if (!state) return nullptr;

    PyTypeObject *type = Py_TYPE(arg);

    // -------------------------------------------------------------------------
    // BUFFER
    // -------------------------------------------------------------------------
    if (type == state->BufferType) {
        PyCaravanBuffer *b = (PyCaravanBuffer *)arg;
        return FastBuild_Dict(
            "type",   (const char *)"buffer",
            "id",     (long long)b->buf.id,
            "target", (long long)b->buf.target,
            "size",   (long long)b->buf.size,
            "usage",  (long long)b->buf.usage
        );
    }

    // -------------------------------------------------------------------------
    // PIPELINE
    // -------------------------------------------------------------------------
    if (type == state->PipelineType) { // Assumes PipelineType is stored in CaravanState
        PyCaravanPipeline *p = (PyCaravanPipeline *)arg;
        
        // Return a structured dictionary reflecting the exact C struct
        return FastBuild_Dict(
            "type",       (const char *)"pipeline",
            "program",    (long long)p->program,
            "vao",        (long long)p->vao,
            "topology",   (long long)p->topology,
            "index_type", (long long)p->index_type,
            
            "render_state", FastBuild_Dict(
                "depth_test",  p->render_state.depth_test_enabled,
                "depth_write", p->render_state.depth_write_mask,
                "depth_func",  (long long)p->render_state.depth_func,
                "blend",       p->render_state.blend_enabled
            ),
            
            "draw_params", FastBuild_Dict(
                "vertex_count",   (long long)p->params.vertex_count,
                "instance_count", (long long)p->params.instance_count,
                "first_vertex",   (long long)p->params.first_vertex,
                "base_instance",  (long long)p->params.base_instance
            )
        );
    }

    // TODO: Add Texture / Framebuffer / Compute inspections here as you build them.

    // If we don't recognize the object, return None natively.
    Py_RETURN_NONE;
}

static PyObject *caravan_test_render(PyObject *m, [[maybe_unused]] PyObject *const *args, [[maybe_unused]] Py_ssize_t nargs, [[maybe_unused]] PyObject *kwnames) {
    WithCaravanGL(m, gl) {
        if (gl.ClearBufferfv == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "GL not initialized");
            return nullptr;
        }
        const GLfloat clear_color[] = {0.1f, 0.2f, 0.3f, 1.0f};
        gl.ClearBufferfv(GL_COLOR, 0, clear_color);
        Py_RETURN_NONE;
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
// Program Object
// -----------------------------------------------------------------------------

static GLuint compile_shader(CaravanGLTable *gl, GLenum type, const char *source) {
    GLuint shader = gl->CreateShader(type);
    gl->ShaderSource(shader, 1, &source, nullptr);
    gl->CompileShader(shader);

    GLint success;
    gl->GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        gl->GetShaderInfoLog(shader, 512, nullptr, info_log);
        PyErr_Format(PyExc_RuntimeError, "Shader Compilation Failed:\n%s", info_log);
        gl->DeleteShader(shader);
        return 0;
    }
    return shader;
}

static int Program_init(PyCaravanProgram *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        const char *vs_src = nullptr;
        const char *fs_src = nullptr;

        void *targets[ProgInit_COUNT] = {
            [IDX_PROG_VS] = &vs_src,
            [IDX_PROG_FS] = &fs_src
        };

        if (!FastParse_Unified(args, kwds, nullptr, &state->parsers.ProgInitParser, targets)) return -1;

        GLuint vs = compile_shader(&gl, GL_VERTEX_SHADER, vs_src);
        if (!vs) return -1;
        GLuint fs = compile_shader(&gl, GL_FRAGMENT_SHADER, fs_src);
        if (!fs) { gl.DeleteShader(vs); return -1; }

        self->id = gl.CreateProgram();
        gl.AttachShader(self->id, vs);
        gl.AttachShader(self->id, fs);
        gl.LinkProgram(self->id);

        // Check Link Status
        GLint success;
        gl.GetProgramiv(self->id, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            gl.GetProgramInfoLog(self->id, 512, nullptr, info_log);
            PyErr_Format(PyExc_RuntimeError, "Program Linking Failed:\n%s", info_log);
            return -1;
        }

        // Clean up shaders
        gl.DeleteShader(vs);
        gl.DeleteShader(fs);
    }
    return 0;
}

static void Program_dealloc(PyCaravanProgram *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    WithCaravanGL(m, gl) {
        if (self->id) gl.DeleteProgram(self->id);
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

// Quick helper to fetch a uniform location from Python
static PyObject* Program_get_uniform_location(PyCaravanProgram *self, PyObject *arg) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "Uniform name must be a string");
        return nullptr;
    }
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        const char *name = PyUnicode_AsUTF8(arg);
        GLint loc = gl.GetUniformLocation(self->id, name);
        return PyLong_FromLong(loc);
    }
    return nullptr;
}

static PyMethodDef Program_methods[] = {
    {"get_uniform_location", (PyCFunction)Program_get_uniform_location, METH_O, "Get uniform location by name"},
    {}
};

static PyType_Slot Program_slots[] = {
    {Py_tp_init, Program_init},
    {Py_tp_dealloc, Program_dealloc},
    {Py_tp_methods, Program_methods},
    {}
};

static PyType_Spec Program_spec = {
    .name = "caravangl.Program", .basicsize = sizeof(PyCaravanProgram),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, .slots = Program_slots,
};

// -----------------------------------------------------------------------------
// VertexArray Object
// -----------------------------------------------------------------------------

static int VertexArray_init(PyCaravanVertexArray *self, PyObject *args, PyObject *kwds) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        gl.GenVertexArrays(1, &self->id);
    }
    return 0;
}

static void VertexArray_dealloc(PyCaravanVertexArray *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *m = PyType_GetModule(tp);
    WithCaravanGL(m, gl) {
        if (self->id) gl.DeleteVertexArrays(1, &self->id);
    }
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

static PyObject* VertexArray_bind_attribute(PyCaravanVertexArray *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *m = PyType_GetModule(Py_TYPE(self));
    WithCaravanGL(m, gl) {
        uint32_t location = 0, type = GL_FLOAT;
        int size = 0, normalized = 0, stride = 0, offset = 0;
        PyObject *py_buffer = nullptr;

        void *targets[VaoAttr_COUNT] = {
            [IDX_VAO_ATTR_LOC]    = &location,
            [IDX_VAO_ATTR_BUF]    = &py_buffer,
            [IDX_VAO_ATTR_SIZE]   = &size,
            [IDX_VAO_ATTR_TYPE]   = &type,
            [IDX_VAO_ATTR_NORM]   = &normalized,
            [IDX_VAO_ATTR_STRIDE] = &stride,
            [IDX_VAO_ATTR_OFFSET] = &offset
        };

        if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.VaoAttrParser, targets)) return nullptr;

        // Verify it's actually our Buffer object
        if (Py_TYPE(py_buffer) != state->BufferType) {
            PyErr_SetString(PyExc_TypeError, "buffer must be a caravangl.Buffer");
            return nullptr;
        }
        PyCaravanBuffer *buf = (PyCaravanBuffer *)py_buffer;

        // Bind VAO and VBO, then map the attribute
        gl.BindVertexArray(self->id);
        gl.BindBuffer(GL_ARRAY_BUFFER, buf->buf.id);
        
        gl.EnableVertexAttribArray(location);
        gl.VertexAttribPointer(location, size, type, normalized ? GL_TRUE : GL_FALSE, stride, (void*)(uintptr_t)offset);
        
        // Unbind VAO to prevent accidental corruption
        gl.BindVertexArray(0);
    }
    Py_RETURN_NONE;
}

static PyMethodDef VertexArray_methods[] = {
    {"bind_attribute", (PyCFunction)(void(*)(void))VertexArray_bind_attribute, METH_FASTCALL | METH_KEYWORDS, "Map a VBO to a shader attribute"},
    {nullptr}
};

static PyType_Slot VertexArray_slots[] = {
    {Py_tp_init, VertexArray_init},
    {Py_tp_dealloc, VertexArray_dealloc},
    {Py_tp_methods, VertexArray_methods},
    {0, nullptr}
};

static PyType_Spec VertexArray_spec = {
    .name = "caravangl.VertexArray", .basicsize = sizeof(PyCaravanVertexArray),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, .slots = VertexArray_slots,
};

/**
 * Pipeline Deallocator
 */
static void Pipeline_dealloc(PyCaravanPipeline *self) {
    PyTypeObject *tp = Py_TYPE(self);
    
    // Clean up the memoryview
    Py_XDECREF(self->params_view);

    // Release context reference
    PyObject *m = PyType_GetModule(tp);
    // (If you need to delete GL objects like programs/vaos, do it here with WithCaravanGL)

    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

/**
 * Getter for .params: Exposes the memoryview for zero-copy mutation
 */
static PyObject* Pipeline_get_params(PyCaravanPipeline *self, void *closure) {
    if (self->params_view) {
        return Py_NewRef(self->params_view);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef Pipeline_getset[] = {
    {"params", (getter)Pipeline_get_params, nullptr, "Direct access to draw parameters", nullptr},
    {}
};

static PyMethodDef Pipeline_methods[] = {
    {"draw", (PyCFunction)(void(*)(void))Pipeline_draw, METH_NOARGS, "Execute the draw call."},
    {}
};

static PyType_Slot Pipeline_slots[] = {
    {Py_tp_init,    Pipeline_init},
    {Py_tp_dealloc, Pipeline_dealloc},
    {Py_tp_methods, Pipeline_methods},
    {Py_tp_getset,  Pipeline_getset},
    {Py_tp_doc,     "CaravanGL Pipeline: Immutable Draw State"},
    {}
};

static PyType_Spec Pipeline_spec = {
    .name      = "caravangl.Pipeline",
    .basicsize = sizeof(PyCaravanPipeline),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots     = Pipeline_slots,
};

static PyMethodDef caravan_methods[] = {
    {"init", (PyCFunction)(void(*)(void))caravan_init, METH_FASTCALL | METH_KEYWORDS, "Initialize loader"},
    {"context", (PyCFunction)(void(*)(void))caravan_context, METH_FASTCALL | METH_KEYWORDS, "Get capabilities"},
    {"inspect",     (PyCFunction)caravan_inspect,                    METH_O, "Inspect internal C/GL state"},
    {"test_render", (PyCFunction)(void(*)(void))caravan_test_render, METH_FASTCALL | METH_KEYWORDS, "Test render"},
    {nullptr, nullptr, 0, nullptr}
};

// -----------------------------------------------------------------------------
// Module Lifecycle
// -----------------------------------------------------------------------------

static int caravan_exec(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state == nullptr) return -1;
    
    memset(state, 0, sizeof(CaravanState));
    
    // Initialize the FastParse registry
    caravan_init_parsers(&state->parsers);

    // Register the Buffer heap type
    state->BufferType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Buffer_spec, nullptr);
    if (!state->BufferType) return -1;
    if (PyModule_AddObjectRef(m, "Buffer", (PyObject *)state->BufferType) < 0) return -1;

    state->PipelineType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Pipeline_spec, nullptr);
    if (!state->PipelineType) return -1;
    if (PyModule_AddObjectRef(m, "Pipeline", (PyObject *)state->PipelineType) < 0) return -1;

    PyTypeObject *ProgType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &Program_spec, nullptr);
    if (PyModule_AddObjectRef(m, "Program", (PyObject *)ProgType) < 0) return -1;
    Py_DECREF(ProgType);

    PyTypeObject *VaoType = (PyTypeObject *)PyType_FromModuleAndSpec(m, &VertexArray_spec, nullptr);
    if (PyModule_AddObjectRef(m, "VertexArray", (PyObject *)VaoType) < 0) return -1;
    Py_DECREF(VaoType);
    
    // EXPORT GL CONSTANTS TO PYTHON
    PyModule_AddIntConstant(m, "FLOAT", GL_FLOAT);
    PyModule_AddIntConstant(m, "TRIANGLES", GL_TRIANGLES);

    return 0;
}

static int caravan_traverse(PyObject *m, visitproc visit, void *arg) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        Py_VISIT(state->BufferType);
        Py_VISIT(state->PipelineType);
    }
    return 0;
}

static int caravan_clear(PyObject *m) {
    CaravanState *state = get_caravan_state(m);
    if (state) {
        caravan_free_parsers(&state->parsers);
        Py_CLEAR(state->BufferType);
        Py_CLEAR(state->PipelineType);
    }
    return 0;
}

static PyModuleDef_Slot caravan_slots[] = {
    {Py_mod_exec, caravan_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {}
};

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

PyMODINIT_FUNC PyInit_caravangl(void) {
    return PyModuleDef_Init(&caravan_module);
}