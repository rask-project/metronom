#include "StatusBarController.h"

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#include <QtCore/private/qjnihelpers_p.h>
#endif

StatusBarController::StatusBarController(QObject *parent) : QObject(parent) {}

bool StatusBarController::lightStatusBar() const
{
    return m_lightStatusBar;
}

void StatusBarController::setLightStatusBar(bool light)
{
    if (m_lightStatusBar == light) {
        return;
    }
    m_lightStatusBar = light;
    emit lightStatusBarChanged();

#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([light]() {
        const QJniObject activity = QtAndroidPrivate::activity();
        const QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) {
            return;
        }

        const int sdk = QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
        if (sdk >= 30) {
            const QJniObject controller =
                window.callObjectMethod("getInsetsController", "()Landroid/view/WindowInsetsController;");
            if (!controller.isValid()) {
                return;
            }
            const jint lightBars =
                QJniObject::getStaticField<jint>("android/view/WindowInsetsController", "APPEARANCE_LIGHT_STATUS_BARS");
            controller.callMethod<void>("setSystemBarsAppearance", "(II)V", light ? lightBars : 0, lightBars);
            return;
        }

        const QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        if (!decorView.isValid()) {
            qCritical() << "Failed to get decor view";
            return;
        }
        const jint lightFlag = QJniObject::getStaticField<jint>("android/view/View", "SYSTEM_UI_FLAG_LIGHT_STATUS_BAR");
        const jint visibility = decorView.callMethod<jint>("getSystemUiVisibility", "()I");
        decorView.callMethod<void>(
            "setSystemUiVisibility",
            "(I)V",
            light ? (visibility | lightFlag) : (visibility & ~lightFlag)
        );
    });
#endif
}
