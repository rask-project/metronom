import QtQuick
import QtQuick.Effects

import Metronom

Background {
    radius: Theme.dialogRadius
    opacity: 0.97
    border.color: Theme.dialogBorder
    border.width: 1

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Theme.dialogShadow
        shadowBlur: 1.0
        shadowVerticalOffset: 8
    }
}
