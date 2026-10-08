# softkeys for a qmake consumer: include(path/to/softkeys/softkeys.pri) and the
# keyboard's sources are compiled into the application, moc included.
#
# Compiled in rather than linked as a library because a consumer builds it
# with its own flags and its own Qt anyway, and qmake runs moc on the
# headers listed here without a second project to keep in step.

SOFTKEYS_ROOT = $$PWD

QT += widgets
CONFIG += c++17
INCLUDEPATH += $$SOFTKEYS_ROOT/include

SOURCES += \
    $$SOFTKEYS_ROOT/src/focus_target.cpp \
    $$SOFTKEYS_ROOT/src/key_cap.cpp \
    $$SOFTKEYS_ROOT/src/key_row.cpp \
    $$SOFTKEYS_ROOT/src/key_row_layout.cpp \
    $$SOFTKEYS_ROOT/src/keyboard.cpp \
    $$SOFTKEYS_ROOT/src/keyboard_layout.cpp \
    $$SOFTKEYS_ROOT/src/modifiers.cpp \
    $$SOFTKEYS_ROOT/src/resize_grip.cpp \
    $$SOFTKEYS_ROOT/src/touch_chords.cpp

HEADERS += \
    $$SOFTKEYS_ROOT/include/softkeys/focus_target.h \
    $$SOFTKEYS_ROOT/include/softkeys/key_cap.h \
    $$SOFTKEYS_ROOT/include/softkeys/key_row.h \
    $$SOFTKEYS_ROOT/include/softkeys/key_row_layout.h \
    $$SOFTKEYS_ROOT/include/softkeys/keyboard.h \
    $$SOFTKEYS_ROOT/include/softkeys/keyboard_layout.h \
    $$SOFTKEYS_ROOT/include/softkeys/metrics.h \
    $$SOFTKEYS_ROOT/include/softkeys/modifiers.h \
    $$SOFTKEYS_ROOT/include/softkeys/resize_grip.h \
    $$SOFTKEYS_ROOT/include/softkeys/target.h \
    $$SOFTKEYS_ROOT/include/softkeys/touch_chords.h
