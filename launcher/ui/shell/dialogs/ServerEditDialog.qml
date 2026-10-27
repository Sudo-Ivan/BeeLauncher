import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Shared add/edit dialog for the Servers tab. editRow < 0 means add mode;
// >= 0 edits that model row via servers.updateServer.
BeeDialog {
    id: dlg
    property int editRow: -1
    property string initialName: ""
    property string initialAddress: ""
    title: editRow < 0 ? qsTr("Add server") : qsTr("Edit server")

    onOpened: {
        nameField.text = initialName
        addrField.text = initialAddress
        formError.text = ""
        nameField.forceActiveFocus()
    }

    ColumnLayout {
        Layout.preferredWidth: 420
        spacing: Theme.sp12
        ColumnLayout {
            spacing: Theme.sp4
            Layout.fillWidth: true
            Text { text: qsTr("Server name"); color: Theme.textDim; font.pixelSize: Theme.caption }
            BeeTextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("My favorite server")
            }
        }
        ColumnLayout {
            spacing: Theme.sp4
            Layout.fillWidth: true
            Text { text: qsTr("Server address"); color: Theme.textDim; font.pixelSize: Theme.caption }
            BeeTextField {
                id: addrField
                Layout.fillWidth: true
                placeholderText: "play.example.com:25565"
            }
        }
        Text {
            id: formError
            visible: text.length > 0
            color: Theme.danger
            font.pixelSize: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    actions: [
        BeeButton {
            variant: "secondary"
            text: qsTr("Cancel")
            onClicked: dlg.close()
        },
        BeeButton {
            variant: "primary"
            text: qsTr("Save")
            onClicked: {
                var nm = nameField.text.trim()
                var ad = addrField.text.trim()
                if (!nm.length || !ad.length) {
                    formError.text = qsTr("Server name and address are required")
                    return
                }
                if (typeof servers !== "undefined" && servers) {
                    if (dlg.editRow < 0)
                        servers.addServer(nm, ad)
                    else
                        servers.updateServer(dlg.editRow, nm, ad)
                }
                dlg.close()
            }
        }
    ]
}
