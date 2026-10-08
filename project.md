# project.md

**softkeys** -- an on-screen keyboard for Qt Widgets applications on
touch screens, as a library the application embeds. Extracted from
BeerSSH's built-in keyboard on 2026-10-08, on the copyright holder's
instruction, so that the holder's other Qt applications get the same
keyboard rather than Android's.

This document is the source of truth and wins over the code. Where the
implementation learns something this does not say, this gains it.

## 1. What it is, and what it is not

**For the holder's own applications only** -- settled 2026-10-08 in as
many words. Inside an application it replaces the system keyboard; it is
not a system-wide Android input method. A phone-wide keyboard would be an
Android `InputMethodService` drawing native views, which Qt Widgets cannot
realistically run inside, so it would share these layouts and this design
but not this code. Not planned.

**Why the system keyboard is not enough** (the holder's reasons): arrow
keys, undo, cut and paste "suck on the android keyboard". A terminal adds
Ctrl, Esc, Tab and the function keys, which is where this keyboard came
from.

## 2. Consumers

| Consumer | How | Status |
|---|---|---|
| beerssh | vendored submodule; its terminal is a target | where it came from; moves over first |
| fuzznet | vendored submodule; its `gui/` views have text entry | the first NEW consumer, the holder's choice |
| fuzzypickles | through fuzznet's views | follows fuzznet |

fuzznet's views with text entry, counted 2026-10-08: `config_view`,
`entries_view`, `log_view`, `notebook_view`, `retention_view` (QLineEdit,
QTextEdit or QPlainTextEdit). fuzznet builds them with a hand-written
Makefile rather than qmake, so softkeys must be consumable from both: a
`softkeys.pri` for qmake and a source list a Makefile can read.

## 3. What comes with it from BeerSSH

All of it already built, tested and sabotage-proved there (BeerSSH's
project.md sec 6.2 has the history and the measurements):

- **Pages and styles.** Compact (thumb-sized, with a digit row on every
  page), full (a real keyboard's proportions), and split (the compact
  keyboard cut in two, beside the content, automatic on a short wide
  screen). Language layouts from XKB-style slot tables, a fourth page where
  a layout needs one, AltGr.
- **Modifiers, three ways at once.** A tap arms for one key, a second locks,
  a third releases (the Treo's cycle); a finger held on a modifier while
  another presses keys chords as on a physical keyboard. The states are
  drawn by the key itself -- ring and short bar for armed, filled with a
  long bar for locked.
- **Two-finger touch** (`touch_chords`), across the split halves too.
- **Resize bars** on the edge facing the content, sizes kept per keyboard.
- **The key row** -- a strip of extra keys over the system keyboard, for
  when the system keyboard is the one in use.

## 4. Design for being generic

Three things tied the keyboard to BeerSSH, and each becomes an interface:

- **What it types into: `sk_target`.** Named keys go out as `Qt::Key`
  values and text as text. softkeys ships a default target that sends
  ordinary Qt key events to whatever has focus, so any text field works --
  arrows, Home/End, and Ctrl+Z/X/C/V for undo and the clipboard. BeerSSH
  supplies its own target, adapting to its terminal input router.
- **The modifier state: `sk_modifiers`.** The once/locked/held machine moves
  here. BeerSSH's router holds one and delegates, because its physical
  keyboard and its on-screen one must share a single state (a Ctrl armed on
  screen applies to the next physical key, and Caps-as-Ctrl arms it).
- **Key codes.** The catalogue names keys in `Qt::Key` rather than in
  BeerSSH's emulator codes. Modifier bits keep the values BeerSSH already
  uses -- Shift 0x01, Alt 0x02, Ctrl 0x04, AltGr 0x08 -- so its adapter can
  assert the two agree at compile time.

**Pages per application.** A terminal wants its F-keys and Esc; a text
editor wants an editing page -- arrows, word jumps, Home/End, undo/redo,
cut/copy/paste, select all. The application chooses.

**The switch to and from the system keyboard.** An application cannot draw
on the system keyboard, which is a window of its own above the
application's. What it can do is put a small button in its own window at
the top-left of the space the system keyboard leaves -- Qt reports where
that keyboard is -- which hides the system keyboard and shows this one; a
key on this one switches back.

## 5. Plan

1. **Skeleton** -- this tree, its gate and its build. [2026-10-08]
2. **Move the code** from BeerSSH's `keyboard/`, with the three interfaces
   above, and its tests. BeerSSH vendors this tree and switches over; its
   whole suite and its keyboard sabotage specs are the proof that nothing
   changed in the move. **Where it stands, 2026-10-08:** the interfaces and
   the `sk_` names were made inside BeerSSH first (its commits `a67bc6a`,
   `19e6d86`) and proved by its suite; the code is here from `19e6d86`, with
   this tree's own tests. BeerSSH builds from this tree as its `softkeys`
   submodule since 2026-10-08 (its commit `0f8357b`), with its suite
   unchanged at 756 and its 73 keyboard sabotage specs all caught here.
3. **The generic target and the editing page.** [2026-10-08]
   `sk_focus_target` sends every key to the focused widget as the key event
   a physical keyboard would; `sk_set_compact_pages` lets an application
   choose its pages, and `editing` is the new one -- Undo, Redo, Cut, Copy,
   Paste, All, Del; Home, word back, the arrows, word forward, End, PgUp,
   PgDn; Shift for selecting. Its keys are the widgets' own bindings (Ctrl+Z,
   Ctrl+Shift+Z, Ctrl+Left...), so the widget decides what each does. The
   default pages stay letters, numbers, terminal, so BeerSSH is unchanged.
4. **The switch button.**
5. **fuzznet adopts it**, arranged with whichever session is working there
   (its inbox, `.git/cc-inbox/`), since that tree is in active use.

## 6. Findings

**A class constant with no definition linked only when optimised.**
`static const int SMALLEST_KEY_DP` and `THICKNESS_DP` were declared in their
classes and never defined outside them; `qBound` takes its arguments by
reference, which needs the definition. Under qmake's `-Os` the value was
folded in and the symbol never asked for, so BeerSSH never saw it; fmake's
less optimised test build failed to link (`undefined reference to
sk_keyboard::SMALLEST_KEY_DP`). Both are `static constexpr` now, which is
its own definition since C++17. Found by trying `fmake` before writing its
README line, which is what that rule is for.

**Comments cite BeerSSH's project.md.** The code came from there, and a bare
`sec 6.2` in a comment is BeerSSH's section, not one of this document's.
They are left as they are rather than rewritten from memory; a comment that
grows a softkeys-specific claim cites this document by name.

**A widget refuses Ctrl text by itself, so a test of Ctrl cannot see a
target that sends it.** The focus target turns Ctrl or Alt over a letter into
a key with no text. Deleting that branch left the Ctrl+A test green: Qt's
`QInputControl` refuses text under Ctrl, and the key alone matched Select All.
Alt has no such guard, so the test asserts Alt+f types nothing; that is the
case the branch exists for, and the sabotage now fails there.

**The style gate is the shared one, and so is its suite.** softkeys was added
to the holder's private list on 2026-10-08 (claude-guidelines `fc87500`), which
puts it in `sync.py`'s spread: `tool/style_gate.py` and now
`tool/test_style_gate.py` are copies of `~/.claude/tool/`, and `make style`
runs the suite before the gate, since a gate's pass means something only once
the gate is known to work.

## 7. Open

- **Application shortcuts are not reached from the keyboard.** The focus
  target delivers to the focus object, past the shortcut map a physical key
  goes through first, so a `QAction` on Ctrl+S does not fire. Reaching the
  map needs Qt's private window-system interface (`gui-private`), which every
  consumer would then have to build against; not done until an application
  wants a shortcut from the keyboard.

- **Licence:** none chosen; the holder's to decide.
- **The editor for the keyboard's keys** that the key row already has:
  deferred by the holder until the default keyboard is right (BeerSSH sec
  6.2, 2026-10-08).
