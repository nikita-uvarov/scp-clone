message(STATUS ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(opengl-test)
cmake_minimum_required(VERSION 2.6)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

include_directories(SYSTEM ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-default -Wswitch-enum -Wuninitialized -Winit-self -Wfloat-equal -Wwrite-strings -Wcast-qual -Wconversion -Waddress -Wlogical-op -Wno-sign-compare -Wzero-as-null-pointer-constant")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_GLEXT_PROTOTYPES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")

set(opengl-test-sources
	src/main.cpp
	src/MainWindow.cpp
	src/GameController.cpp
	src/GLUtils.cpp)

set(opengl-test-headers
    src/MainWindow.h
    src/IGameController.h
    src/GameController.h
    src/GLUtils.h
    src/SDLUtils.h)

add_executable(opengl-test ${opengl-test-sources})

target_link_libraries(opengl-test ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARY} ${OPENGL_LIBRARY})
