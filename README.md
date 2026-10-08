# softkeys

An on-screen keyboard for Qt Widgets applications on touch screens, as a
library the application embeds: compact, full-size and split keyboards,
language layouts, two-finger modifier chords, and a strip of extra keys
for use beside the system keyboard. Extracted from BeerSSH.

`project.md` holds the design and the plan.

## Building

    make            # the static library, in build/
    fmake           # the same, with no build file to read

    make test       # build and run the tests
    fmake test

    make style      # the style gate

An application built with qmake compiles softkeys in with
`include(softkeys/softkeys.pri)`.

For an ordinary application, `sk_focus_target` types into whatever widget has
focus, and `sk_set_compact_pages` adds the `editing` page -- undo, the
clipboard, selection, word jumps:

    static sk_focus_target target;
    sk_set_compact_pages({ "letters", "numbers", "editing" });
    auto *keyboard = new sk_keyboard(&target, window);

## Copyright

Copyright (C) 2026 Nabeel Sowan <nabeel@vibes.se>
