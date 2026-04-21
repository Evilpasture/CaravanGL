cmake_minimum_required(VERSION 3.25)
project(CaravanGL C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# 1. Dependency Resolution (The "Look Up" Logic)
# -----------------------------------------------------------------------------

# Helper function to add externals only if they haven't been added by a parent
macro(caravan_add_dependency TARGET_NAME SUBDIR)
    if(NOT TARGET ${TARGET_NAME})
        message(STATUS "CaravanGL: Target '${TARGET_NAME}' not found. Looking in ${SUBDIR}...")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
            add_subdirectory(${SUBDIR})
        else()
            message(FATAL_ERROR "CaravanGL: Dependency '${TARGET_NAME}' missing at ${SUBDIR}. "
                                "Did you forget 'git submodule update --init'?")
        endif()
    else()
        message(STATUS "CaravanGL: Using existing target '${TARGET_NAME}' provided by parent project.")
    endif()
endmacro()

# Resolve dependencies
caravan_add_dependency(fast_capi_core "extern/fast-capi")
caravan_add_dependency(mag_mutex_lib  "extern/mag-mutex")

# -----------------------------------------------------------------------------
# 2. Python Configuration
# -----------------------------------------------------------------------------
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Define your main Python module 
python_add_library(caravangl MODULE 
    src/caravangl/caravangl.c
    src/caravangl/caravangl_arg_indices.c
    src/caravangl/caravangl_pipeline.c
    src/caravangl/caravangl_buffer.c
    src/caravangl/caravangl_uniformbatch.c
    src/caravangl/caravangl_program.c
    src/caravangl/caravangl_vao.c
    src/caravangl/caravangl_texture.c
    src/caravangl/caravangl_framebuffer.c
    src/caravangl/caravangl_sampler.c
    src/caravangl/caravangl_context.c
    src/caravangl/caravangl_sync.c
    src/caravangl/caravangl_query.c
    src/caravangl/caravangl_compute.c
    # The PURE C Core logic
    src/core/caravan_core.c
)

# Link everything
target_link_libraries(caravangl PRIVATE 
    Python::Module
    fast_capi_core
    mag_mutex_lib
)

target_include_directories(caravangl PRIVATE include)
target_compile_definitions(caravangl PRIVATE CARAVANGL_PYTHON_BINDINGS)

# -----------------------------------------------------------------------------
# 3. The Enforcer (Clang-Tidy)
# -----------------------------------------------------------------------------
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CLANG_TIDY_COMMAND
        "${CLANG_TIDY_EXE}"
        "--extra-arg=-std=c23" 
        "--header-filter=include/.*"
    )
    set_target_properties(caravangl PROPERTIES C_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# -----------------------------------------------------------------------------
# 4. OS-Specific Logic & Installation
# -----------------------------------------------------------------------------
if(APPLE)
    target_link_options(caravangl PRIVATE "-undefined" "dynamic_lookup")
endif()

set_target_properties(caravangl PROPERTIES PREFIX "")
install(TARGETS caravangl DESTINATION .)