# Set project directory and build directory
PROJECT_DIR := $(shell pwd)
BUILD_DIR := $(PROJECT_DIR)/build

# Compiler and flags
CC := gcc
CFLAGS := -g

# Source files and their corresponding object files
CLIENT_SRC := client/client.c
SERVER_SRC := server/server.c
CLIENT_OBJ := $(BUILD_DIR)/client.o
SERVER_OBJ := $(BUILD_DIR)/server.o
CLIENT_BIN := $(BUILD_DIR)/client
SERVER_BIN := $(BUILD_DIR)/server

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule for building the client executable
$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $(CLIENT_BIN)

# Rule for building the server executable
$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $(SERVER_BIN)

# Rule for compiling the client source file
$(CLIENT_OBJ): $(CLIENT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

# Rule for compiling the server source file
$(SERVER_OBJ): $(SERVER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

# Rule for cleaning the build directory
clean:
	rm -rf $(BUILD_DIR)

# Default target to build both client and server
all: $(CLIENT_BIN) $(SERVER_BIN)

