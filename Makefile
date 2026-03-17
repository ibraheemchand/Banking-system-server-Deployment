# ─── Bank Management System – Makefile ───────────────────────────────────────
# Supports:  Windows (MSYS2 / MinGW-w64)
#            Linux / macOS (POSIX sockets, pthreads)
#
# Usage:
#   make          – build the executable
#   make run      – build and start the server
#   make clean    – remove build artefacts
# ─────────────────────────────────────────────────────────────────────────────

CXX      = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2

SRC      = main.cpp Account.cpp Bank.cpp Admin.cpp
HDR      = Account.h Bank.h Admin.h

# ── Platform detection ──────────────────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    TARGET = bms.exe
    # Winsock2 is required on Windows
    LIBS   = -lws2_32
else
    TARGET = bms
    LIBS   =
endif

# ── Targets ──────────────────────────────────────────────────────────────────
.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LIBS)
	@echo ""
	@echo "  Build successful → $(TARGET)"
	@echo "  Run with:  make run    or    ./$(TARGET)"
	@echo ""

run: all
	./$(TARGET)

clean:
	rm -f bms bms.exe
