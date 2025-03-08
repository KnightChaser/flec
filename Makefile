# Set the build directory
BUILD_DIR := $(shell pwd)/build

# Compiler and flags
CC := gcc
CFLAGS := -g -lpthread

# Source files and executables
SERVER_SRC := ./server/*.c
CLIENT_SRC := ./client/*.c
SERVER_BIN := $(BUILD_DIR)/server
CLIENT_BIN := $(BUILD_DIR)/client

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule to build the server executable
$(SERVER_BIN): $(SERVER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC)

# Rule to build the client executable
$(CLIENT_BIN): $(CLIENT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)

# Rule for cleaning the build directory
clean:
	rm -rf $(BUILD_DIR)

# Default target to build both client and server
all: $(SERVER_BIN) $(CLIENT_BIN)

