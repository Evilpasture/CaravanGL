// --- extern/CaravanGL/include/caravangl.h ---
#pragma once

/**
 * CaravanGL Master Header
 * Standard inclusion order for the Pure C Graphics Core.
 */
// clang-format off
#include "caravangl_core.h"      // 1. GL Types, Constants, and Function Table
#include "caravangl_specs.h"     // 2. Engine Structs (Context, Buffers, Handles)
#include "caravangl_context.h"   // 3. Lifecycle & TLS Management
#include "caravangl_state.h"     // 4. Shadow State Logic & Locking Macros
#include "caravangl_uniforms.h"  // 5. Uniform Dispatch Wrappers
#include "caravangl_loader.h"    // 6. The Loading Logic
// clang-format on