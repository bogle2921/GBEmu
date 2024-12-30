INCLUDES := -I ./include
FLAGS := -g
BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src
ROM_DIR := roms
CC = gcc
RM := rm -rf
EXE :=

# OS SPECIFIC FLAGS AND LIBS
ifeq ($(OS),Windows_NT)
    RM = del
    EXE = .exe
    PLATFORM_DIR = windows
    SDL_LIBS = -L./lib/$(PLATFORM_DIR) -lmingw32 -lSDL2main -lSDL2 -mwindows -lsetupapi -lwinmm -lversion
else
    UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Darwin)
				PLATFORM_DIR = macos
				SDL_LIBS = -L./lib/$(PLATFORM_DIR) -lSDL2main -lSDL2 \
									-framework AudioToolbox -framework CoreAudio \
									-framework CoreHaptics -framework CoreServices \
									-framework Carbon -framework ForceFeedback \
									-framework GameController -framework IOKit \
									-framework Cocoa -framework OpenGL -framework Metal \
									-framework CoreVideo -framework CoreFoundation \
									-liconv
    else
        PLATFORM_DIR = linux
        SDL_LIBS = -L./lib/$(PLATFORM_DIR) -lSDL2main -lSDL2 -lm -ldl -lpthread -lrt -lX11
    endif
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

$(BUILD_DIR)/bus.o: $(SRC_DIR)/bus.c | $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/instructions.o: $(SRC_DIR)/instructions.c | $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/graphics.o: $(SRC_DIR)/graphics.c | $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

# MAKE EXECUTABLE
$(BIN_DIR)/gb-emu$(EXE): $(BUILD_DIR)/cart.o $(BUILD_DIR)/bus.o $(BUILD_DIR)/instructions.o $(BUILD_DIR)/graphics.o $(SRC_DIR)/main.c | $(BIN_DIR)
	$(CC) $(FLAGS) $(INCLUDES) $^ $(SDL_LIBS) -o $@
	
# CLEAN UP
.PHONY: clean
clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR) || true

# PLATFORM INFO
.PHONY: info
info:
	@echo "Platform directory: $(PLATFORM_DIR)"
	@echo "SDL libs: $(SDL_LIBS)"
