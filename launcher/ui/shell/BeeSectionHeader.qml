import QtQuick
import QtQuick.Layouts
import Bee.Shell

// Small caps section header used to group form rows.
ColumnLayout {
    property string text: ""
    spacing: Theme.sp4
    Layout.fillWidth: true
    Text {
        text: parent.text.toUpperCase()
        color: Theme.accent
        font.pixelSize: Theme.caption
        font.weight: Font.DemiBold
        font.letterSpacing: 1.2
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
}
