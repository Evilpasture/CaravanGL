#include "caravangl_context.h"
#include "caravangl_state.h"
#include "pycaravangl.h"

// -----------------------------------------------------------------------------
// Framebuffer Object (FBO)
// -----------------------------------------------------------------------------

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PyCaravanGL_Status Framebuffer_init(PyCaravanFramebuffer *self, PyObject *args, PyObject *kwds) {
    // No parsing needed for init, just context activation
    WithActiveGL(OpenGL, cv_state, -1) {
        self->owning_context = (PyCaravanContext *)Py_NewRef(_cv_ctx);
        OpenGL->GenFramebuffers(1, &self->fbo.id);
    }
    return 0;
}

static int Framebuffer_traverse(PyCaravanFramebuffer *self, visitproc visit, void *arg) {
    Py_VISIT(self->owning_context);
    return 0;
}

static int Framebuffer_clear(PyCaravanFramebuffer *self) {
    Py_CLEAR(self->owning_context);
    return 0;
}

PyCaravanGL_Slot Framebuffer_dealloc(PyCaravanFramebuffer *self) {
    // Thread-safe deferred deletion
    CV_SAFE_DEALLOC(self, fbo.id, fbo_count, fbos, OpenGL->DeleteFramebuffers(1, &self->fbo.id));

    Py_XDECREF(self->owning_context);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyCaravanGL_API Framebuffer_attach_texture(PyCaravanFramebuffer *self, PyObject *const *args,
                                           Py_ssize_t nargs, PyObject *kwnames) {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    CaravanState *state = get_caravan_state(mod);

    uint32_t attachment = 0;
    PyObject *py_tex = nullptr;
    int level = 0;

    void *targets[FboAttach_COUNT] = {[IDX_FBO_ATT_ATTACH] = &attachment,
                                      [IDX_FBO_ATT_TEX] = (void *)&py_tex,
                                      [IDX_FBO_ATT_LEVEL] = &level};

    if (!FastParse_Unified(args, nargs, kwnames, &state->parsers.FboAttachParser, targets)) {
        return nullptr;
    }

    if (!Py_IS_TYPE(py_tex, state->TextureType)) {
        PyErr_SetString(PyExc_TypeError, "texture must be a caravangl.Texture");
        return nullptr;
    }

    PyCaravanTexture *tex = (PyCaravanTexture *)py_tex;

    WithActiveGL(OpenGL, cv_state, nullptr) {
        if (_cv_ctx != self->owning_context) [[clang::unlikely]] {
            PyErr_SetString(PyExc_RuntimeError, "Framebuffer context mismatch.");
            return nullptr;
        }
        cv_bind_fbo_combined(cv_state, OpenGL, self->fbo.id);
        OpenGL->FramebufferTexture2D(GL_FRAMEBUFFER, attachment, tex->tex.target, tex->tex.id,
                                     level);

        switch (attachment) {
        // Dense range: GL_COLOR_ATTACHMENT0 is 0x8CE0, 1 is 0x8CE1, etc.
        case GL_COLOR_ATTACHMENT0:
        case GL_COLOR_ATTACHMENT1:
        case GL_COLOR_ATTACHMENT2:
        case GL_COLOR_ATTACHMENT3:
        case GL_COLOR_ATTACHMENT4:
        case GL_COLOR_ATTACHMENT5:
        case GL_COLOR_ATTACHMENT6:
        case GL_COLOR_ATTACHMENT7:
        case GL_COLOR_ATTACHMENT8:
        case GL_COLOR_ATTACHMENT9:
        case GL_COLOR_ATTACHMENT10:
        case GL_COLOR_ATTACHMENT11:
        case GL_COLOR_ATTACHMENT12:
        case GL_COLOR_ATTACHMENT13:
        case GL_COLOR_ATTACHMENT14:
        case GL_COLOR_ATTACHMENT15:
            self->fbo.color_attachments_count++;
            break;

        case GL_DEPTH_ATTACHMENT:
            self->fbo.has_depth = true;
            break;

        case GL_STENCIL_ATTACHMENT:
            self->fbo.has_stencil = true;
            break;

        case GL_DEPTH_STENCIL_ATTACHMENT:
            self->fbo.has_depth = true;
            self->fbo.has_stencil = true;
            break;

        default:
            // Handle unexpected attachments (GL_DEPTH_STENCIL, etc.)
            break;
        }
    }
    Py_RETURN_NONE;
}

PyCaravanGL_API Framebuffer_check_status(PyCaravanFramebuffer *self,
                                         [[maybe_unused]] PyObject *args) {
    if (self->fbo.color_attachments_count == 0 && !self->fbo.has_depth) {
        PyErr_SetString(PyExc_RuntimeError, "Framebuffer has no attachments!");
        return nullptr;
    }

    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_bind_fbo_combined(cv_state, OpenGL, self->fbo.id);
        GLenum status = OpenGL->CheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            PyErr_Format(PyExc_RuntimeError, "Framebuffer not complete! GL Status Code: 0x%X",
                         status);
            return nullptr;
        }
    }
    Py_RETURN_TRUE;
}

PyCaravanGL_API Framebuffer_bind(PyCaravanFramebuffer *self, [[maybe_unused]] PyObject *args) {
    [[maybe_unused]] PyObject *mod = PyType_GetModule(Py_TYPE(self));
    WithActiveGL(OpenGL, cv_state, nullptr) {
        cv_bind_fbo_combined(cv_state, OpenGL, self->fbo.id);
    }
    Py_RETURN_NONE;
}

static const PyMethodDef Framebuffer_methods[] = {
    {"attach_texture", (PyCFunction)(void (*)(void))Framebuffer_attach_texture,
     METH_FASTCALL | METH_KEYWORDS, "Attach a texture to the Framebuffer."},
    {"check_status", (PyCFunction)Framebuffer_check_status, METH_NOARGS,
     "Verify FBO completeness."},
    {"bind", (PyCFunction)Framebuffer_bind, METH_NOARGS, "Bind as the active Framebuffer."},
    {nullptr}};

static const PyType_Slot Framebuffer_slots[] = {{Py_tp_new, (void *)PyType_GenericNew},
                                                {Py_tp_init, Framebuffer_init},
                                                {Py_tp_dealloc, Framebuffer_dealloc},
                                                {Py_tp_methods, (PyMethodDef *)Framebuffer_methods},
                                                {0, nullptr}};

const PyType_Spec Framebuffer_spec = {
    .name = "caravangl.Framebuffer",
    .basicsize = sizeof(PyCaravanFramebuffer),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = (PyType_Slot *)Framebuffer_slots,
};