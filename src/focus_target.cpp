#include "softkeys/focus_target.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>

namespace {

Qt::KeyboardModifiers qt_modifiers(quint8 modifiers) {
	Qt::KeyboardModifiers out;
	if (modifiers & SK_MOD_SHIFT) out |= Qt::ShiftModifier;
	if (modifiers & SK_MOD_CTRL) out |= Qt::ControlModifier;
	if (modifiers & SK_MOD_ALT) out |= Qt::AltModifier;
	return out;
}

/*
 * The text a physical keyboard attaches to a named key. A widget inserts
 * from the text rather than the key -- QTextEdit puts a tab in only when
 * the event carries one -- so a Tab with no text does nothing at all.
 */
QString text_of(int qt_key) {
	switch (qt_key) {
	case Qt::Key_Tab:       return QStringLiteral("\t");
	case Qt::Key_Return:
	case Qt::Key_Enter:     return QStringLiteral("\r");
	case Qt::Key_Backspace: return QStringLiteral("\b");
	case Qt::Key_Escape:    return QStringLiteral("\x1b");
	case Qt::Key_Delete:    return QStringLiteral("\x7f");
	default:                return QString();
	}
}

} /* namespace */

void sk_focus_target::key(int qt_key, quint8 modifiers) {
	const quint8 all = quint8(modifiers | m_modifiers.effective());
	const bool chord = all & (SK_MOD_CTRL | SK_MOD_ALT);
	send(qt_key, all, chord ? QString() : text_of(qt_key));
	m_modifiers.spend();
}

void sk_focus_target::text(const QString &text) {
	const quint8 all = m_modifiers.effective();

	/*
	 * Ctrl or Alt over a character is a key, not text: Ctrl+Z is undo
	 * because the widget matches the KEY against its bindings, and an event
	 * carrying "z" as text is typed as a z. The key code is the character's
	 * capital, which is what Qt::Key_A to Key_Z are.
	 */
	if (all & (SK_MOD_CTRL | SK_MOD_ALT)) {
		if (text.size() == 1) send(text.at(0).toUpper().unicode(), all, QString());
		m_modifiers.spend();
		return;
	}

	/*
	 * Shift and AltGr already chose the character. Shift still goes with
	 * it, as it does from a physical keyboard; AltGr is not a Qt modifier
	 * a widget reads.
	 */
	const int code = text.size() == 1 ? int(text.at(0).toUpper().unicode()) : int(Qt::Key_unknown);
	send(code, all, text);
	m_modifiers.spend();
}

void sk_focus_target::send(int qt_key, quint8 modifiers, const QString &text) {
	QObject *receiver = QGuiApplication::focusObject();
	if (!receiver) return;

	const Qt::KeyboardModifiers held = qt_modifiers(modifiers);
	QKeyEvent press(QEvent::KeyPress, qt_key, held, text);
	QCoreApplication::sendEvent(receiver, &press);

	/* The press may have moved focus -- Tab does -- and the release goes
	 * where a physical one would, to whatever has focus now. */
	receiver = QGuiApplication::focusObject();
	if (!receiver) return;
	QKeyEvent release(QEvent::KeyRelease, qt_key, held, text);
	QCoreApplication::sendEvent(receiver, &release);
}
