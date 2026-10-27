import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// Form row: title and optional subtitle on the left, control on the right.
RowLayout {
    id: row
    property string label: ""
    property string subtitle: ""
    default property alias control: holder.data
    spacing: Theme.sp16
    Layout.fillWidth: true

    ColumnLayout {
        spacing: 2
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        Text {
            text: row.label
            color: Theme.text
            font.pixelSize: Theme.body
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            visible: row.subtitle.length > 0
            text: row.subtitle
            color: Theme.textFaint
            font.pixelSize: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
    RowLayout {
        id: holder
        spacing: Theme.sp8
        Layout.alignment: Qt.AlignVCenter
    }
}
