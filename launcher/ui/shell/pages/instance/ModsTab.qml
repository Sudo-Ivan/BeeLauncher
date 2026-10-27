import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Resource list tab shared by mods, core mods and the pack folders. A combo
// switches the backing ShellResourceModel; rows support enable toggling,
// search filtering and removal with a confirmation dialog.
Item {
    id: root
    property var bridge: null
    property int packIndex: 0
    property string filter: ""

    function activeModel() {
        if (!root.bridge)
            return null
        var models = [root.bridge.modsModel, root.bridge.coreModsModel, root.bridge.resourcePacksModel,
                      root.bridge.texturePacksModel, root.bridge.shaderPacksModel, root.bridge.dataPacksModel]
        return models[root.packIndex]
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // toolbar
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.sp16
            spacing: Theme.sp8

            BeeComboBox {
                Layout.preferredWidth: 200
                model: [qsTr("Loader mods"), qsTr("Core mods"), qsTr("Resource packs"),
                        qsTr("Texture packs"), qsTr("Shader packs"), qsTr("Data packs")]
                currentIndex: root.packIndex
                onActivated: function(i) { root.packIndex = i }
            }
            BeeTextField {
                Layout.preferredWidth: 280
                placeholderText: qsTr("Search")
                onTextChanged: root.filter = text
            }
            Item { Layout.fillWidth: true }
            BeeButton {
                variant: "secondary"
                btnHeight: 36
                text: qsTr("Open folder")
                iconSource: "icons/folder.svg"
                onClicked: { var m = root.activeModel(); if (m) m.openFolder() }
            }
            BeeIconButton {
                iconSource: "icons/refresh.svg"
                Accessible.name: qsTr("Refresh")
                onClicked: { var m = root.activeModel(); if (m) m.refresh() }
            }
        }

        ListView {
            id: modList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.sp16
            Layout.rightMargin: Theme.sp16
            clip: true
            spacing: 2
            model: root.activeModel()
            visible: count !== 0
            ScrollBar.vertical: BeeScrollBar {}

            delegate: ItemDelegate {
                id: del
                width: modList.width
                implicitHeight: 56
                readonly property bool matches: root.filter.length === 0
                    || String(model.name).toLowerCase().indexOf(root.filter.toLowerCase()) >= 0
                visible: matches
                height: matches ? implicitHeight : 0

                contentItem: RowLayout {
                    spacing: Theme.sp12
                    BeeCheckBox {
                        id: enBox
                        visible: modList.model !== null && modList.model.canToggle
                        text: ""
                        checked: model.enabled === undefined ? true : model.enabled
                        onToggled: {
                            var want = checked
                            // restore the live binding, then apply to the source
                            checked = Qt.binding(function() { return model.enabled })
                            if (modList.model)
                                modList.model.setEnabled(index, want)
                        }
                    }
                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true
                        Text {
                            text: model.name
                            color: model.enabled === false ? Theme.textFaint : Theme.text
                            font.pixelSize: Theme.body
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: {
                                var parts = []
                                if (model.info !== undefined && String(model.info).length)
                                    parts.push(model.info)
                                if (model.fileName !== undefined && String(model.fileName).length)
                                    parts.push(model.fileName)
                                if (model.sizeText !== undefined && String(model.sizeText).length)
                                    parts.push(model.sizeText)
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
                                text: qsTr("Open folder")
                                iconSource: "icons/folder.svg"
                                onTriggered: if (modList.model) modList.model.openFolder()
                            }
                            BeeMenuItem {
                                text: qsTr("Remove")
                                iconSource: "icons/delete.svg"
                                dangerItem: true
                                enabled: modList.model !== null && modList.model.canDelete
                                onTriggered: {
                                    confirmRemove.targetRow = index
                                    confirmRemove.targetName = model.fileName !== undefined ? model.fileName : model.name
                                    confirmRemove.open()
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

        // empty state
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: modList.count === 0
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.sp8
            IconImage {
                source: "../../icons/puzzle.svg"
                color: Theme.textFaint
                sourceSize: Qt.size(40, 40)
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: root.bridge && root.bridge.isMinecraft ? qsTr("Nothing here yet") : qsTr("No content lists for this instance type")
                color: Theme.textDim
                font.pixelSize: Theme.body
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    BeeDialog {
        id: confirmRemove
        property int targetRow: -1
        property string targetName: ""
        title: qsTr("Remove file")
        Text {
            text: qsTr("Remove \"%1\"? This deletes the file from disk.").arg(confirmRemove.targetName)
            color: Theme.textDim
            font.pixelSize: Theme.body
            wrapMode: Text.WordWrap
            Layout.preferredWidth: 360
        }
        actions: [
            BeeButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: confirmRemove.close() },
            BeeButton {
                text: qsTr("Remove")
                variant: "danger"
                onClicked: {
                    confirmRemove.close()
                    if (modList.model && confirmRemove.targetRow >= 0)
                        modList.model.remove(confirmRemove.targetRow)
                }
            }
        ]
    }
}
