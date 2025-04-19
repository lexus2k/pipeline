# Makefile

CXX = g++
CXXFLAGS = -std=c++20 -I./include -I./src -I./unittests/googletest/include
LDFLAGS = -L./unittests/googletest/lib -lgtest -lgtest_main -pthread

BUILD_TESTS ?= n
BUILD_EXAMPLES ?= n

SRC = src/pipeline.cpp \
    src/pipeline_pad.cpp \
    src/pipeline_pads.cpp \
    src/pipeline_node.cpp \
    src/pipeline_nodes.cpp

SRC_TESTS = unittests/test_basic.cpp \
    unittests/test_template_nodes.cpp

OBJ = $(SRC:.cpp=.o)
OBJ_TESTS = $(SRC_TESTS:.cpp=.o)
TARGET = libpipeline.a
TESTS = unittests/test_pipeline

# Installation directories
DESTDIR ?= /usr/local
INCLUDEDIR = $(DESTDIR)/include
LIBDIR = $(DESTDIR)/lib

.PHONY: all clean test install

all: $(TARGET)

# Build the library
$(TARGET): $(OBJ)
	ar rcs $@ $^

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build tests
test: $(TARGET) unittests/test_pipeline
	./unittests/test_pipeline

$(OBJ_TESTS): $(SRC_TESTS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

unittests/test_pipeline: $(OBJ_TESTS) $(TARGET)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) -L. -lpipeline

ifeq ($(BUILD_TESTS), y)
all: unittests/test_pipeline

clean:
	rm -f unittests/test_pipeline
endif

ifeq ($(BUILD_EXAMPLES), y)
all: examples

examples: examples/file_processing_example

examples/file_processing_example: examples/text_file_processor/file_processing_example.cpp $(TARGET)
	$(CXX) $(CXXFLAGS) $< -o $@ -lpipeline

clean:
	rm -f examples/file_processing_example
endif

install: $(TARGET)
	# Create target directories
	mkdir -p $(INCLUDEDIR)
	mkdir -p $(LIBDIR)
	# Copy public headers
	cp -r include/* $(INCLUDEDIR)
	# Copy the library
	cp $(TARGET) $(LIBDIR)

clean:
	rm -f $(OBJ) $(TARGET) unittests/test_pipeline
