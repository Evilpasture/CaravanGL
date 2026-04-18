cmake_minimum_required(VERSION 3.25)
project(CaravanGL C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# 1. Add the submodule directory
add_subdirectory(extern/fast-capi)
add_subdirectory(extern/mag-mutex)

# 2. Define your main Python module 
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
)

# 3. Link the submodule and Python
target_link_libraries(caravangl PRIVATE 
    Python::Module
    fast_capi_core
    mag_mutex_lib
)

# 4. The Enforcer: Run clang-tidy using the project's .clang-tidy YAML
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

if(CLANG_TIDY_EXE)
    message(STATUS "Clang-Tidy Enforcer: ON (Source: .clang-tidy file)")
    
    # We remove --checks and --warnings-as-errors from here.
    # Clang-tidy will now automatically find your .clang-tidy file in the project root.
    set(CLANG_TIDY_COMMAND
        "${CLANG_TIDY_EXE}"
        "--extra-arg=-std=c23"  # Still necessary to force C23 parsing
        "--header-filter=include/.*"
    )

    # Set the property on the target
    set_target_properties(caravangl PROPERTIES
        C_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
    
    # Optional: Fail the build if the config file is missing
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")
        message(FATAL_ERROR "CaravanGL: .clang-tidy config missing! The enforcer has nothing to enforce.")
    endif()
else()
    message(WARNING "Clang-Tidy not found! Enforcer is bypassed.")
endif()

target_include_directories(caravangl PRIVATE include)

if(APPLE)
    # Required for Python modules on macOS
    target_link_options(caravangl PRIVATE "-undefined" "dynamic_lookup")
endif()

set_target_properties(caravangl PROPERTIES PREFIX "")
install(TARGETS caravangl DESTINATION .)