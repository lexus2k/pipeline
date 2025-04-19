# CMakeLists.txt for Unit Tests

set(CMAKE_BUILD_TYPE Debug)

project(unittests)

set(CMAKE_CXX_STANDARD 20)

# Specify the minimum version of CMake
cmake_minimum_required(VERSION 3.10)

# Find the Google Test package
find_package(GTest REQUIRED)

# Include the directories for the pipeline library and Google Test
include_directories(${GTEST_INCLUDE_DIRS} ../src ../include)

# Create the test executable
add_executable(test_pipeline
    test_basic.cpp
    test_template_nodes.cpp
    main.cpp)

# Link the Google Test libraries and the pipeline library
target_link_libraries(test_pipeline ${GTEST_LIBRARIES} pthread pipeline)

# Enable testing
enable_testing()

# Add the test to CMake's testing framework
add_test(NAME test_pipeline COMMAND test_pipeline)