CXX ?= c++
NOE_BIN ?= build/noe
NOE_SOURCES := $(wildcard compiler/bootstrap/*.cpp)

.PHONY: all test core release clean

all: $(NOE_BIN)

$(NOE_BIN): $(NOE_SOURCES) include/noe/noe.hpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude/noe $(NOE_SOURCES) -o $(NOE_BIN)

core: $(NOE_BIN)
	NOE_BIN=./$(NOE_BIN) sh scripts/test_core.sh

test: $(NOE_BIN)
	sh scripts/test.sh

release: $(NOE_BIN)
	sh scripts/release_check.sh

clean:
	rm -rf build
