# ============================================================================
# Makefile for Microsoft NMAKE (x64 Native Tools Command Prompt for VS)
# Target: ConsoleMinesweeper.exe
# Standard: C++17 Strict Pedantic Mode (/W4 /WX /permissive-)
# ============================================================================

# Compiler definition
CXX = cl

# Target executable name
TARGET = ConsoleMinesweeper.exe

# Source files
SRCS = ConsoleMinesweeper.cpp
OBJS = ConsoleMinesweeper.obj

# Strict MSVC C++17 Compiler Flags:
#  /nologo        - Suppress startup banner
#  /EHsc          - Standard C++ exception handling model
#  /std:c++17     - Enable C++17 standard features
#  /W4            - Output level 4 (strict) warnings
#  /WX            - Treat all warnings as errors (Pedantic requirement)
#  /permissive-   - Strict standard conformance mode
#  /Zc:__cplusplus- Correctly report updated __cplusplus macro
#  /utf-8         - Set source and execution character sets to UTF-8
COMMON_FLAGS = /nologo /EHsc /std:c++17 /W4 /WX /permissive- /Zc:__cplusplus /utf-8

# Release Build Flags (Optimization)
RELEASE_FLAGS = /O2 /DNDEBUG /MD

# Debug Build Flags (Symbols, no optimization)
DEBUG_FLAGS = /Zi /Od /DDEBUG /MDd

# Default target
all: release

# Release build target
release:
	@echo Building Release x64 target...
	$(CXX) $(COMMON_FLAGS) $(RELEASE_FLAGS) $(SRCS) /Fe:$(TARGET)

# Debug build target with PDB debug symbols
debug:
	@echo Building Debug x64 target...
	$(CXX) $(COMMON_FLAGS) $(DEBUG_FLAGS) $(SRCS) /Fe:$(TARGET) /link /DEBUG

# Build and execute target
run: release
	.\$(TARGET)

# Clean build artifacts
clean:
	@if exist *.obj del /Q /F *.obj
	@if exist *.pdb del /Q /F *.pdb
	@if exist *.ilk del /Q /F *.ilk
	@if exist $(TARGET) del /Q /F $(TARGET)
	@echo Cleaned build artifacts.

# Help message
help:
	@echo Microsoft NMAKE usage commands:
	@echo   nmake              - Build release executable (ConsoleMinesweeper.exe)
	@echo   nmake release      - Build optimized release target
	@echo   nmake debug        - Build debug target with PDB symbols
	@echo   nmake run          - Build and execute game
	@echo   nmake clean        - Delete obj, pdb, ilk, and exe files