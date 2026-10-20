import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import Bee.Shell

ToolButton {
    id: ib
    property string iconSource: ""
    property bool dangerItem: false
    implicitWidth: 36
    implicitHeight: 36
    Accessible.role: Accessible.Button
    contentItem: IconImage {
        source: ib.iconSource
        color: ib.dangerItem ? Theme.danger : (ib.down || ib.hovered ? Theme.text : Theme.textDim)
        sourceSize: Qt.size(20, 20)
        anchors.centerIn: parent
        width: 20; height: 20
    }
    background: Rectangle {
        radius: Theme.r8
        color: ib.down ? Theme.surfaceRaised : (ib.hovered ? Theme.surfaceHover : "transparent")
    }
    Rectangle {
        visible: ib.activeFocus
        anchors.fill: parent; anchors.margins: -3
        radius: Theme.r8 + 3; color: "transparent"
        border.width: 2; border.color: Theme.accentAlpha(0.6)
    }
}
