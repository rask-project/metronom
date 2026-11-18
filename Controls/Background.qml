import QtQuick

import Metronom

Rectangle {
    id: control

    color: Theme.backgroundEnd

    gradient: Gradient {
        GradientStop {
            position: 0.25
            color: Theme.backgroundStart
        }
        GradientStop {
            position: 1.00
            color: Theme.backgroundEnd
        }
    }
}
