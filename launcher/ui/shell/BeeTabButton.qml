import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Horizontal tab selector used for in-page tabs.
ItemDelegate {
    id: tab
    property bool active: false
    property string iconSource: ""
    signal activated()
    implicitHeight: 40
    implicitWidth: tabRow.implicitWidth + Theme.sp16 * 2
    contentItem: RowLayout {
        id: tabRow
        spacing: Theme.sp8
        IconImage {
            visible: tab.iconSource.length > 0
            source: tab.iconSource
            color: tab.active ? Theme.accent : Theme.textDim
            sourceSize: Qt.size(16, 16)
        }
        Text {
            text: tab.text
            color: tab.active ? Theme.text : Theme.textDim
            font.pixelSize: Theme.body
            font.weight: tab.active ? Font.DemiBold : Font.Medium
        }
    }
    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: Theme.r8
            color: tab.hovered && !tab.active ? Theme.surfaceHover : "transparent"
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            radius: 1
            visible: tab.active
            color: Theme.accent
        }
    }
    onClicked: tab.activated()
}
