#include "NativeWindow.h"

#include <QTimer>

#ifdef Q_OS_MACOS
#include "MacWindowEffects.h"
#endif

NativeWindow::NativeWindow(QObject *parent) : QObject(parent) {}

QWindow *NativeWindow::window() const
{
    return m_window;
}

void NativeWindow::setWindow(QWindow *window)
{
    if (m_window == window) {
        return;
    }
    if (m_window != nullptr) {
        disconnect(m_window, nullptr, this, nullptr);
    }
    m_window = window;
    emit windowChanged();

    if (m_window == nullptr) {
        return;
    }
    // Hiding the window drops its NSWindow; the one created on show starts unstyled.
    connect(m_window, &QWindow::visibleChanged, this, [this](bool visible) {
        if (visible) {
            QTimer::singleShot(0, this, &NativeWindow::apply);
        }
    });
    // Deferred so the QML window finishes setting its flags before winId() creates the native window.
    QTimer::singleShot(0, this, &NativeWindow::apply);
}

QColor NativeWindow::color() const
{
    return m_color;
}

void NativeWindow::setColor(const QColor &color)
{
    if (m_color == color) {
        return;
    }
    m_color = color;
    emit colorChanged();
    apply();
}

qreal NativeWindow::alpha() const noexcept
{
    return m_alpha;
}

void NativeWindow::setAlpha(qreal alpha)
{
    if (qFuzzyCompare(m_alpha, alpha)) {
        return;
    }
    m_alpha = alpha;
    emit alphaChanged();
    apply();
}

bool NativeWindow::dark() const noexcept
{
    return m_dark;
}

void NativeWindow::setDark(bool dark)
{
    if (m_dark == dark) {
        return;
    }
    m_dark = dark;
    emit darkChanged();
    apply();
}

bool NativeWindow::translucent() const noexcept
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

qreal NativeWindow::leadingInset() const noexcept
{
    return m_leadingInset;
}

void NativeWindow::apply()
{
#ifdef Q_OS_MACOS
    if (m_window == nullptr) {
        return;
    }
    const WId winId = m_window->winId();
    MacWindowEffects::applyTranslucentWindow(winId, m_color, m_alpha, m_dark);
    MacWindowEffects::setWindowResizable(winId, m_window->minimumSize() != m_window->maximumSize());

    const qreal inset = MacWindowEffects::windowButtonsTrailingEdge(winId);
    if (qFuzzyCompare(m_leadingInset, inset)) {
        return;
    }
    m_leadingInset = inset;
    emit leadingInsetChanged();
#endif
}
