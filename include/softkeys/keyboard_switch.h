#ifndef SK_KEYBOARD_SWITCH_H
#define SK_KEYBOARD_SWITCH_H

#include <QToolButton>

#include <functional>

/*
 * The switch from the system keyboard to this one (softkeys' project.md
 * sec 4): a small button over the application's own window, at the
 * bottom-left -- the top-left of the space the system keyboard leaves,
 * since an application cannot draw on that keyboard, which is a window
 * of its own.
 *
 * Shown only while it means something: the application offers it, the
 * system keyboard is up, and the focus is a text field inside the
 * window. Pressing it only says so; the application hides the system
 * keyboard and shows its sk_keyboard, because it is the one that knows
 * where that keyboard goes and how to keep the system's down.
 *
 * Moved to the bottom-right when the focused field is under the
 * bottom-left, so it never covers what is being typed into.
 */
class sk_keyboard_switch : public QToolButton {
	Q_OBJECT

public:
	explicit sk_keyboard_switch(QWidget *window);

	/* Whether the application offers the switch at all. Off by default. */
	void set_offered(bool offered);
	bool offered() const { return m_offered; }

	/*
	 * Whether the system keyboard is up. QInputMethod answers by default;
	 * a test running offscreen, where there is no system keyboard, says.
	 */
	void set_panel_probe(std::function<bool()> probe);

	/* Show or hide, and place, from the state now; signals call it too. */
	void refresh();

signals:
	void switch_requested();

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	QWidget *focused_field() const;

	QWidget *m_window;
	bool m_offered = false;
	std::function<bool()> m_panel_probe;
};

#endif /* SK_KEYBOARD_SWITCH_H */
