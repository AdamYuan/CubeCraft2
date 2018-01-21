cmake_minimum_required(VERSION 3.7)
project(CubeCraft2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=core-avx2 -ldl")

add_definitions(-DSQLITE_THREADSAFE=0)

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	link_libraries(${OPENGL_gl_LIBRARY})
endif()

find_package(glm 0.9.8 REQUIRED)

find_package(GLEW 2 REQUIRED)
if (GLEW_FOUND)
	include_directories(${GLEW_INCLUDE_DIR})
	link_libraries(${GLEW_LIBRARY})
endif()

find_package(Threads REQUIRED)
if(THREADS_FOUND)
    link_libraries(${CMAKE_THREAD_LIBS_INIT})
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules" ${CMAKE_MODULE_PATH})

find_package(GLFW3 3.2 REQUIRED)
if (GLFW3_FOUND)
	include_directories(${GLFW3_INCLUDE_PATH})
	link_libraries(${GLFW3_LIBRARY})
endif()

#add source files
file(GLOB SOURCE_FILES main.cpp src/*.cpp dep/*/*.cpp dep/*/*.c)
include_directories(dep)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
