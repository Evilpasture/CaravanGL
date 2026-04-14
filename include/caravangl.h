#pragma once

/*
 * CaravanGL - Modern C23 OpenGL Engine for Python Subinterpreters
 * 
 * This umbrella header includes the engine components in the strict 
 * dependency order required to avoid circular definitions.
 */

#include "caravangl_core.h"       // 1. Basic GL Types, Constants, and X-Macros
#include "caravangl_specs.h"      // 2. Engine Structs (Buffers, Context, Textures)
#include "caravangl_module.h"     // 3. Python State (CaravanState containing Function Pointers)
#include "caravangl_loader.h"     // 4. Loader implementation (load_gl)
#include "caravangl_uniforms.h"   // 5. Uniform dispatch wrappers