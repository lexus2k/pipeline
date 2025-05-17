#!/bin/sh

cd build/unittests
valgrind --tool=memcheck --leak-check=full --show-reachable=yes ./test_pipeline

valgrind --tool=massif --heap=yes --stacks=yes ./test_pipeline
