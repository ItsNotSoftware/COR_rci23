CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c99 -g

SRC_DIR=src

OBJ_DIR=obj

# Automatically list all the C source files
SRCS=$(wildcard $(SRC_DIR)/*.c)

# Automatically generate a list of object files
OBJS=$(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Define the final binary name
BIN=COR

# Default target
all: $(BIN)

# Rule to link the program
$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to compile the object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean the build
clean:
	rm -rf $(OBJ_DIR) $(BIN)
