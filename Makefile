# OFFICIAL MAKEFILE FOR GB-EMU
# BY LINUS TORVALDS... JK BANDIT AND NICK HEYER 

CC := gcc
RM := rm -rf
MKDIR := mkdir -p

# DIRECTORIES
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
ROM_DIR := roms
RM := rm -rf
EXE :=
LOG_DIR := logs
INCLUDE_DIR := include

# BASE FLAGS
BASE_CFLAGS := -Wall -Wextra -std=c11
INCLUDES := -I $(INCLUDE_DIR)
LDFLAGS := -lSDL2


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

# TARGET
TARGET := gb-emu$(EXE)

# SOURCES/OBJECTS
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# MODE FLAGS
DEBUG_FLAGS := -g -DDEBUG
RELEASE_FLAGS := -O2
SANITIZE_FLAGS := -fsanitize=address -fsanitize=undefined

# TARGETS
.PHONY: all clean debug release sanitize dirs

all: clean release

debug: CFLAGS = $(BASE_CFLAGS) $(DEBUG_FLAGS)
debug: dirs $(BIN_DIR)/$(TARGET)

release: CFLAGS = $(BASE_CFLAGS) $(RELEASE_FLAGS)
release: dirs $(BIN_DIR)/$(TARGET)

sanitize: CFLAGS = $(BASE_CFLAGS) $(DEBUG_FLAGS) $(SANITIZE_FLAGS)
sanitize: dirs $(BIN_DIR)/$(TARGET)

dirs:
	@$(MKDIR) $(BUILD_DIR) $(BIN_DIR) $(ROM_DIR) $(LOG_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ $(SDL_LIBS) -o $@

clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR) $(LOG_DIR) 2>/dev/null || true
