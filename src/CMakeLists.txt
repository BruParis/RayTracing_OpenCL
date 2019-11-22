cmake_minimum_required (VERSION 2.8)

# Fix for eclipse C++11 headers
set (CMAKE_CXX_COMPILER_ARG1 "-std=c++11")

project (RayTracing)

set(CMAKE_BUILD_TYPE "Release")

set (ROOT_DIR .)
set (CMAKE_CXX_FLAGS "-std=c++11 -pthread -ltbb -O3 -w -m64 -fno-omit-frame-pointer -fopenmp")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ROOT_DIR}/CMake/Modules/")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ROOT_DIR}/../build")

find_package( OpenCL REQUIRED )
find_package( Qt5 COMPONENTS Core Widgets REQUIRED )

#################################
# Qt
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${Qt5_INCLUDE_DIRS}
                    ${Qt5Core_INCLUDE_DIRS}
                    ${Qt5Widgets_INCLUDE_DIRS})

set(QT_LIBRARIES ${Qt5_LIBRARIES}
                 ${Qt5Core_LIBRARIES}
                 ${Qt5Widgets_LIBRARIES})

set(FORMS_SRC ImgWindow.ui)
QT5_WRAP_UI(FORMS_HEADERS ${FORMS_SRC})
#################################

file(GLOB RayTracing_SOURCES *.cpp ImgWindow/*.cpp ImgWindow/*.ui CLOperator/*.cpp Scene/*.cpp)
add_executable( RayTracing ${RayTracing_SOURCES} )

target_link_libraries( RayTracing ${OpenCL_LIBRARIES} ${QT_LIBRARIES})
