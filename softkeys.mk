# softkeys for a hand-written Makefile -- the counterpart of softkeys.pri.
#
#     SOFTKEYS_DIR = path/to/softkeys
#     include $(SOFTKEYS_DIR)/softkeys.mk
#
# and then compile $(SOFTKEYS_SRCS) and the moc output of
# $(SOFTKEYS_MOC_HDRS) with -I$(SOFTKEYS_INCLUDE), C++17 and Qt Widgets.
#
# The lists are derived from the tree rather than written out, so a file
# softkeys adds reaches every consumer without an edit there: the sources
# are every src/*.cpp, and the headers moc must see are the ones that
# declare Q_OBJECT. `make check-mk` in this tree builds and runs the tests
# from these lists alone, which is what keeps them honest.

SOFTKEYS_DIR ?= softkeys

SOFTKEYS_INCLUDE  := $(SOFTKEYS_DIR)/include
SOFTKEYS_SRCS     := $(wildcard $(SOFTKEYS_DIR)/src/*.cpp)
SOFTKEYS_HDRS     := $(wildcard $(SOFTKEYS_INCLUDE)/softkeys/*.h)

# /dev/null so that grep, given no headers, reads nothing rather than stdin.
SOFTKEYS_MOC_HDRS := $(shell grep -l '\bQ_OBJECT\b' $(SOFTKEYS_HDRS) /dev/null)
