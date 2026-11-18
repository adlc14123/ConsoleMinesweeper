# ============================================================================
# Makefile for ConsoleMinesweeper.cpp
# Standard: C++17 (Strict Pedantic Mode)
# Compilers: GCC / Clang
# Platforms: Linux, macOS, FreeBSD, Windows (MinGW/MSYS2)
# ============================================================================

# Compiler definition (default: g++, override with CXX=clang++ make)
CXX ?= g++

# Target output binary name
TARGET = ConsoleMinesweeper

# Source files
SRCS = ConsoleMinesweeper.cpp
OBJS = $(SRCS:.cpp=.o)

# Base C++ Standard specification
CXXFLAGS += -std=c++17

# Strict Warning and Pedantic Flags (Treat warnings as errors)
STRICT_FLAGS = -Wall \
               -Wextra \
               -Wpedantic \
               -pedantic-errors \
               -Werror \
               -Wshadow \
               -Wnon-virtual-dtor \
               -Wold-style-cast \
               -Wcast-align \
               -Wunused \
               -Woverloaded-virtual \
               -Wsign-conversion \
               -Wconversion \
               -Wnull-dereference \
               -Wdouble-promotion \
               -Wformat=2

CXXFLAGS += $(STRICT_FLAGS)

# Targets declaration
.PHONY: all release debug sanitize clean run help

# Default target
all: release

# Release build mode (Optimized -O3)
release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Debug build mode (Symbolic debug info -g3, no optimization -O0)
debug: CXXFLAGS += -g3 -O0 -DDEBUG
debug: $(TARGET)

# Sanitize build mode (AddressSanitizer & UndefinedBehaviorSanitizer)
sanitize: CXXFLAGS += -g3 -O0 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: LDFLAGS += -fsanitize=address,undefined
sanitize: $(TARGET)

# Linking rule
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(TARGET)

# Compilation rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run target
run: $(TARGET)
	./$(TARGET)

# Clean built files
clean:
	@rm -f $(OBJS) $(TARGET) $(TARGET).exe

# Help menu
help:
	@echo "Usage commands:"
	@echo "  make               - Build release mode (GCC default)"
	@echo "  make CXX=clang++   - Build release mode using Clang"
	@echo "  make debug         - Build debug mode with symbol tables"
	@echo "  make sanitize      - Build with Address/UB Sanitizers"
	@echo "  make run           - Build and execute the game"
	@echo "  make clean         - Remove build artifacts"