cmake_minimum_required(VERSION 3.10)

# Project name
project(text_file_processor_example)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(file_processing_example file_processing_example.cpp)

# Include the pipeline library headers
target_include_directories(file_processing_example PRIVATE ../../include)

# Link the pipeline library
target_link_libraries(file_processing_example PRIVATE pipeline)

# Set the output directory for the executable
set_target_properties(file_processing_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/text_file_processor
)
