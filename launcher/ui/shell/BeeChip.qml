import QtQuick
import Bee.Shell

Rectangle {
    id: chip
    visible: label.length > 0
    property string label: ""
    implicitWidth: txt.implicitWidth + Theme.sp16
    implicitHeight: 22
    radius: Theme.r8
    color: Theme.surfaceRaised
    border.color: Theme.border
    Text {
        id: txt
        anchors.centerIn: parent
        text: chip.label
        color: Theme.textDim
        font.pixelSize: Theme.caption
    }
}
