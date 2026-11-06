# Main Makefile for SFERAv2-WSCP-DUT
# Compiles C files and includes CBOR library from submodules

# Directories
SRC_DIR := src
BUILD_DIR := build
SUBMODULES_DIR := submodules
CBOR_DIR := $(SUBMODULES_DIR)/cbor

# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -I$(CBOR_DIR)/include
LDFLAGS := 

# Find all C source files
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Include CBOR makefile
include $(CBOR_DIR)/cbor.mk

# Output executable
TARGET := $(BUILD_DIR)/main

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link executable
$(TARGET): $(C_OBJECTS) $(CBOR_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean