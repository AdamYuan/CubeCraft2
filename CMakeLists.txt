cmake_minimum_required(VERSION 3.7)
project(CubeCraft2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=core-avx2")

#add source files
file(GLOB SOURCE_FILES main.cpp src/*.cpp dep/*/*.cpp)
include_directories(dep)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} stdc++fs)


find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${OPENGL_gl_LIBRARY})
endif()

find_package(glm 0.9.8 REQUIRED)

find_package(GLEW 2 REQUIRED)
if (GLEW_FOUND)
	include_directories(${GLEW_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARY})
endif()

find_package(Threads REQUIRED)
if(THREADS_FOUND)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules" ${CMAKE_MODULE_PATH})

find_package(GLFW3 3.2 REQUIRED)
if (GLFW3_FOUND)
	include_directories(${GLFW3_INCLUDE_PATH})
	target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARY})
endif()

find_package(SQLite3 REQUIRED)
if (SQLITE3_FOUND)
    include_directories(${SQLITE3_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})
endif()

find_package(Config++ REQUIRED)
if (CONFIG++_FOUND)
    include_directories(${CONFIG++_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${CONFIG++_LIBRARY})
endif()
