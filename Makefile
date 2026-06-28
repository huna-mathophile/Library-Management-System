# Makefile for Library Management System
# Auto-detects OS and uses the right flags.

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2
TARGET  = library_system
SRC     = main.c

# Auto-detect OS
UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(OS),Windows_NT)
    # Windows (MinGW / MSYS2)
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm
    TARGET := $(TARGET).exe
    RM = del /Q
else ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    RM = rm -f
else ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    RM = rm -f
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) temp.txt studTemp.txt

.PHONY: all run clean
