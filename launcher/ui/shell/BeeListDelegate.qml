import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Generic list row with leading icon area, primary + secondary text, and a
// trailing slot for extra controls.
ItemDelegate {
    id: del
    property string title: ""
    property string subtitle: ""
    property string iconSource: ""
    property bool selected: false
    default property alias trailing: trail.data
    implicitHeight: Math.max(56, delCol.implicitHeight + Theme.sp16)
    contentItem: RowLayout {
        spacing: Theme.sp12
        Rectangle {
            visible: del.iconSource.length > 0
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            radius: Theme.r8
            color: Theme.surfaceHover
            IconImage {
                anchors.centerIn: parent
                source: del.iconSource
                color: Theme.textDim
                sourceSize: Qt.size(20, 20)
            }
        }
        ColumnLayout {
            id: delCol
            spacing: 0
            Layout.fillWidth: true
            Text {
                text: del.title
                color: Theme.text
                font.pixelSize: Theme.body
                font.weight: Font.Medium
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                visible: del.subtitle.length > 0
                text: del.subtitle
                color: Theme.textDim
                font.pixelSize: Theme.caption
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
        RowLayout { id: trail; spacing: Theme.sp8 }
    }
    background: Rectangle {
        radius: Theme.r12
        color: del.selected ? Theme.accentAlpha(0.15) : (del.hovered ? Theme.surfaceHover : "transparent")
        border.width: del.selected ? 1 : 0
        border.color: Theme.accentAlpha(0.5)
    }
}
