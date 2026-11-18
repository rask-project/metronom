#pragma once

#include <QColor>
#include <QWindow>

namespace MacWindowEffects
{
// Hidden title, unified empty toolbar, and a translucent blurred background under the Qt content.
// The appearance follows the app theme, not the system one, so the blur matches the content colors.
void applyTranslucentWindow(WId winId, const QColor &color, qreal alpha, bool dark);
// Right edge of the close/minimize/zoom buttons in the window's content coordinates.
qreal windowButtonsTrailingEdge(WId winId);
// Qt keeps every window resizable and relies on content size limits, which the expanded title bar escapes.
void setWindowResizable(WId winId, bool resizable);
} // namespace MacWindowEffects
