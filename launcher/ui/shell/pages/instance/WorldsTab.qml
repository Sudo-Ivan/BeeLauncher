import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Saved worlds list. Worlds cannot be toggled, only inspected and deleted,
// so the checkbox column from ModsTab is replaced by game mode info.
Item {
    id: root
    property var bridge: null

    function formatTime(ms) {
        if (!ms)
            return qsTr("Never played")
        var d = new Date(ms)
        return d.toLocaleDateString() + " " + d.toLocaleTimeString()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.sp16
            spacing: Theme.sp8
            Text {
                text: qsTr("Saved worlds")
                color: Theme.text
                font.pixelSize: Theme.bodyLarge
                font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
            BeeButton {
                variant: "secondary"
                btnHeight: 36
                text: qsTr("Open folder")
                iconSource: "icons/folder.svg"
                onClicked: if (root.bridge && root.bridge.worldsModel) root.bridge.worldsModel.openFolder()
            }
            BeeIconButton {
                iconSource: "icons/refresh.svg"
                Accessible.name: qsTr("Refresh")
                onClicked: if (root.bridge && root.bridge.worldsModel) root.bridge.worldsModel.refresh()
            }
        }

        ListView {
            id: worldList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.sp16
            Layout.rightMargin: Theme.sp16
            clip: true
            spacing: 2
            model: root.bridge ? root.bridge.worldsModel : null
            visible: count !== 0
            ScrollBar.vertical: BeeScrollBar {}

            delegate: ItemDelegate {
                id: del
                width: worldList.width
                implicitHeight: 56
                contentItem: RowLayout {
                    spacing: Theme.sp12
                    Rectangle {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        radius: Theme.r8
                        color: Theme.surfaceHover
                        IconImage {
                            anchors.centerIn: parent
                            source: "../../icons/world.svg"
                            color: Theme.textDim
                            sourceSize: Qt.size(20, 20)
                        }
                    }
                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true
                        Text {
                            text: model.name
                            color: Theme.text
                            font.pixelSize: Theme.body
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: {
                                var parts = []
                                if (model.gameMode !== undefined && String(model.gameMode).length)
                                    parts.push(model.gameMode)
                                if (model.sizeText !== undefined && String(model.sizeText).length)
                                    parts.push(model.sizeText)
                                parts.push(root.formatTime(model.lastModified))
                                if (model.seed !== undefined && String(model.seed).length)
                                    parts.push(qsTr("seed %1").arg(model.seed))
                                if (model.info !== undefined && String(model.info).length)
                                    parts.push(model.info)
                                return parts.join("   ")
                            }
                            color: Theme.textDim
                            font.pixelSize: Theme.caption
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    BeeIconButton {
                        iconSource: "icons/more_horizontal.svg"
                        onClicked: rowMenu.popup()
                        BeeMenu {
                            id: rowMenu
                            BeeMenuItem {
                                text: qsTr("Open worlds folder")
                                iconSource: "icons/folder.svg"
                                onTriggered: if (worldList.model) worldList.model.openFolder()
                            }
                            BeeMenuItem {
                                text: qsTr("Delete")
                                iconSource: "icons/delete.svg"
                                dangerItem: true
                                enabled: worldList.model !== null && worldList.model.canDelete
                                onTriggered: {
                                    confirmDelete.targetRow = index
                                    confirmDelete.targetName = model.name
                                    confirmDelete.open()
                                }
                            }
                        }
                    }
                }
                background: Rectangle {
                    radius: Theme.r12
                    color: del.hovered ? Theme.surfaceHover : "transparent"
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !worldList.model || worldList.count === 0
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.sp8
            IconImage {
                source: "../../icons/world.svg"
                color: Theme.textFaint
                sourceSize: Qt.size(40, 40)
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: root.bridge && root.bridge.isMinecraft ? qsTr("No saved worlds") : qsTr("No worlds for this instance type")
                color: Theme.textDim
                font.pixelSize: Theme.body
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    BeeDialog {
        id: confirmDelete
        property int targetRow: -1
        property string targetName: ""
        title: qsTr("Delete world")
        Text {
            text: qsTr("Delete \"%1\"? The world folder is removed permanently.").arg(confirmDelete.targetName)
            color: Theme.textDim
            font.pixelSize: Theme.body
            wrapMode: Text.WordWrap
            Layout.preferredWidth: 360
        }
        actions: [
            BeeButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: confirmDelete.close() },
            BeeButton {
                text: qsTr("Delete")
                variant: "danger"
                onClicked: {
                    confirmDelete.close()
                    if (worldList.model && confirmDelete.targetRow >= 0)
                        worldList.model.remove(confirmDelete.targetRow)
                }
            }
        ]
    }
}
