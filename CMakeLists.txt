cmake_minimum_required (VERSION 2.8)

# Fix for eclipse C++11 headers
set (CMAKE_CXX_COMPILER_ARG1 "-std=c++11")

project (RayTracing)

set(CMAKE_BUILD_TYPE "Release")

set (ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (CMAKE_CXX_FLAGS "-std=c++11 -pthread -O3 -w -m64 -fno-omit-frame-pointer -fopenmp")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ROOT_DIR}/CMake/Modules/")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ROOT_DIR}")
message( "output: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

find_package( OpenCL REQUIRED )
file(GLOB RayTracing_SOURCES *.cpp CLOperator/*.cpp Scene/*.cpp)

add_executable( RayTracing ${RayTracing_SOURCES} )
target_link_libraries( RayTracing ${OpenCL_LIBRARIES} )
