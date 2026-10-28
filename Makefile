# Simple Makefile for systems without CMake.  Usage: make && ./build/metro_navigator
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
INCLUDES := -Iinclude
BUILD    := build

CORE_SRC := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
CORE_OBJ := $(patsubst src/%.cpp,$(BUILD)/obj/%.o,$(CORE_SRC))

.PHONY: all run test clean

all: $(BUILD)/metro_navigator $(BUILD)/metro_tests

$(BUILD)/obj/%.o: src/%.cpp $(wildcard include/metro/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/metro_navigator: $(CORE_OBJ) $(BUILD)/obj/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD)/metro_tests: $(CORE_OBJ) tests/test_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE_OBJ) tests/test_main.cpp -o $@

run: $(BUILD)/metro_navigator
	./$(BUILD)/metro_navigator

test: $(BUILD)/metro_tests
	./$(BUILD)/metro_tests

clean:
	rm -rf $(BUILD)
