# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -I../../include

# Target executable
TARGET = file_processing_example

# Source files
SRCS = file_processing_example.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Library paths and libraries
LDFLAGS = -L../../build -lpipeline

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
    rm -f $(OBJS) $(TARGET)

.PHONY: all clean