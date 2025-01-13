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
INCLUDE_DIR := include
LOG_DIR := logs

# TARGET
TARGET := gb-emu

# SOURCES/OBJECTS
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# BASE FLAGS
BASE_CFLAGS := -Wall -Wextra -std=c11
INCLUDES := -I $(INCLUDE_DIR)
LDFLAGS := -lSDL2

# PLATFORM CONFIG
ifeq ($(OS),Windows_NT)
    RM := del
    TARGET := $(TARGET).exe
    LDFLAGS := -L lib -lmingw32 -lSDL2main -lSDL2
else
    BASE_CFLAGS += -D_POSIX_C_SOURCE=199309L
endif

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
sanitize: LDFLAGS += $(SANITIZE_FLAGS)
sanitize: dirs $(BIN_DIR)/$(TARGET)

dirs:
	@$(MKDIR) $(BUILD_DIR) $(BIN_DIR) $(ROM_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR) $(LOG_DIR) 2>/dev/null || true
