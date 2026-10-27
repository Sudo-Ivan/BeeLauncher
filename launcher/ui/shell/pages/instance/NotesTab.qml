import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Free-form notes editor. Writes through to BaseInstance::setNotes on every
// edit and re-syncs from the bridge when the instance changes, unless the
// editor is focused.
Item {
    id: root
    property var bridge: null
    property bool syncing: false

    function sync() {
        root.syncing = true
        edit.text = root.bridge ? root.bridge.notes : ""
        root.syncing = false
    }
    Component.onCompleted: sync()
    Connections {
        target: root.bridge
        function onInstanceChanged() { if (!edit.activeFocus) root.sync() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp24
        spacing: Theme.sp8

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Notes")
                color: Theme.text
                font.pixelSize: Theme.bodyLarge
                font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("Saved automatically")
                color: Theme.textFaint
                font.pixelSize: Theme.caption
            }
        }

        TextArea.flickable: TextArea {
            id: edit
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: TextEdit.Wrap
            font.pixelSize: Theme.body
            color: Theme.text
            selectionColor: Theme.accent
            selectedTextColor: Theme.onAccent
            placeholderText: qsTr("Write notes about this instance")
            placeholderTextColor: Theme.textFaint
            ScrollBar.vertical: BeeScrollBar {}
            background: Rectangle {
                radius: Theme.r12
                color: Theme.surface
                border.width: edit.activeFocus ? 2 : 1
                border.color: edit.activeFocus ? Theme.accentAlpha(0.6) : Theme.border
            }
            onTextChanged: if (!root.syncing && root.bridge) root.bridge.notes = text
        }
    }
}
