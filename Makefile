# softkeys -- see project.md.
#
# TARGETS
#   make            the static library, in $(BUILD_DIR)/lib
#   make test       build and run the tests (never built by plain make)
#   make style      the style gate: whitespace, indentation, and the docs
#   make hooks      install the commit-msg hook
#   make clean      the build's own clean; veryclean removes the build trees
#

QMAKE ?= $(shell command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null || echo qmake6)

BUILD_DIR      ?= build
TEST_BUILD_DIR ?= $(BUILD_DIR)-test

LIB      = $(BUILD_DIR)/lib/libsoftkeys.a
TEST_BIN = $(TEST_BUILD_DIR)/softkeys-tests

# A wall-clock ceiling on the test binary, inside the target rather than
# around it, since a hung keyboard test would otherwise wait for ever.
TEST_TIMEOUT ?= 300

SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard include/softkeys/*.h)

.DEFAULT_GOAL := all

all: $(LIB)

$(BUILD_DIR)/Makefile: softkeys.pro softkeys.pri
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(QMAKE) ../softkeys.pro CONFIG+=release

$(LIB): $(BUILD_DIR)/Makefile $(SOURCES) $(HEADERS) FORCE
	$(MAKE) -C $(BUILD_DIR)

$(TEST_BUILD_DIR)/Makefile: test/test.pro softkeys.pri
	mkdir -p $(TEST_BUILD_DIR)
	cd $(TEST_BUILD_DIR) && $(QMAKE) ../test/test.pro

tests-build: $(TEST_BUILD_DIR)/Makefile FORCE
	$(MAKE) -C $(TEST_BUILD_DIR)

# Offscreen, and the binary drops a platform theme itself (test/main.cpp).
# A run over no binary is a failure, not a pass.
test: tests-build
	@[ -x $(TEST_BIN) ] || { echo "test: no test binary at $(TEST_BIN)" >&2; exit 1; }
	QT_QPA_PLATFORM=offscreen timeout $(TEST_TIMEOUT) ./$(TEST_BIN)

style:
	python3 tool/style_gate.py check
	python3 tool/style_gate.py docs

hooks:
	@dir=$$(git rev-parse --git-common-dir 2>/dev/null); \
	if [ -z "$$dir" ]; then echo "hooks: not a git repository" >&2; exit 1; fi; \
	mkdir -p "$$dir/hooks"; \
	install -m 0755 tool/hooks/commit-msg "$$dir/hooks/commit-msg"; \
	echo "hooks: commit-msg installed into $$dir/hooks/"

clean:
	@if [ -f $(BUILD_DIR)/Makefile ]; then $(MAKE) -C $(BUILD_DIR) clean; fi
	@if [ -f $(TEST_BUILD_DIR)/Makefile ]; then $(MAKE) -C $(TEST_BUILD_DIR) clean; fi

# The build trees are this Makefile's own creations, named by variables that
# are checked non-empty and relative before anything is removed.
veryclean: clean
	@for dir in $(BUILD_DIR) $(TEST_BUILD_DIR); do \
		test -n "$$dir" || { echo "veryclean: refusing an empty path" >&2; exit 1; }; \
		case "$$dir" in \
			/*) echo "veryclean: refusing the absolute path $$dir" >&2; exit 1 ;; \
			*..*) echo "veryclean: refusing $$dir -- it escapes the tree" >&2; exit 1 ;; \
			.|./) echo "veryclean: refusing the working directory" >&2; exit 1 ;; \
		esac; \
		if [ -d "$$dir" ]; then echo "veryclean: removing $$dir"; rm -rf "$$dir"; fi; \
	done

help:
	@sed -n '/^# TARGETS/,/^#$$/p' $(firstword $(MAKEFILE_LIST)) | sed 's/^# \{0,1\}//'

.PHONY: all tests-build test style hooks clean veryclean help FORCE
FORCE:
