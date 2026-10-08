#include <QAbstractButton>
#include <QApplication>
#include <QtTest>

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

} /* namespace */

class softkeys_test : public QObject {
	Q_OBJECT

private slots:
	void a_tap_cycles_once_locked_off();
	void a_hold_carries_keys_and_leaves_nothing();
	void every_compact_page_is_one_height_and_starts_with_digits();
	void keys_reach_the_target_as_qt_keys_and_text();
	void the_split_holds_the_compact_keys_and_two_more();
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
