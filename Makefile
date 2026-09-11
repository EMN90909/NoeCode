BUILD_DIR ?= build
BUILD_TYPE ?= Release

.PHONY: all build test doctor clean install
all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE)

test: build
	ctest --test-dir $(BUILD_DIR) -C $(BUILD_TYPE) --output-on-failure
	./scripts/test_core.sh

doctor: build
	./$(BUILD_DIR)/noqeri doctor .

install: build
	cmake --install $(BUILD_DIR) --config $(BUILD_TYPE)

clean:
	cmake -E remove_directory $(BUILD_DIR)
