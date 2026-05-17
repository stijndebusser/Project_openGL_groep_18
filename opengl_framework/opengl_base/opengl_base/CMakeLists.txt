cmake_minimum_required(VERSION 3.24)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(opengl_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_directories(${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(OpenGL REQUIRED)
find_package(GLM REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_library(GLAD glad.c  "bezier.cpp"  "FrameBuffer.h")

include_directories(${CMAKE_SOURCE_DIR}/includes)
include_directories(${CMAKE_SOURCE_DIR})

add_executable(opengl_project
    main.cpp
    stb_image.cpp
    Model.cpp
  "bezier.cpp"  "FrameBuffer.h")

target_link_libraries(opengl_project PRIVATE
    glfw3
    opengl32
    GLAD
    assimp::assimp
)