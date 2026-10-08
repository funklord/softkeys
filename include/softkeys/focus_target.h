#ifndef SK_FOCUS_TARGET_H
#define SK_FOCUS_TARGET_H

#include <QString>

#include "softkeys/modifiers.h"
#include "softkeys/target.h"

/*
 * The target for an ordinary application (softkeys' project.md sec 4):
 * every key goes to whatever has focus, as the key events a physical
 * keyboard would have produced, so a QLineEdit, a QTextEdit or anything
 * else that reads key events works with no code of its own.
 *
 * The keyboard's buttons take no focus, so the widget the user was typing
 * into keeps it while they press them.
 *
 * Delivered to the focus object directly. That skips the application's
 * shortcut map, which is what a physical key passes through first -- so a
 * QAction on Ctrl+S is not reached from here. Recorded in project.md sec
 * 7; the widget-level bindings (undo, the clipboard, selection, word
 * jumps) are the widget's own and all arrive.
 */
class sk_focus_target : public sk_target {
public:
	sk_modifiers &modifiers() override { return m_modifiers; }
	void key(int qt_key, quint8 modifiers) override;
	void text(const QString &text) override;

private:
	void send(int qt_key, quint8 modifiers, const QString &text);

	sk_modifiers m_modifiers;
};

#endif /* SK_FOCUS_TARGET_H */
