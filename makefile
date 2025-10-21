# --------- Directory Variables ---------
SRC_DIR     := src
INCLUDE_DIR := include
OBJ_DIR     := obj
BIN_DIR     := bin

# --------- Compiler Settings -----------
CC      := gcc
CFLAGS  := -Wall -Wextra -I$(INCLUDE_DIR)
LDFLAGS := -lreadline -lhistory
TARGET  := $(BIN_DIR)/myshell

SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/shell.c $(SRC_DIR)/execute.c $(SRC_DIR)/jobs.c
OBJS := $(OBJ_DIR)/main.o $(OBJ_DIR)/shell.o $(OBJ_DIR)/execute.o $(OBJ_DIR)/jobs.o

all: $(TARGET)
	@echo "✅ Build complete! Run the shell using: ./bin/myshell"

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "🔗 Linking object files..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "🚀 Executable created at $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "🛠️  Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@echo "🧹 Cleaning object files and binaries..."
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
	@echo "✨ Clean complete."

run: all
	@echo "🚀 Running myshell..."
	./$(TARGET)

.PHONY: all clean run
