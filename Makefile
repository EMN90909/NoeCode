BUILD_DIR ?= build

.PHONY: all build test doctor install clean stage2-proof
all: build

build:
	BUILD_DIR=$(BUILD_DIR) ./scripts/build.sh

test: build
	NQ=./$(BUILD_DIR)/noqeri ./scripts/test_core.sh

doctor: build
	./$(BUILD_DIR)/noqeri doctor

install: build
	BUILD_DIR=$(BUILD_DIR) ./scripts/install.sh

stage2-proof:
	node scripts/bootstrap-stage2.mjs

clean:
	rm -rf $(BUILD_DIR)
