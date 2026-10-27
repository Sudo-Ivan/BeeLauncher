import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// Prompts for a username and creates an offline account.
BeeDialog {
    id: dlg
    title: qsTr("Add offline account")

    Text {
        text: qsTr("Choose a username for the new offline account.")
        color: Theme.textDim
        font.pixelSize: Theme.body
    }
    BeeTextField {
        id: nameField
        placeholderText: qsTr("Username")
        Layout.preferredWidth: 320
        onAccepted: if (addBtn.enabled) addBtn.clicked()
    }

    actions: [
        BeeButton { text: qsTr("Cancel"); variant: "secondary"; onClicked: dlg.close() },
        BeeButton {
            id: addBtn
            text: qsTr("Add")
            variant: "primary"
            enabled: nameField.text.trim().length > 0
            onClicked: {
                accounts.addOffline(nameField.text.trim())
                dlg.close()
            }
        }
    ]

    onOpened: nameField.forceActiveFocus()
    onClosed: nameField.text = ""
}
