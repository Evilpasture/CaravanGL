// --- START OF FILE include/caravangl_context.h ---
#pragma once
#include "caravangl_specs.h"

// -----------------------------------------------------------------------------
// Pure C API: Context Management
// -----------------------------------------------------------------------------

/**
 * The Global TLS Pointer.
 * Tracks the active context for the current OS thread.
 * Natively uses the C23 `thread_local` keyword.
 */
extern thread_local CaravanHandle *cv_current_handle;

/**
 * Initializes a new CaravanHandle, setting up its Mutex and zeroing state.
 */
void caravan_init_handle(CaravanHandle *h);

/**
 * Queries hardware limits (Texture sizes, UBOs, Computes) and populates caps.
 * Assumes the function table (h->gl) is already loaded.
 */
void caravan_query_caps(CaravanHandle *h);

/**
 * Flushes all pending OpenGL object deletions to the driver.
 * IMPORTANT: Must be called while h->ctx.state_lock is HELD.
 */
void caravan_flush_garbage(CaravanHandle *h);

/**
 * Activates the handle on the current thread and flushes pending garbage.
 * IMPORTANT: Must be called while h->ctx.state_lock is HELD at call site.
 */
void caravan_make_current(CaravanHandle *h);

/**
 * Thread-safely enqueues a GL object ID for deferred deletion.
 * @param count Current number of elements (updated on success)
 * @param capacity Maximum size of the array
 * @param array The storage array
 * @param id The OpenGL resource ID to delete
 */
void cv_enqueue_garbage(size_t *count, size_t capacity, GLuint array[static 1], GLuint id);

void cv_enqueue_garbage_ptr(size_t *count, size_t capacity, GLsync array[static 1], void *ptr);