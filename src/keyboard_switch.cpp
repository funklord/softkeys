#include "softkeys/keyboard_switch.h"

#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QInputMethod>
#include <QTimer>

#include "softkeys/metrics.h"
#include "softkeys/painted_icon.h"

sk_keyboard_switch::sk_keyboard_switch(QWidget *window)
    : QToolButton(window),
      m_window(window) {
	setObjectName(QStringLiteral("keyboard-switch"));
	setAccessibleName(QStringLiteral("Switch keyboard"));
	setToolTip(QStringLiteral("Switch keyboard"));

	/* The field being typed into keeps the focus while this is pressed. */
	setFocusPolicy(Qt::NoFocus);
	setAutoRaise(false);
	setFixedSize(SK_TOUCH_TARGET_DP, SK_TOUCH_TARGET_DP);

	const int size = qMax(16, int(SK_TOUCH_TARGET_DP * 0.6));
	setIcon(sk_painted_icon(SK_ICON_KEYBOARD, palette().color(QPalette::ButtonText), size * 2));
	setIconSize(QSize(size, size));
	hide();

	m_panel_probe = [] { return QGuiApplication::inputMethod()->isVisible(); };

	connect(this, &QAbstractButton::clicked, this, &sk_keyboard_switch::switch_requested);

	/*
	 * Queued: the input method reports itself visible before the window
	 * has been resized to the room it leaves, and the focus moves before
	 * the new field has its geometry.
	 */
	const auto later = [this] { QTimer::singleShot(0, this, [this] { refresh(); }); };
	QInputMethod *input = QGuiApplication::inputMethod();
	connect(input, &QInputMethod::visibleChanged, this, later);
	connect(input, &QInputMethod::keyboardRectangleChanged, this, later);
	connect(qApp, &QApplication::focusChanged, this, later);
	m_window->installEventFilter(this);
}

void sk_keyboard_switch::set_offered(bool offered) {
	m_offered = offered;
	refresh();
}

void sk_keyboard_switch::set_panel_probe(std::function<bool()> probe) {
	m_panel_probe = std::move(probe);
	refresh();
}

QWidget *sk_keyboard_switch::focused_field() const {
	QWidget *focus = QApplication::focusWidget();
	if (!focus || focus == this || !m_window->isAncestorOf(focus)) return nullptr;
	if (!focus->isVisible()) return nullptr;
	return focus->inputMethodQuery(Qt::ImEnabled).toBool() ? focus : nullptr;
}

void sk_keyboard_switch::refresh() {
	QWidget *field = m_offered && m_panel_probe && m_panel_probe() ? focused_field() : nullptr;
	if (!field) {
		hide();
		return;
	}

	/*
	 * Inside the window's CONTENTS, not its whole height. Qt 6.9 and later
	 * draw an Android window edge to edge and report the system keyboard
	 * as part of the safe area, which the layouts keep clear through the
	 * contents margins while the window itself stays full height -- so on
	 * the phone the button sat under the very keyboard it switches away
	 * from. Where the window does shrink (adjustResize without the safe
	 * area, or a desktop) the contents are the window and this is the same.
	 */
	const QRect room = m_window->contentsRect();
	const int margin = 4;
	QRect spot(QPoint(room.left() + margin, room.bottom() - margin - height() + 1), size());
	const QRect under(field->mapTo(m_window, QPoint(0, 0)), field->size());
	if (spot.intersects(under)) spot.moveRight(room.right() - margin);
	setGeometry(spot);
	show();
	raise();
}

bool sk_keyboard_switch::eventFilter(QObject *watched, QEvent *event) {
	if (watched == m_window && (event->type() == QEvent::Resize
	                            || event->type() == QEvent::ContentsRectChange)) {
		QTimer::singleShot(0, this, [this] { refresh(); });
	}
	return QToolButton::eventFilter(watched, event);
}
