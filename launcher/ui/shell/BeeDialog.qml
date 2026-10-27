import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Shared modal dialog. Set title, put controls in the default content area,
// and put BeeButton instances in the actions list for the footer.
Popup {
    id: dlg
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape
    property string title: ""
    property alias actions: actionRow.children
    default property alias content: contentCol.data

    anchors.centerIn: parent
    padding: 0
    width: implicitWidth
    height: implicitHeight
    implicitWidth: contentLayout.implicitWidth + Theme.sp24 * 2
    implicitHeight: contentLayout.implicitHeight + Theme.sp24 * 2
    Overlay.modal: Rectangle { color: Theme.shadow(0.45) }
    background: Rectangle {
        radius: Theme.r16
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.border
    }
    contentItem: ColumnLayout {
        id: contentLayout
        spacing: Theme.sp16
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp24
            Layout.leftMargin: Theme.sp24
            Layout.rightMargin: Theme.sp24
            Text { text: dlg.title; color: Theme.text; font.pixelSize: Theme.title; font.weight: Font.DemiBold; Layout.fillWidth: true }
            BeeIconButton { iconSource: "icons/dismiss.svg"; onClicked: dlg.close() }
        }
        ColumnLayout {
            id: contentCol
            Layout.leftMargin: Theme.sp24
            Layout.rightMargin: Theme.sp24
            Layout.bottomMargin: actionRow.children.length ? 0 : Theme.sp24
            spacing: Theme.sp12
        }
        RowLayout {
            id: actionRow
            visible: children.length > 0
            Layout.alignment: Qt.AlignRight
            Layout.bottomMargin: Theme.sp24
            spacing: Theme.sp8
        }
    }
}
