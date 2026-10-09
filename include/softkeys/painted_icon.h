#ifndef SK_PAINTED_ICON_H
#define SK_PAINTED_ICON_H

#include <QColor>
#include <QIcon>

/* The icons softkeys draws for itself; see src/painted_icon.cpp. */
enum sk_icon_t {
	SK_ICON_SETTINGS,
	SK_ICON_HIDE,

	/* Switch keyboards: the system one to this one, or back. */
	SK_ICON_KEYBOARD
};

/* `size` is the pixmap's side in pixels; the ink follows the palette. */
QIcon sk_painted_icon(sk_icon_t icon, const QColor &ink, int size);

#endif /* SK_PAINTED_ICON_H */
