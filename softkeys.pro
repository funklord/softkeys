# The library on its own: what `make` builds, so the tree compiles without a
# consumer. Applications use softkeys.pri instead.

TEMPLATE = lib
CONFIG += staticlib
TARGET = softkeys
DESTDIR = lib

include(softkeys.pri)

# Size over speed, as every project here builds (build-and-commit.md).
QMAKE_CXXFLAGS += -Wall -Wextra
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -Os
