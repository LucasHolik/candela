# Makefile for Candela

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -I. -I./src -DUNICODE -D_UNICODE

# Linker flags
LDFLAGS = -mwindows -lgdi32 -lcomctl32 -luser32 -lshell32 -ldxva2

# Resource compiler
RC = windres

# Build directory
BUILD_DIR = build

# Source files
SRCS = src/main.cpp src/tray.cpp src/gui.cpp src/settings.cpp src/brightness.cpp

# Resource file
RC_FILE = candela.rc

# Object files
OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
RC_OBJ = $(BUILD_DIR)/candela.res

# Executable name
TARGET = candela.exe

# Target executable path
TARGET_PATH = $(BUILD_DIR)/$(TARGET)

.PHONY: all clean

all: $(TARGET_PATH)

$(TARGET_PATH): $(OBJS) $(RC_OBJ)
	$(CXX) $(OBJS) $(RC_OBJ) -o $(TARGET_PATH) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.cpp
	@cmd /c if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RC_OBJ): $(RC_FILE)
	@cmd /c if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(RC) $(RC_FILE) -O coff -o $(RC_OBJ)

clean:
	cmd /c if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)