import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

MenuItem {
    id: mi
    property string iconSource: ""
    property bool dangerItem: false
    implicitHeight: 36
    Accessible.role: Accessible.MenuItem
    contentItem: RowLayout {
        spacing: Theme.sp12
        IconImage {
            visible: mi.iconSource.length > 0
            source: mi.iconSource
            color: mi.dangerItem ? Theme.danger : Theme.textDim
            sourceSize: Qt.size(18, 18)
            Layout.preferredWidth: 18; Layout.preferredHeight: 18
        }
        Text { text: mi.text; color: mi.dangerItem ? Theme.danger : Theme.text; font.pixelSize: Theme.body; Layout.fillWidth: true }
    }
    background: Rectangle { radius: Theme.r8; color: mi.highlighted ? Theme.surfaceHover : "transparent" }
}
