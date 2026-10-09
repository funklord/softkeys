# softkeys -- see project.md.
#
# TARGETS
#   make            the static library, in $(BUILD_DIR)/lib
#   make test       build and run the tests (never built by plain make)
#   make check-mk   build and run them again from softkeys.mk, without qmake
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

# The gate's own suite first, since a gate's pass means something only once
# the gate is known to work; both are copied from ~/.claude/tool/.
# --- softkeys.mk, proved --------------------------------------------------
# A consumer with a hand-written Makefile (fuzznet) builds from softkeys.mk,
# so the tests are built here from that file's lists alone: a source it
# missed fails to link, and a header whose moc it missed fails the same way.
# Plain rules, no qmake, in a directory of their own.
MK_BUILD_DIR ?= $(BUILD_DIR)-mk
MK_BIN = $(MK_BUILD_DIR)/softkeys-tests

SOFTKEYS_DIR = .
include softkeys.mk

QT_PC    ?= Qt6Widgets
MOC      ?= $(shell $(QMAKE) -query QT_HOST_LIBEXECS 2>/dev/null)/moc
MK_FLAGS  = -std=c++17 -Os -fPIC -Wall -Wextra -I$(SOFTKEYS_INCLUDE) -I$(MK_BUILD_DIR) \
            $(shell pkg-config --cflags $(QT_PC) Qt6Test)
MK_LIBS   = $(shell pkg-config --libs $(QT_PC) Qt6Test)

MK_MOC_SRCS = $(patsubst $(SOFTKEYS_INCLUDE)/softkeys/%.h,$(MK_BUILD_DIR)/moc_%.cpp,$(SOFTKEYS_MOC_HDRS))
MK_OBJS = $(patsubst $(SOFTKEYS_DIR)/src/%.cpp,$(MK_BUILD_DIR)/%.o,$(SOFTKEYS_SRCS)) \
          $(MK_MOC_SRCS:.cpp=.o) $(MK_BUILD_DIR)/test_main.o

$(MK_BUILD_DIR)/moc_%.cpp: $(SOFTKEYS_INCLUDE)/softkeys/%.h
	@mkdir -p $(MK_BUILD_DIR)
	$(MOC) $< -o $@

$(MK_BUILD_DIR)/main.moc: test/main.cpp
	@mkdir -p $(MK_BUILD_DIR)
	$(MOC) $< -o $@

$(MK_BUILD_DIR)/%.o: $(SOFTKEYS_DIR)/src/%.cpp $(SOFTKEYS_HDRS)
	@mkdir -p $(MK_BUILD_DIR)
	$(CXX) $(MK_FLAGS) -c $< -o $@

$(MK_BUILD_DIR)/moc_%.o: $(MK_BUILD_DIR)/moc_%.cpp
	$(CXX) $(MK_FLAGS) -c $< -o $@

$(MK_BUILD_DIR)/test_main.o: test/main.cpp $(MK_BUILD_DIR)/main.moc $(SOFTKEYS_HDRS)
	$(CXX) $(MK_FLAGS) -c $< -o $@

$(MK_BIN): $(MK_OBJS)
	$(CXX) $^ -o $@ $(MK_LIBS)

check-mk: $(MK_BIN)
	QT_QPA_PLATFORM=offscreen timeout $(TEST_TIMEOUT) ./$(MK_BIN)

style:
	python3 tool/test_style_gate.py
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
	@for dir in $(BUILD_DIR) $(TEST_BUILD_DIR) $(MK_BUILD_DIR); do \
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

.PHONY: all tests-build test check-mk style hooks clean veryclean help FORCE
FORCE:
