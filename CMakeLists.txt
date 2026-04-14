cmake_minimum_required(VERSION 3.25)
project(CaravanGL C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python REQUIRED COMPONENTS Development.Module)

add_library(caravangl MODULE src/caravangl/caravangl.c)

target_include_directories(caravangl PRIVATE include)
target_link_libraries(caravangl PRIVATE Python::Module)

if(APPLE)
    target_link_options(caravangl PRIVATE "-undefined" "dynamic_lookup")
endif()

# Ensure name is exactly caravangl.so
set_target_properties(caravangl PROPERTIES PREFIX "")

# This is the line that scikit-build-core uses to place the file in the wheel
install(TARGETS caravangl DESTINATION .)