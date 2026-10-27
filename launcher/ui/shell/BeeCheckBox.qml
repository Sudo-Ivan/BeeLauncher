import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

CheckBox {
    id: cb
    property string subtitle: ""
    spacing: Theme.sp12
    indicator: Rectangle {
        width: 20
        height: 20
        radius: 6
        color: cb.checked ? Theme.accent : "transparent"
        border.width: 2
        border.color: cb.checked ? Theme.accent : Theme.textFaint
        IconImage {
            anchors.centerIn: parent
            visible: cb.checked
            source: "icons/checkmark.svg"
            color: Theme.onAccent
            sourceSize: Qt.size(12, 12)
        }
    }
    contentItem: ColumnLayout {
        spacing: 0
        Text { text: cb.text; color: Theme.text; font.pixelSize: Theme.body }
        Text {
            visible: cb.subtitle.length > 0
            text: cb.subtitle
            color: Theme.textFaint
            font.pixelSize: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
