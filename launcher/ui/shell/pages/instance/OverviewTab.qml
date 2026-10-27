import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Instance summary: details card, content counts with quick links, and a
// notes preview that jumps to the Notes tab.
Item {
    id: root
    property var bridge: null
    signal gotoTab(int index)

    component Card: Rectangle {
        default property alias rows: cardCol.data
        Layout.fillWidth: true
        radius: Theme.r16
        color: Theme.surface
        border.color: Theme.border
        implicitHeight: cardCol.implicitHeight + Theme.sp32
        ColumnLayout {
            id: cardCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.sp16
            spacing: Theme.sp8
        }
    }
    component InfoRow: RowLayout {
        id: infoRow
        property string label: ""
        property string value: ""
        spacing: Theme.sp12
        Layout.fillWidth: true
        Text { text: infoRow.label; color: Theme.textFaint; font.pixelSize: Theme.caption; Layout.preferredWidth: 110 }
        Text { text: infoRow.value; color: Theme.text; font.pixelSize: Theme.body; elide: Text.ElideMiddle; Layout.fillWidth: true }
    }
    component LinkButton: BeeButton {
        Layout.fillWidth: true
        variant: "ghost"
        btnHeight: 36
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: col.implicitHeight + Theme.sp32 * 2
        ScrollBar.vertical: BeeScrollBar {}

        ColumnLayout {
            id: col
            x: Theme.sp32
            width: parent.width - Theme.sp32 * 2
            spacing: Theme.sp16
            Item { width: 1; height: Theme.sp16 }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width > 900 ? 2 : 1
                columnSpacing: Theme.sp16
                rowSpacing: Theme.sp16

                Card {
                    Layout.alignment: Qt.AlignTop
                    BeeSectionHeader { text: qsTr("Details") }
                    InfoRow { label: qsTr("Type"); value: root.bridge ? root.bridge.typeDisplay : "" }
                    InfoRow { label: qsTr("Version"); value: root.bridge ? root.bridge.versionText : "" }
                    InfoRow {
                        label: qsTr("Instance folder")
                        value: root.bridge ? root.bridge.instanceRoot : ""
                        BeeIconButton {
                            iconSource: "icons/folder.svg"
                            onClicked: if (root.bridge) root.bridge.openFolder()
                        }
                    }
                    InfoRow {
                        label: qsTr("Game folder")
                        value: root.bridge ? root.bridge.gameRoot : ""
                        BeeIconButton {
                            iconSource: "icons/folder.svg"
                            onClicked: if (root.bridge) root.bridge.openGameFolder()
                        }
                    }
                }

                Card {
                    Layout.alignment: Qt.AlignTop
                    visible: root.bridge === null || root.bridge.isMinecraft
                    BeeSectionHeader { text: qsTr("Content") }
                    InfoRow { label: qsTr("Mods"); value: root.bridge && root.bridge.modsModel ? root.bridge.modsModel.count : 0 }
                    InfoRow { label: qsTr("Worlds"); value: root.bridge && root.bridge.worldsModel ? root.bridge.worldsModel.count : 0 }
                    RowLayout {
                        spacing: Theme.sp8
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.sp8
                        LinkButton { text: qsTr("Mods folder"); iconSource: "icons/puzzle.svg"; onClicked: if (root.bridge && root.bridge.modsModel) root.bridge.modsModel.openFolder() }
                        LinkButton { text: qsTr("Screenshots"); iconSource: "icons/image.svg"; onClicked: if (root.bridge) root.bridge.openScreenshotsFolder() }
                    }
                    RowLayout {
                        spacing: Theme.sp8
                        Layout.fillWidth: true
                        LinkButton { text: qsTr("Other logs"); iconSource: "icons/log.svg"; onClicked: if (root.bridge) root.bridge.openClassicPage("logs") }
                        LinkButton { text: qsTr("Classic settings"); iconSource: "icons/settings.svg"; onClicked: if (root.bridge) root.bridge.openClassicPage("settings") }
                    }
                }
            }

            Card {
                BeeSectionHeader { text: qsTr("Notes") }
                Text {
                    Layout.fillWidth: true
                    text: {
                        if (!root.bridge || !root.bridge.notes.length)
                            return qsTr("No notes yet.")
                        var lines = root.bridge.notes.split("\n")
                        return lines.slice(0, 5).join("\n")
                    }
                    color: root.bridge && root.bridge.notes.length ? Theme.text : Theme.textFaint
                    font.pixelSize: Theme.body
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 5
                }
                BeeButton {
                    variant: "ghost"
                    btnHeight: 32
                    text: qsTr("Edit notes")
                    iconSource: "icons/note.svg"
                    onClicked: root.gotoTab(4)
                }
            }
            Item { width: 1; height: Theme.sp16 }
        }
    }
}
