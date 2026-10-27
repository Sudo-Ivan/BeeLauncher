import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// New instance wizard for the QML shell. Driven by the createCtrl context
// property (ShellCreateController) and runs the creation task through the
// shared taskRunner context property (ShellTaskRunner).
BeeDialog {
    id: dlg
    title: qsTr("New instance")
    width: 640
    height: 560
    closePolicy: taskRunner.busy ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

    property string loaderUid: ""
    property var loaderOptions: []
    property string errorText: ""

    function reloadLoaders() {
        var descriptor = versionList.currentIndex >= 0 ? createCtrl.versionDescriptorAt(versionList.currentIndex) : ""
        createCtrl.loadLoaders(loaderUid, descriptor)
        loaderVersionBox.currentIndex = 0
    }

    function pickDefaultVersion() {
        var row = createCtrl.recommendedVersionRow()
        if (row < 0 && versionList.count > 0)
            row = 0
        versionList.currentIndex = row
        reloadLoaders()
    }

    onOpened: {
        errorText = ""
        loaderOptions = createCtrl.loaderOptions()
        createCtrl.loadVersions()
        pickDefaultVersion()
    }

    Connections {
        target: createCtrl
        function onVersionsChanged() { dlg.pickDefaultVersion() }
        function onLoadersChanged() { loaderVersionBox.currentIndex = 0 }
        function onInstanceCreated() { dlg.close() }
        function onCreateFailed(reason) { dlg.errorText = reason }
    }

    // name + group
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp12
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.sp4
            Text { text: qsTr("Name"); color: Theme.textDim; font.pixelSize: Theme.caption }
            BeeTextField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Instance name") }
        }
        ColumnLayout {
            Layout.preferredWidth: 180
            spacing: Theme.sp4
            Text { text: qsTr("Group"); color: Theme.textDim; font.pixelSize: Theme.caption }
            BeeTextField { id: groupField; Layout.fillWidth: true; placeholderText: qsTr("Optional") }
        }
    }

    // loader selection
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4
        Text { text: qsTr("Mod loader"); color: Theme.textDim; font.pixelSize: Theme.caption }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.sp8
            Repeater {
                model: dlg.loaderOptions
                delegate: Rectangle {
                    required property var modelData
                    property bool active: dlg.loaderUid === modelData.uid
                    implicitWidth: chipLabel.implicitWidth + Theme.sp16
                    implicitHeight: 26
                    radius: Theme.r8
                    color: active ? Theme.accentAlpha(0.18) : (chipArea.containsMouse ? Theme.surfaceHover : Theme.surface)
                    border.width: 1
                    border.color: active ? Theme.accentAlpha(0.6) : Theme.border
                    Text {
                        id: chipLabel
                        anchors.centerIn: parent
                        text: modelData.name
                        color: active ? Theme.accent : Theme.textDim
                        font.pixelSize: Theme.caption
                        font.weight: active ? Font.DemiBold : Font.Medium
                    }
                    MouseArea {
                        id: chipArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            dlg.loaderUid = modelData.uid
                            dlg.reloadLoaders()
                        }
                    }
                }
            }
        }
    }

    // version type filter + refresh
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4
        Text { text: qsTr("Version"); color: Theme.textDim; font.pixelSize: Theme.caption }
        Item { Layout.fillWidth: true }
        BeeTabButton { text: qsTr("Releases"); active: createCtrl.typeFilter === "release"; onActivated: createCtrl.setTypeFilter("release") }
        BeeTabButton { text: qsTr("Snapshots"); active: createCtrl.typeFilter === "snapshot"; onActivated: createCtrl.setTypeFilter("snapshot") }
        BeeTabButton { text: qsTr("All"); active: createCtrl.typeFilter === "all"; onActivated: createCtrl.setTypeFilter("all") }
        BeeIconButton { iconSource: "icons/refresh.svg"; onClicked: createCtrl.reload(true) }
    }

    // version list
    // BeeDialog sizes itself from the content column's implicit size, so the
    // list carries the dialog's target inner size (640 - 2 * sp24 margins).
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 592
        Layout.preferredHeight: 210
        radius: Theme.r12
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        ListView {
            id: versionList
            anchors.fill: parent
            anchors.margins: Theme.sp4
            clip: true
            model: createCtrl.versionModel
            ScrollBar.vertical: BeeScrollBar {}
            delegate: BeeListDelegate {
                width: ListView.view.width
                title: model.descriptor
                selected: ListView.isCurrentItem
                onClicked: {
                    versionList.currentIndex = index
                    dlg.reloadLoaders()
                }
                BeeChip { label: model.type }
                BeeChip { visible: model.recommended === true; label: qsTr("Recommended") }
            }
        }
        BusyIndicator {
            anchors.centerIn: parent
            running: createCtrl.versionsBusy
            visible: running
        }
        Text {
            anchors.centerIn: parent
            visible: versionList.count === 0 && !createCtrl.versionsBusy
            text: qsTr("No versions loaded")
            color: Theme.textFaint
            font.pixelSize: Theme.body
        }
    }

    // loader version
    ColumnLayout {
        visible: dlg.loaderUid.length > 0
        Layout.fillWidth: true
        spacing: Theme.sp4
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp8
            Text { text: qsTr("Loader version"); color: Theme.textDim; font.pixelSize: Theme.caption }
            BusyIndicator {
                running: createCtrl.loadersBusy
                visible: running
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }
        }
        BeeComboBox {
            id: loaderVersionBox
            Layout.fillWidth: true
            model: createCtrl.loaderModel
            textRole: "descriptor"
            delegate: ItemDelegate {
                width: loaderVersionBox.width
                height: 36
                contentItem: Text {
                    text: model.descriptor + (model.recommended ? "  " + qsTr("(recommended)") : "")
                    color: Theme.text
                    font.pixelSize: Theme.body
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { radius: Theme.r8; color: highlighted ? Theme.surfaceHover : "transparent" }
                onClicked: {
                    loaderVersionBox.currentIndex = index
                    loaderVersionBox.popup.close()
                }
            }
        }
        Text {
            visible: !createCtrl.loadersBusy && createCtrl.loaderModel.count === 0
            text: qsTr("No loader versions available for this Minecraft version")
            color: Theme.textFaint
            font.pixelSize: Theme.caption
        }
    }

    // error line
    Text {
        visible: dlg.errorText.length > 0
        text: dlg.errorText
        color: Theme.danger
        font.pixelSize: Theme.caption
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    // task progress
    RowLayout {
        visible: taskRunner.busy
        Layout.fillWidth: true
        spacing: Theme.sp12
        ProgressBar {
            Layout.fillWidth: true
            indeterminate: taskRunner.progress < 0
            value: taskRunner.progress >= 0 ? taskRunner.progress : 0
        }
        Text {
            text: taskRunner.statusText
            color: Theme.textDim
            font.pixelSize: Theme.caption
            elide: Text.ElideRight
            Layout.maximumWidth: 260
        }
        BeeButton {
            variant: "ghost"
            btnHeight: 28
            text: qsTr("Abort")
            onClicked: taskRunner.abort()
        }
    }

    actions: [
        BeeButton {
            variant: "ghost"
            text: qsTr("Classic wizard")
            onClicked: createCtrl.openClassicWizard()
        },
        Item { width: Theme.sp24 },
        BeeButton {
            variant: "secondary"
            text: qsTr("Cancel")
            onClicked: dlg.close()
        },
        BeeButton {
            variant: "primary"
            text: qsTr("Create")
            opacity: enabled ? 1.0 : 0.5
            enabled: nameField.text.trim().length > 0
                     && versionList.currentIndex >= 0
                     && !taskRunner.busy
                     && (dlg.loaderUid.length === 0 || loaderVersionBox.currentIndex >= 0)
            onClicked: {
                dlg.errorText = ""
                var loaderDescriptor = dlg.loaderUid.length > 0
                        ? createCtrl.loaderDescriptorAt(loaderVersionBox.currentIndex)
                        : ""
                createCtrl.create(nameField.text, groupField.text,
                                  createCtrl.versionDescriptorAt(versionList.currentIndex),
                                  dlg.loaderUid, loaderDescriptor)
            }
        }
    ]
}
