# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Werror -std=c99 -Iinclude -fPIC `sdl2-config --cflags` -g

# Linker flags
LDFLAGS = `sdl2-config --libs`

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib

# Files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
SHARED_LIB = $(LIB_DIR)/libhappines.so

# Targets
TARGET = $(BIN_DIR)/happines

# Phony targets
.PHONY: all clean run shared

# Default target
all: $(TARGET)

# Main target
$(TARGET): $(OBJ_FILES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# Object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

# Run target
run: $(TARGET)
	./$(TARGET)

# nestest.nes target
nestest: $(TARGET)
	./$(TARGET) test/nestest.nes

# Shared library target
shared: $(OBJ_FILES)
	@mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) -shared $^ $(LDFLAGS) -o $(SHARED_LIB)

# Prevent object files from being deleted when using shared target
.SECONDARY:

# Prevent conflicts with files named "shared"
.PHONY: shared

# Add ".so" to the list of file extensions that make considers intermediate
.INTERMEDIATE: $(SHARED_LIB)

# Test target using luajit
test: shared
	luajit test/test.lua test/tests/$(OPCODE).json
