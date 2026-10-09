#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPlainTextEdit>
#include <QtTest>

#include "softkeys/focus_target.h"
#include "softkeys/key_cap.h"
#include "softkeys/keyboard.h"
#include "softkeys/keyboard_layout.h"
#include "softkeys/modifiers.h"
#include "softkeys/target.h"

namespace {

/*
 * A target that writes down what reached it, so the keyboard is tested
 * without anything to type into. It spends the armed modifiers after each
 * keystroke, which is a target's job (sk_target::key).
 */
class recording_target : public sk_target {
public:
	sk_modifiers &modifiers() override { return m_modifiers; }

	void key(int qt_key, quint8 modifiers) override {
		keys.append(qMakePair(qt_key, quint8(modifiers | m_modifiers.effective())));
		m_modifiers.spend();
	}

	void text(const QString &text) override {
		texts.append(qMakePair(text, m_modifiers.effective()));
		m_modifiers.spend();
	}

	QList<QPair<int, quint8>> keys;
	QList<QPair<QString, quint8>> texts;

private:
	sk_modifiers m_modifiers;
};

QAbstractButton *button_labelled(const QWidget &parent, const QString &label) {
	for (QAbstractButton *button : parent.findChildren<QAbstractButton *>()) {
		if (button->text() == label) return button;
	}
	return nullptr;
}

QStringList labels(const QWidget *within) {
	QStringList out;
	for (const sk_key_cap *cap : within->findChildren<sk_key_cap *>()) out.append(cap->text());
	out.sort();
	return out;
}

/*
 * The compact pages back to the default however a test leaves, since the
 * catalog is one per application and every later test reads it.
 */
struct pages_restored {
	const QStringList saved = sk_compact_pages();
	~pages_restored() { sk_set_compact_pages(saved); }
};

/* A line edit in a window that is active, so it holds the focus. */
bool focus_on(QLineEdit &edit) {
	edit.show();
	edit.activateWindow();
	edit.setFocus();
	return QTest::qWaitForWindowActive(&edit) && QGuiApplication::focusObject() == &edit;
}

} /* namespace */

class softkeys_test : public QObject {
	Q_OBJECT

private slots:
	void a_tap_cycles_once_locked_off();
	void a_hold_carries_keys_and_leaves_nothing();
	void every_compact_page_is_one_height_and_starts_with_digits();
	void keys_reach_the_target_as_qt_keys_and_text();
	void the_split_holds_the_compact_keys_and_two_more();
	void the_focus_target_types_into_the_focused_field();
	void ctrl_over_a_letter_is_the_widgets_binding();
	void a_named_key_carries_the_text_a_keyboard_would();
	void the_editing_page_cuts_pastes_undoes_and_selects();
	void pages_are_the_applications_choice();
	void a_half_header_sits_above_the_keys_and_survives_a_rebuild();
};

void softkeys_test::a_tap_cycles_once_locked_off() {
	sk_modifiers state;
	state.cycle(SK_MOD_CTRL);
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::ONCE);

	/* A keystroke spends an arm and not a lock. */
	state.spend();
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::OFF);

	state.cycle(SK_MOD_CTRL);
	state.cycle(SK_MOD_CTRL);
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::LOCKED);
	state.spend();
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::LOCKED);

	state.cycle(SK_MOD_CTRL);
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::OFF);
}

void softkeys_test::a_hold_carries_keys_and_leaves_nothing() {
	sk_modifiers state;

	/* Down, a key, up: a chord, so not a tap, and nothing left behind. */
	state.hold(SK_MOD_CTRL);
	QCOMPARE(state.effective(), quint8(SK_MOD_CTRL));
	state.spend();
	QCOMPARE(state.effective(), quint8(SK_MOD_CTRL));
	QVERIFY(!state.release(SK_MOD_CTRL));
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::OFF);

	/* Down and up with nothing between is a tap. */
	state.hold(SK_MOD_CTRL);
	QVERIFY(state.release(SK_MOD_CTRL));

	/* A hold over a lock leaves the lock: the hold never owned it. */
	state.cycle(SK_MOD_CTRL);
	state.cycle(SK_MOD_CTRL);
	state.hold(SK_MOD_CTRL);
	state.spend();
	QVERIFY(!state.release(SK_MOD_CTRL));
	QCOMPARE(state.state(SK_MOD_CTRL), sk_modifiers::LOCKED);
}

void softkeys_test::every_compact_page_is_one_height_and_starts_with_digits() {
	const QList<sk_group_t> &pages = sk_catalog::instance().all(sk_catalog::STYLE_COMPACT);
	QVERIFY(pages.size() >= 3);

	const QStringList digits = { QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
	                             QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6"),
	                             QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9"),
	                             QStringLiteral("0"), QStringLiteral("-") };
	for (const sk_group_t &page : pages) {
		QCOMPARE(page.rows.size(), pages.first().rows.size());
		QStringList top;
		for (const sk_key_t &key : page.rows.value(0)) top.append(key.label);
		QCOMPARE(top, digits);
		QVERIFY(page.digit_row_first);
	}
}

void softkeys_test::keys_reach_the_target_as_qt_keys_and_text() {
	recording_target target;
	sk_keyboard keyboard(&target);

	QAbstractButton *esc = button_labelled(keyboard, QStringLiteral("Esc"));
	QAbstractButton *q = button_labelled(keyboard, QStringLiteral("q"));
	QAbstractButton *ctrl = button_labelled(keyboard, QStringLiteral("Ctrl"));
	QVERIFY(esc && q && ctrl);

	/* A named key is a Qt::Key: nothing of any one application's codes. */
	esc->click();
	QCOMPARE(target.keys.size(), 1);
	QCOMPARE(target.keys.at(0).first, int(Qt::Key_Escape));

	/* A letter is text, carrying an armed Ctrl, which it then spends. */
	ctrl->click();
	QCOMPARE(target.modifiers().state(SK_MOD_CTRL), sk_modifiers::ONCE);
	q->click();
	QCOMPARE(target.texts.size(), 1);
	QCOMPARE(target.texts.at(0).first, QStringLiteral("q"));
	QCOMPARE(target.texts.at(0).second, quint8(SK_MOD_CTRL));
	QCOMPARE(target.modifiers().state(SK_MOD_CTRL), sk_modifiers::OFF);
}

void softkeys_test::the_split_holds_the_compact_keys_and_two_more() {
	recording_target target;
	sk_keyboard keyboard(&target);

	/*
	 * Each page split: compact's keys, less the digit row the split leaves
	 * out, plus a second space bar and a second Shift, because each thumb
	 * reaches only its own half.
	 */
	const QList<sk_group_t> &pages = sk_catalog::instance().all(sk_catalog::STYLE_COMPACT);
	for (int g = 0; g < pages.size(); ++g) {
		keyboard.set_style(sk_catalog::STYLE_COMPACT);
		keyboard.set_group(g);
		QStringList expected = labels(&keyboard);
		if (pages.at(g).digit_row_first) {
			for (const sk_key_t &key : pages.at(g).rows.at(0)) expected.removeOne(key.label);
		}
		if (expected.contains(QStringLiteral("space"))) expected.append(QStringLiteral("space"));
		if (expected.contains(QStringLiteral("Shift"))) expected.append(QStringLiteral("Shift"));
		expected.sort();

		keyboard.set_style(sk_catalog::STYLE_SPLIT);
		keyboard.set_group(g);
		QStringList split = labels(keyboard.half(0)) + labels(keyboard.half(1));
		split.sort();
		QCOMPARE(split, expected);
	}
}

void softkeys_test::the_focus_target_types_into_the_focused_field() {
	QLineEdit edit;
	QVERIFY(focus_on(edit));
	sk_focus_target target;

	target.text(QStringLiteral("a"));
	target.text(QStringLiteral("b"));
	QCOMPARE(edit.text(), QStringLiteral("ab"));

	target.key(Qt::Key_Left, SK_MOD_NONE);
	target.text(QStringLiteral("x"));
	QCOMPARE(edit.text(), QStringLiteral("axb"));

	target.key(Qt::Key_Backspace, SK_MOD_NONE);
	QCOMPARE(edit.text(), QStringLiteral("ab"));
}

void softkeys_test::a_named_key_carries_the_text_a_keyboard_would() {
	/* A text editor inserts a tab from the event's text, not its key. */
	QPlainTextEdit edit;
	edit.show();
	edit.activateWindow();
	edit.setFocus();
	QVERIFY(QTest::qWaitForWindowActive(&edit));
	QCOMPARE(QGuiApplication::focusObject(), &edit);

	sk_focus_target target;
	target.text(QStringLiteral("a"));
	target.key(Qt::Key_Tab, SK_MOD_NONE);
	target.text(QStringLiteral("b"));
	QCOMPARE(edit.toPlainText(), QStringLiteral("a\tb"));
}

void softkeys_test::ctrl_over_a_letter_is_the_widgets_binding() {
	QLineEdit edit;
	edit.setText(QStringLiteral("abc"));
	QVERIFY(focus_on(edit));
	sk_focus_target target;

	/* An armed Ctrl and the a key: select all, not an "a" typed. */
	target.modifiers().cycle(SK_MOD_CTRL);
	target.text(QStringLiteral("a"));
	QCOMPARE(edit.text(), QStringLiteral("abc"));
	QCOMPARE(edit.selectedText(), QStringLiteral("abc"));
	QCOMPARE(target.modifiers().state(SK_MOD_CTRL), sk_modifiers::OFF);

	/*
	 * Alt over a letter is a key with no text, as Ctrl is. A widget
	 * refuses Ctrl text by itself, so Ctrl alone cannot show it; Alt is
	 * where a letter sent as text gets typed.
	 */
	edit.setText(QStringLiteral("abc"));
	target.modifiers().cycle(SK_MOD_ALT);
	target.text(QStringLiteral("f"));
	QCOMPARE(edit.text(), QStringLiteral("abc"));
}

void softkeys_test::the_editing_page_cuts_pastes_undoes_and_selects() {
	pages_restored restore;
	QVERIFY(sk_set_compact_pages({ QStringLiteral("letters"), QStringLiteral("editing") }));

	QLineEdit edit;
	edit.setText(QStringLiteral("hello world"));
	QVERIFY(focus_on(edit));
	sk_focus_target target;
	sk_keyboard keyboard(&target);
	keyboard.set_style(sk_catalog::STYLE_COMPACT);
	const int page = sk_catalog::instance().index_of(QStringLiteral("editing"),
	                                                 sk_catalog::STYLE_COMPACT);
	QVERIFY(page >= 0);
	keyboard.set_group(page);

	auto press = [&](const char *label) {
		QAbstractButton *button = button_labelled(keyboard, QString::fromUtf8(label));
		if (!button) QTest::qFail(qPrintable(QStringLiteral("no key labelled %1").arg(QString::fromUtf8(label))), __FILE__, __LINE__);
		else button->click();
	};

	QApplication::clipboard()->clear();
	press("All");
	press("Cut");
	QCOMPARE(edit.text(), QString());
	QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("hello world"));

	press("Paste");
	press("Paste");
	QCOMPARE(edit.text(), QStringLiteral("hello worldhello world"));
	press("Undo");
	QCOMPARE(edit.text(), QStringLiteral("hello world"));
	press("Redo");
	QCOMPARE(edit.text(), QStringLiteral("hello worldhello world"));

	/* Shift armed over a word jump selects the word. */
	edit.setText(QStringLiteral("hello world"));
	press("Hom");
	press("Shift");
	press("\u00bb");
	QVERIFY2(edit.selectedText().startsWith(QStringLiteral("hello")),
	         qPrintable(edit.selectedText()));
	QVERIFY(!edit.selectedText().contains(QStringLiteral("world")));

	press("Copy");
	QCOMPARE(QApplication::clipboard()->text(), edit.selectedText());
}

void softkeys_test::pages_are_the_applications_choice() {
	pages_restored restore;
	const QStringList before = sk_compact_pages();
	QString why;

	QVERIFY(!sk_set_compact_pages({ QStringLiteral("editing") }, &why));
	QVERIFY(why.contains(QStringLiteral("letters")));
	QVERIFY(!sk_set_compact_pages({ QStringLiteral("letters"), QStringLiteral("edit") }, &why));
	QVERIFY(why.contains(QStringLiteral("edit")));
	QVERIFY(!sk_set_compact_pages({ QStringLiteral("letters"), QStringLiteral("letters") }, &why));
	QCOMPARE(sk_compact_pages(), before);

	const QStringList every = sk_compact_page_ids();
	QVERIFY(sk_set_compact_pages(every));
	QStringList built;
	for (const sk_group_t &page : sk_catalog::instance().all(sk_catalog::STYLE_COMPACT)) {
		built.append(page.id);

		/* Every row fills its grid: a short one puts each key after
		 * the mistake in the wrong column. */
		for (const QList<sk_key_t> &row : page.rows) {
			int columns = 0;
			for (const sk_key_t &key : row) columns += key.span;
			QCOMPARE(columns, page.columns);
		}
		QCOMPARE(page.rows.size(), sk_catalog::instance().all(sk_catalog::STYLE_COMPACT).first().rows.size());
	}
	QCOMPARE(built.mid(0, every.size()), every);
}

void softkeys_test::a_half_header_sits_above_the_keys_and_survives_a_rebuild() {
	recording_target target;
	sk_keyboard keyboard(&target);
	keyboard.set_style(sk_catalog::STYLE_SPLIT);

	/* The halves outside the keyboard, where an application puts them. */
	QWidget window;
	QHBoxLayout *row = new QHBoxLayout(&window);
	row->addWidget(keyboard.half(0));
	row->addWidget(keyboard.half(1));
	window.resize(800, 400);
	window.show();
	keyboard.show();
	QVERIFY(QTest::qWaitForWindowExposed(&window));

	QPointer<QLabel> status = new QLabel(QStringLiteral("connected"));
	keyboard.half_header(0)->layout()->addWidget(status);
	QCoreApplication::processEvents();

	/* A page change empties the grids; the header is not the grid's. */
	keyboard.set_group(1);
	QCoreApplication::processEvents();
	QTest::qWait(1);
	QVERIFY2(status, "a rebuild deleted what the application put in the header");
	QVERIFY(status->isVisible());

	const QList<sk_key_cap *> caps = keyboard.half(0)->findChildren<sk_key_cap *>();
	QVERIFY(!caps.isEmpty());
	int top = INT_MAX;
	for (const sk_key_cap *cap : caps) top = qMin(top, cap->mapTo(&window, QPoint(0, 0)).y());
	const int status_bottom = status->mapTo(&window, QPoint(0, status->height())).y();
	QVERIFY2(status_bottom <= top, qPrintable(QStringLiteral("the header ends at %1, the keys start at %2")
	                                               .arg(status_bottom).arg(top)));
}

int main(int argc, char *argv[]) {
	/*
	 * An offscreen run has nothing to theme, and a theme plugin such as
	 * gtk3 opens a display of its own whatever the platform is -- with no
	 * display it kills the run before a test (BeerSSH found this).
	 */
	if (qgetenv("QT_QPA_PLATFORM") == QByteArrayLiteral("offscreen")) {
		qunsetenv("QT_QPA_PLATFORMTHEME");
	}
	QApplication app(argc, argv);
	softkeys_test test;
	return QTest::qExec(&test, argc, argv);
}

#include "main.moc"
