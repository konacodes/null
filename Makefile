# null programming language compiler
# Uses LLVM for code generation

CC := clang
CFLAGS := -Wall -Wextra -g -O2 $(shell llvm-config --cflags)
LDFLAGS := $(shell llvm-config --ldflags --libs core native --system-libs)

SRC_DIR := src
BUILD_DIR := build
BIN := null

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean test test-compiler test-lang

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN)

test: test-compiler test-lang

test-compiler:
	@echo "Running compiler tests..."
	@for t in tests/compiler/*.c; do \
		[ -f "$$t" ] && $(CC) $(CFLAGS) "$$t" -o /tmp/test_runner $(LDFLAGS) && /tmp/test_runner || true; \
	done

test-lang:
	@echo "Running language tests..."
	@for t in tests/lang/*.null; do \
		[ -f "$$t" ] && echo "Testing $$t..." && ./$(BIN) "$$t" || true; \
	done

# Dependencies
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/analyzer.h $(SRC_DIR)/codegen.h
$(BUILD_DIR)/lexer.o: $(SRC_DIR)/lexer.c $(SRC_DIR)/lexer.h
$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/lexer.h
$(BUILD_DIR)/analyzer.o: $(SRC_DIR)/analyzer.c $(SRC_DIR)/analyzer.h $(SRC_DIR)/parser.h
$(BUILD_DIR)/codegen.o: $(SRC_DIR)/codegen.c $(SRC_DIR)/codegen.h $(SRC_DIR)/parser.h
