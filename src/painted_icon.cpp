#include "softkeys/painted_icon.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

#include <cmath>

/*
 * The side-column icons and the switch button's, DRAWN rather than set
 * from a character.
 *
 * A gear at U+2699 and a chevron at U+25BE would be three lines of code
 * and a gamble: the label font is the system's, Android falls back
 * through it for symbols, and a device without the glyph draws a tofu
 * box. sec 11's rule is absent-not-faked, and a box where a control
 * should be is worse than the word it replaced. Qt's standard pixmaps
 * have no settings icon either.
 *
 * So they are painted from primitives, which cannot be missing, scale
 * with the button, and take their colour from the palette -- so they
 * follow the light and dark schemes without a second asset.
 */
QIcon sk_painted_icon(sk_icon_t icon, const QColor &ink, int size) {
	QPixmap canvas(size, size);
	canvas.fill(Qt::transparent);

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing, true);

	QPen pen(ink);
	pen.setWidthF(qMax(1.5, size / 11.0));
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter.setPen(pen);

	const qreal unit = size;
	if (icon == SK_ICON_SETTINGS) {
		/*
		 * A cog: the settings symbol everywhere a person has used a
		 * phone, so it needs no learning.
		 *
		 * This was three sliders, and the reasoning for them was
		 * backwards. Sliders were chosen because a gear's teeth are the
		 * first thing to turn to mush at 36dp -- an argument about
		 * RENDERING, offered against an argument about MEANING, and
		 * meaning wins: sliders read as a filter or an equaliser to most
		 * people, and an icon that has to be explained is the wrong
		 * icon. Reported by the copyright holder, who expected a cog.
		 *
		 * The rendering problem is real and is answered by drawing
		 * FEWER, FATTER teeth than a gear really has. Six radial strokes
		 * with round caps stay separate at any size this is drawn at; a
		 * finely-toothed gear becomes a blurred disc, which is what the
		 * old comment was afraid of and was right about.
		 */
		const QPointF centre(unit * 0.5, unit * 0.5);

		/*
		 * FILLED, not stroked, and that is the whole difference between
		 * a cog and a ship's wheel.
		 *
		 * The first attempt drew a stroked ring with six radial lines
		 * out of it. On a device it read as a helm or a sun: thin spokes
		 * STICKING OUT of a circle, where a gear's teeth are part of the
		 * rim. So the outline is one path -- out to the tooth, along it,
		 * back to the rim, around to the next -- filled solid, with the
		 * centre punched out by an odd-even subpath.
		 */
		/*
		 * Eight teeth with RADIAL sides, and the sides are what matter.
		 *
		 * The version before this walked from the rim at one angle out
		 * to the tip at another, so every tooth was a wedge that came to
		 * a point, and the whole read as a starfish -- reported as "not
		 * quite clear", which it was. A gear tooth has parallel flanks:
		 * both corners of its base sit at the SAME angles as the two
		 * corners of its tip, so the side runs straight out along a
		 * radius and the top is flat.
		 *
		 * Short teeth, too. The rim is 0.33 and the tip 0.42, about a
		 * quarter again rather than half, because a tall tooth on a
		 * small body is the other half of the starfish.
		 */
		const int teeth = 8;
		const qreal step = 360.0 / teeth;
		const qreal half = step * 0.20;
		const qreal outer = unit * 0.42;
		const qreal inner = unit * 0.33;

		const auto polar = [&](qreal degrees, qreal radius) {
			const qreal angle = qDegreesToRadians(degrees);
			return QPointF(centre.x() + std::cos(angle) * radius,
			                centre.y() + std::sin(angle) * radius);
		};

		/*
		 * Between the teeth the outline follows the root CIRCLE rather
		 * than a straight chord, which would flatten the body.
		 *
		 * arcTo's angles run the other way from polar(): Qt measures
		 * counter-clockwise from three o'clock and screen y grows
		 * downward, so a clockwise walk is negated.
		 */
		const QRectF root(centre.x() - inner, centre.y() - inner,
		                   inner * 2, inner * 2);

		QPainterPath gear;
		gear.moveTo(polar(-half, inner));
		for (int i = 0; i < teeth; ++i) {
			const qreal base = step * i;
			gear.lineTo(polar(base - half, inner));
			gear.lineTo(polar(base - half, outer));
			gear.lineTo(polar(base + half, outer));
			gear.lineTo(polar(base + half, inner));
			gear.arcTo(root, -(base + half), -(step - half * 2));
		}
		gear.closeSubpath();

		/* The hole. Without it a filled gear is just a lumpy disc. */
		gear.addEllipse(centre, unit * 0.15, unit * 0.15);
		gear.setFillRule(Qt::OddEvenFill);

		painter.setPen(Qt::NoPen);
		painter.fillPath(gear, ink);
	} else if (icon == SK_ICON_KEYBOARD) {
		/*
		 * A keyboard: a rounded body, two rows of keys and a space bar.
		 * What Android draws for "switch keyboard" too, so it reads as
		 * the other keyboard rather than as this one going away.
		 */
		painter.drawRoundedRect(QRectF(unit * 0.12, unit * 0.26, unit * 0.76, unit * 0.50),
		                         unit * 0.08, unit * 0.08);
		for (int row = 0; row < 2; ++row) {
			for (int key = 0; key < 4; ++key) {
				const QPointF dot(unit * (0.27 + key * 0.153), unit * (0.38 + row * 0.12));
				painter.drawPoint(dot);
			}
		}
		painter.drawLine(QPointF(unit * 0.34, unit * 0.64), QPointF(unit * 0.66, unit * 0.64));
	} else {
		/*
		 * A chevron coming down to a bar: the keyboard going away, which
		 * is what every platform's dismiss control draws.
		 */
		QPainterPath path;
		path.moveTo(unit * 0.26, unit * 0.34);
		path.lineTo(unit * 0.50, unit * 0.56);
		path.lineTo(unit * 0.74, unit * 0.34);
		painter.drawPath(path);
		painter.drawLine(QPointF(unit * 0.26, unit * 0.74),
		                  QPointF(unit * 0.74, unit * 0.74));
	}

	painter.end();
	return QIcon(canvas);
}
