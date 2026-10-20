import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

ComboBox {
    id: cb
    implicitHeight: 40
    leftPadding: Theme.sp12
    rightPadding: Theme.sp12
    model: [qsTr("Name"), qsTr("Last played"), qsTr("Play time")]
    indicator: Item { width: 0 }
    Accessible.role: Accessible.ComboBox
    contentItem: RowLayout {
        spacing: Theme.sp8
        IconImage { source: "icons/arrow_sort.svg"; color: Theme.textDim; sourceSize: Qt.size(16,16); Layout.preferredWidth:16; Layout.preferredHeight:16 }
        Text { text: cb.displayText; color: Theme.text; font.pixelSize: Theme.body; Layout.fillWidth: true }
        IconImage { source: "icons/chevron_down.svg"; color: Theme.textDim; sourceSize: Qt.size(14,14) }
    }
    background: Rectangle {
        radius: Theme.r12
        color: cb.hovered ? Theme.surfaceHover : Theme.surface
        border.width: 1
        border.color: cb.activeFocus ? Theme.accentAlpha(0.6) : Theme.border
    }
    popup: Popup {
        y: cb.height + 4
        width: cb.width
        padding: Theme.sp8
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: cb.popup.visible ? cb.delegateModel : null
        }
        background: Rectangle { radius: Theme.r12; color: Theme.surfaceRaised; border.color: Theme.border }
    }
    delegate: ItemDelegate {
        width: cb.width
        height: 36
        contentItem: Text { text: modelData; color: Theme.text; font.pixelSize: Theme.body; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: Theme.r8; color: highlighted ? Theme.surfaceHover : "transparent" }
    }
}
