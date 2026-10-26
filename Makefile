# Makefile for GraphSketch Project

# Compiler
CXX ?= /usr/bin/clang++

# Compiler flags
CXXFLAGS = -std=c++17 -O2 -Wall

# Include directories
SDKROOT := $(shell xcrun --show-sdk-path 2>/dev/null)
ifeq ($(SDKROOT),)
SDKROOT := /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
endif
CPPFLAGS = -isystem $(SDKROOT)/usr/include/c++/v1 -isystem $(SDKROOT)/usr/include
INCLUDES = -Iinclude

# Source files
SRC = src/main.cpp src/GraphSketchTensor.cpp

# Output executable name
TARGET = graphsketch

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -o $(TARGET) $(SRC)

# Clean target
clean:
	rm -f $(TARGET)
