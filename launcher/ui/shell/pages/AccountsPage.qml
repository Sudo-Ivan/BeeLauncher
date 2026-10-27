import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Accounts page. Expects the ShellAccountBridge context property "accounts".
Item {
    id: page

    AddOfflineDialog { id: offlineDlg }
    AddElybyDialog { id: elybyDlg }
    MsaSignInDialog { id: msaDlg }

    BeeDialog {
        id: removeDlg
        property int row: -1
        title: qsTr("Remove account?")
        Text {
            text: qsTr("Do you really want to delete this account?")
            color: Theme.textDim
            font.pixelSize: Theme.body
        }
        actions: [
            BeeButton { text: qsTr("Cancel"); variant: "secondary"; onClicked: removeDlg.close() },
            BeeButton { text: qsTr("Remove"); variant: "danger"; onClicked: { accounts.remove(removeDlg.row); removeDlg.close() } }
        ]
    }

    BeeMenu {
        id: addMenu
        BeeMenuItem { text: qsTr("Microsoft account"); iconSource: "icons/globe.svg"; enabled: accounts.msaSupported; onTriggered: accounts.addMicrosoft() }
        BeeMenuItem { text: qsTr("Offline account"); iconSource: "icons/person.svg"; onTriggered: offlineDlg.open() }
        BeeMenuItem { text: qsTr("Ely.by account"); iconSource: "icons/world.svg"; onTriggered: accounts.addElyby("", "") }
    }

    // The bridge starts the Microsoft device code flow immediately; the dialog
    // opens as soon as the server hands back a code.
    Connections {
        target: accounts
        function onMsaCodeRequested(url, code) {
            msaDlg.url = url
            msaDlg.code = code
            if (!msaDlg.opened)
                msaDlg.open()
        }
        function onMsaFailed(reason) {
            // Surface failures that happen before the code arrives.
            if (reason.length > 0 && !msaDlg.opened) {
                msaDlg.errorText = reason
                msaDlg.open()
            }
        }
        function onElybyCodeRequested(url, code) {
            elybyDlg.url = url
            elybyDlg.code = code
            if (!elybyDlg.opened)
                elybyDlg.open()
        }
        function onElybyFailed(reason) {
            if (reason.length > 0 && !elybyDlg.opened) {
                elybyDlg.errorText = reason
                elybyDlg.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp32
        spacing: Theme.sp16

        // page header
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp12
            Text {
                text: qsTr("Signed-in accounts")
                color: Theme.text
                font.pixelSize: Theme.headline
                font.weight: Font.DemiBold
            }
            BusyIndicator {
                visible: accounts.busy
                running: visible
                implicitWidth: 24
                implicitHeight: 24
                palette.dark: Theme.accent
            }
            Item { Layout.fillWidth: true }
            BeeButton {
                variant: "secondary"
                text: qsTr("Classic dialog")
                onClicked: accounts.openClassic()
            }
            BeeButton {
                id: addBtn
                variant: "primary"
                text: qsTr("Add account")
                iconSource: "icons/person_add.svg"
                onClicked: addMenu.popup(addBtn, 0, addBtn.height + Theme.sp4)
            }
        }

        // account list
        ListView {
            id: accountList
            visible: accounts.count > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.sp8
            model: accounts
            ScrollBar.vertical: BeeScrollBar {}

            delegate: ItemDelegate {
                id: row
                width: accountList.width
                implicitHeight: 64
                contentItem: RowLayout {
                    spacing: Theme.sp12
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 20
                        color: Theme.accentAlpha(0.25)
                        Text {
                            anchors.centerIn: parent
                            text: model.profileName.length > 0 ? model.profileName.charAt(0).toUpperCase() : "?"
                            color: Theme.accent
                            font.pixelSize: Theme.bodyLarge
                            font.weight: Font.DemiBold
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: model.profileName
                            color: Theme.text
                            font.pixelSize: Theme.body
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: model.typeDisplay + " - " + model.statusText
                            color: model.statusOk ? Theme.textDim : Theme.danger
                            font.pixelSize: Theme.caption
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    BusyIndicator {
                        visible: model.isWorking
                        running: visible
                        implicitWidth: 20
                        implicitHeight: 20
                        palette.dark: Theme.accent
                    }
                    BeeChip { label: qsTr("Default"); visible: model.isDefault }
                    BeeIconButton {
                        iconSource: "icons/star.svg"
                        visible: !model.isDefault
                        Accessible.name: qsTr("Set as default")
                        onClicked: accounts.setDefault(index)
                    }
                    BeeIconButton {
                        iconSource: "icons/refresh.svg"
                        visible: model.typeId !== "offline"
                        Accessible.name: qsTr("Refresh")
                        onClicked: accounts.refresh(index)
                    }
                    BeeIconButton {
                        iconSource: "icons/delete.svg"
                        dangerItem: true
                        Accessible.name: qsTr("Remove")
                        onClicked: { removeDlg.row = index; removeDlg.open() }
                    }
                }
                background: Rectangle {
                    radius: Theme.r12
                    color: row.hovered ? Theme.surfaceHover : "transparent"
                    border.width: model.isDefault ? 1 : 0
                    border.color: Theme.accentAlpha(0.5)
                }
            }
        }

        // empty state
        ColumnLayout {
            visible: accounts.count === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.sp12
            Item { Layout.fillHeight: true }
            Rectangle {
                width: 96
                height: 96
                radius: 48
                color: Theme.surface
                Layout.alignment: Qt.AlignHCenter
                IconImage {
                    anchors.centerIn: parent
                    source: "../icons/person.svg"
                    color: Theme.textFaint
                    sourceSize: Qt.size(44, 44)
                    width: 44
                    height: 44
                }
            }
            Text {
                text: qsTr("No accounts yet")
                color: Theme.text
                font.pixelSize: Theme.title
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: qsTr("Sign in with a Microsoft account to play online, or add an offline account")
                color: Theme.textDim
                font.pixelSize: Theme.body
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 420
            }
            BeeButton {
                variant: "primary"
                text: qsTr("Add account")
                iconSource: "icons/person_add.svg"
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Theme.sp8
                onClicked: addMenu.popup(this, 0, height + Theme.sp4)
            }
            Item { Layout.fillHeight: true }
        }
    }
}
