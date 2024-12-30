# CORE VARIABLES
CC := gcc
RM := rm -rf
MKDIR := mkdir -p

# DIRECTORIES
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
LIB_DIR := lib
ROM_DIR := roms
INCLUDE_DIR := include

# FILES
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET := gb-emu

# COMPILER FLAGS
INCLUDES := -I $(INCLUDE_DIR)
CFLAGS := -Wall -Wextra
LDFLAGS := -lSDL2

# BUILD CONFIGS
DEBUG_FLAGS := -g -DDEBUG
RELEASE_FLAGS := -O2
SANITIZE_FLAGS := -fsanitize=address -fsanitize=undefined

# OS SPECIFIC CONFIG
ifeq ($(OS),Windows_NT)
    RM := del
    TARGET := $(TARGET).exe
    LDFLAGS := -L $(LIB_DIR) -lmingw32 -lSDL2main -lSDL2
endif

# BUILD TARGETS
.PHONY: all clean debug release sanitize dirs

all: release

debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(BIN_DIR)/$(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: dirs $(BIN_DIR)/$(TARGET)

sanitize: CFLAGS += $(DEBUG_FLAGS) $(SANITIZE_FLAGS)
sanitize: LDFLAGS += $(SANITIZE_FLAGS)
sanitize: dirs $(BIN_DIR)/$(TARGET)

# CREATE DIRECTORIES
dirs:
	@$(MKDIR) $(BUILD_DIR) $(BIN_DIR) $(ROM_DIR)

# COMPILE OBJECTS
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# LINK EXECUTABLE
$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# CLEANUP
clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR) || true
