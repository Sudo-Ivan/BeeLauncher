import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// One or two field prompt used for rename, copy and move-to-group.
BeeDialog {
    id: dlg
    property string field1Label: qsTr("Name")
    property string field2Label: ""
    property string field1Text: ""
    property string field2Text: ""
    property string targetId: ""
    signal acceptedText(string value1, string value2)

    function openWith(t, id, v1) {
        title = t
        targetId = id
        field1Text = v1
        field2Text = ""
        dlg.open()
    }

    // Fields are set imperatively because typing breaks the text binding.
    onOpened: {
        f1.text = dlg.field1Text
        f2.text = dlg.field2Text
        f1.forceActiveFocus()
    }

    ColumnLayout {
        spacing: Theme.sp8
        Layout.preferredWidth: 380
        Text { text: dlg.field1Label; color: Theme.textDim; font.pixelSize: Theme.caption }
        BeeTextField { id: f1; Layout.fillWidth: true }
        Text { visible: dlg.field2Label.length > 0; text: dlg.field2Label; color: Theme.textDim; font.pixelSize: Theme.caption }
        BeeTextField { id: f2; visible: dlg.field2Label.length > 0; Layout.fillWidth: true }
    }
    actions: [
        BeeButton {
            variant: "ghost"
            text: qsTr("Cancel")
            onClicked: dlg.close()
        },
        BeeButton {
            variant: "primary"
            text: qsTr("OK")
            enabled: f1.text.trim().length > 0
            onClicked: {
                dlg.close()
                dlg.acceptedText(f1.text.trim(), f2.text.trim())
            }
        }
    ]
}
