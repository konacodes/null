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
	@passed=0; failed=0; \
	for t in tests/compiler/*.c; do \
		[ -f "$$t" ] || continue; \
		if $(CC) $(CFLAGS) "$$t" -o /tmp/test_runner $(LDFLAGS) 2>/dev/null && /tmp/test_runner; then \
			passed=$$((passed + 1)); \
		else \
			echo "FAIL: $$t"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "Compiler tests: $$passed passed, $$failed failed"; \
	[ $$failed -eq 0 ]

test-lang:
	@echo "Running language tests..."
	@passed=0; failed=0; \
	for t in tests/lang/*.null; do \
		[ -f "$$t" ] || continue; \
		echo -n "Testing $$t... "; \
		if ./$(BIN) "$$t" >/dev/null 2>&1; then \
			echo "OK"; \
			passed=$$((passed + 1)); \
		else \
			echo "FAIL"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "Language tests: $$passed passed, $$failed failed"; \
	[ $$failed -eq 0 ]

# Dependencies
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/analyzer.h $(SRC_DIR)/codegen.h $(SRC_DIR)/interp.h
$(BUILD_DIR)/lexer.o: $(SRC_DIR)/lexer.c $(SRC_DIR)/lexer.h
$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/lexer.h
$(BUILD_DIR)/analyzer.o: $(SRC_DIR)/analyzer.c $(SRC_DIR)/analyzer.h $(SRC_DIR)/parser.h
$(BUILD_DIR)/codegen.o: $(SRC_DIR)/codegen.c $(SRC_DIR)/codegen.h $(SRC_DIR)/parser.h
$(BUILD_DIR)/interp.o: $(SRC_DIR)/interp.c $(SRC_DIR)/interp.h $(SRC_DIR)/parser.h
