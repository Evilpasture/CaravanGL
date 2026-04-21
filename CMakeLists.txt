cmake_minimum_required(VERSION 3.25)
project(CaravanGL C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Option for Python (Default ON for standalone builds)
option(CARAVANGL_BUILD_PYTHON "Build Python bindings for CaravanGL" ON)

# 2. Dependency Macro (Fixed to check local folder first)
macro(caravan_add_dependency TARGET_NAME SUBDIR)
    if(NOT TARGET ${TARGET_NAME})
        set(LOCAL_DEP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}")
        if(EXISTS "${LOCAL_DEP_PATH}/CMakeLists.txt")
            message(STATUS "CaravanGL: Building internal dependency ${TARGET_NAME} from ${SUBDIR}")
            add_subdirectory("${LOCAL_DEP_PATH}")
        else()
             message(FATAL_ERROR "CaravanGL: Dependency '${TARGET_NAME}' missing. Did you git submodule update?")
        endif()
    else()
        message(STATUS "CaravanGL: Using parent-provided dependency ${TARGET_NAME}")
    endif()
endmacro()

# -----------------------------------------------------------------------------
# 3. Handle Static Core (Always Built)
# -----------------------------------------------------------------------------
caravan_add_dependency(mag_mutex_lib "extern/mag-mutex")

add_library(caravangl_core STATIC src/core/caravangl_core.c)
target_include_directories(caravangl_core PUBLIC include)
target_link_libraries(caravangl_core PUBLIC mag_mutex_lib)

# -----------------------------------------------------------------------------
# 4. Handle Python Bindings (Conditional)
# -----------------------------------------------------------------------------
if(CARAVANGL_BUILD_PYTHON)
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
    caravan_add_dependency(fast_capi_core "extern/fast-capi")

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
        # Note: We link to the core logic instead of compiling twice
    )

    target_link_libraries(caravangl PRIVATE 
        caravangl_core
        Python::Module
        fast_capi_core
    )

    target_include_directories(caravangl PRIVATE src/caravangl)
    target_compile_definitions(caravangl PRIVATE CARAVANGL_PYTHON_BINDINGS)

    if(APPLE)
        target_link_options(caravangl PRIVATE "-undefined" "dynamic_lookup")
    endif()

    set_target_properties(caravangl PROPERTIES PREFIX "")
    install(TARGETS caravangl DESTINATION .)
endif()

# -----------------------------------------------------------------------------
# 5. The Enforcer (Clang-Tidy) - Applied to the Static Core
# -----------------------------------------------------------------------------
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CLANG_TIDY_COMMAND
        "${CLANG_TIDY_EXE}"
        "--extra-arg=-std=c23" 
        "--header-filter=include/.*"
    )
    # Apply to the core C logic
    set_target_properties(caravangl_core PROPERTIES C_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()