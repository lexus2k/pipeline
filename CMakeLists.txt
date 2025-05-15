cmake_minimum_required(VERSION 3.10)

set(CMAKE_BUILD_TYPE Debug)

project(pipeline)

option(BUILD_TESTS "Build the tests" ON)
option(BUILD_EXAMPLES "Build the examples" OFF)
option(BUILD_WITH_RTTI "Build with RTTI" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the pipeline library
add_library(pipeline
    src/pipeline.cpp
    src/pipeline_pad.cpp
    src/pipeline_pads.cpp
    src/pipeline_node.cpp
    src/pipeline_nodes.cpp
    src/pipeline_sharedmem_node.cpp
)

if (BUILD_WITH_RTTI)
    target_compile_options(pipeline PRIVATE -frtti)
else()
    target_compile_options(pipeline PRIVATE -fno-rtti)
endif()

# Include directories
target_include_directories(pipeline PUBLIC include)

target_include_directories(pipeline PRIVATE src)

# Install the library
install(TARGETS pipeline
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install public headers
install(DIRECTORY include/ DESTINATION include)

if (BUILD_TESTS)
    # Add the unittests directory
    add_subdirectory(unittests)
endif()

if (BUILD_EXAMPLES)
    # Add the examples directory
    add_subdirectory(examples)
endif()
