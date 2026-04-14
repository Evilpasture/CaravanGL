cmake_minimum_required(VERSION 3.25)
project(CaravanGL C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python REQUIRED COMPONENTS Development.Module)

# 1. Add the submodule directory
add_subdirectory(extern/fast-capi)

# 2. Define your main Python module 
# ADDED: caravangl_arg_indices.c must be here!
add_library(caravangl MODULE 
    src/caravangl/caravangl.c
    src/caravangl/caravangl_arg_indices.c
)

# 3. Link the submodule and Python
target_link_libraries(caravangl PRIVATE 
    Python::Module
    fast_capi_core
)

target_include_directories(caravangl PRIVATE include)

if(APPLE)
    # Required for Python modules on macOS
    target_link_options(caravangl PRIVATE "-undefined" "dynamic_lookup")
endif()

set_target_properties(caravangl PROPERTIES PREFIX "")

install(TARGETS caravangl DESTINATION .)