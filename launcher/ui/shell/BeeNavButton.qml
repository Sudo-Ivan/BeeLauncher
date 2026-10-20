import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

Item {
    id: nav
    property string text: ""
    property string iconSource: ""
    property bool active: false
    property bool collapsed: false
    implicitHeight: 48
    Accessible.name: nav.text
    Accessible.role: Accessible.Button
    signal activated()

    Rectangle {
        anchors.fill: parent
        anchors.margins: Theme.sp8
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        radius: Theme.r12
        color: nav.active ? Theme.accentAlpha(0.14) : (area.containsMouse ? Theme.surfaceHover : "transparent")
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 22
        spacing: Theme.sp12
        IconImage {
            source: nav.iconSource
            color: nav.active ? Theme.accent : Theme.textDim
            sourceSize: Qt.size(22, 22)
            Layout.preferredWidth: 22; Layout.preferredHeight: 22
        }
        Text {
            visible: !nav.collapsed && nav.width > 110
            text: nav.text
            color: nav.active ? Theme.accent : Theme.text
            font.pixelSize: Theme.body
            font.weight: nav.active ? Font.DemiBold : Font.Normal
            Layout.fillWidth: true
        }
    }
    MouseArea { id: area; anchors.fill: parent; hoverEnabled: true; onClicked: nav.activated() }
}
