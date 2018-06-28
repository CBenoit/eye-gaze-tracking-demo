cmake_minimum_required(VERSION 3.10)
project(eye_gaze_tracking_demo)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(OpenCV REQUIRED)

add_subdirectory(ow/)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization -Wvector-operation-performance -Winvalid-pch \
                            -Wunused -Wconversion -Wlogical-op -Wuninitialized -Wsuggest-final-types -Wdouble-promotion \
                            -Wmissing-include-dirs -Wall -pedantic  -Wcast-qual -Wswitch-default -Wunreachable-code \
                            -Wfloat-equal -Wundef -Wcast-align -Wcast-align -Wredundant-decls -Winit-self -Wshadow \
                            -Wnon-virtual-dtor -Weffc++ -Wswitch-enum -Wmissing-declarations -fmessage-length=150 -W \
                            -Wpointer-arith -Wmain -Wwrite-strings -Wmissing-noreturn")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=leak -fsanitize=undefined -g")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

include_directories(
        SYSTEM ow/include
        SYSTEM ow/external
        SYSTEM eye-tracking-lib/include
        SYSTEM /usr/include/opencv
)

add_executable(
        eye_gaze_tracking_demo
        ${SOURCES}
        eye-tracking-lib/src/eye_finder.cpp
)

target_link_libraries(eye_gaze_tracking_demo ow gui ${OpenCV_LIBS} sfml-system sfml-audio)
