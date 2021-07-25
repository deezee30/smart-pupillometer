cmake_minimum_required(VERSION 3.9.4)
project(pupil_detector VERSION 0.1.0) # project name and version

# Constants
set(CPACK_PROJECT_NAME ${PROJECT_NAME}) # project name
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION}) # project version
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Libraries
find_package(OpenCV REQUIRED) # OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Set up output
add_executable(pupil_detector main.cpp) # declare executable

target_link_libraries(pupil_detector ${OpenCV_LIBS}) # link OpenCV

include(CPack)

# add binary tree directory to the list of paths to search for include files
target_include_directories(pupil_detector PUBLIC "${PROJECT_BINARY_DIR}")
target_compile_features(pupil_detector PRIVATE cxx_std_17)