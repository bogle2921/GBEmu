INCLUDES := -I ./include
FLAGS := -g
BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src
LIB_DIR := lib
ROM_DIR := roms
SDL_LIBS := -lSDL2
CC = gcc
RM := rm -rf
EXE :=

# OS SPECIFIC FLAGS BECAUSE <REDACTED> LOVES WINDOWS
ifeq ($(OS),Windows_NT)
    RM = del
    EXE = .exe
    SDL_LIBS = -L $(LIB_DIR) -lmingw32 -lSDL2main -lSDL2
endif

# ENSURE REQUIRED DIRECTORIES EXIST
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(ROM_DIR):
	@mkdir -p $(ROM_DIR)

# DEFAULT TARGET
.PHONY: all
all: $(BUILD_DIR) $(BIN_DIR) $(BIN_DIR)/gb-emu$(EXE)

# OBJECT FILES
$(BUILD_DIR)/cart.o: $(SRC_DIR)/cart.c | $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

# MAKE EXECUTABLE
$(BIN_DIR)/gb-emu$(EXE): $(BUILD_DIR)/cart.o $(SRC_DIR)/main.c | $(BIN_DIR)
	$(CC) $(FLAGS) $(INCLUDES) $^ $(SDL_LIBS) -o $@
	
# CLEAN UP
.PHONY: clean
clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR) || true
