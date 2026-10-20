import QtQuick
import QtQuick.Controls.Basic
import Bee.Shell

ScrollBar {
    orientation: Qt.Vertical
    policy: ScrollBar.AsNeeded
    contentItem: Rectangle {
        implicitWidth: 6
        radius: 3
        color: parent.pressed ? Theme.textDim : (parent.hovered ? Theme.textDim : Theme.border)
    }
    background: null
}
