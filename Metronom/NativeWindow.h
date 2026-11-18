#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

// Translucent, blurred window with the content extended under the title bar (macOS only).
class NativeWindow : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(bool translucent READ translucent CONSTANT)
    Q_PROPERTY(qreal leadingInset READ leadingInset NOTIFY leadingInsetChanged)

public:
    explicit NativeWindow(QObject *parent = nullptr);

    QWindow *window() const;
    void setWindow(QWindow *window);

    QColor color() const;
    void setColor(const QColor &color);

    qreal alpha() const noexcept;
    void setAlpha(qreal alpha);

    bool dark() const noexcept;
    void setDark(bool dark);

    bool translucent() const noexcept;

    qreal leadingInset() const noexcept;

signals:
    void windowChanged();
    void colorChanged();
    void alphaChanged();
    void darkChanged();
    void leadingInsetChanged();

private:
    void apply();

    QPointer<QWindow> m_window;
    qreal m_alpha = 0.8;
    qreal m_leadingInset = 0.0;
    QColor m_color = Qt::black;
    bool m_dark = false;
};
