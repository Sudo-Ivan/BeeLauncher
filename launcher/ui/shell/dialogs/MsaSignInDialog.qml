import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// Microsoft device code sign-in, mirroring MSALoginDialog. The page calls
// accounts.addMicrosoft() and opens this dialog when msaCodeRequested fires.
BeeDialog {
    id: dlg
    title: qsTr("Sign in with Microsoft")

    property string url: ""
    property string code: ""
    property string statusText: ""
    property string errorText: ""

    Text {
        text: qsTr("Open %1 in a browser and enter the code below to link your account.").arg(dlg.url.length > 0 ? dlg.url : "microsoft.com/link")
        color: Theme.textDim
        font.pixelSize: Theme.body
        wrapMode: Text.WordWrap
        Layout.preferredWidth: 420
    }

    // Selectable code; the Copy button uses the hidden-edit selectAll+copy trick.
    TextEdit {
        id: codeEdit
        readOnly: true
        text: dlg.code
        visible: dlg.code.length > 0
        color: Theme.accent
        font.family: "monospace"
        font.pixelSize: Theme.headline
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        selectByMouse: true
        Layout.fillWidth: true
    }

    RowLayout {
        spacing: Theme.sp8
        Layout.alignment: Qt.AlignHCenter
        BeeButton {
            text: qsTr("Copy code")
            iconSource: "icons/copy.svg"
            visible: dlg.code.length > 0
            onClicked: { codeEdit.selectAll(); codeEdit.copy(); codeEdit.deselect() }
        }
        BeeButton {
            text: qsTr("Open microsoft.com/link")
            iconSource: "icons/link.svg"
            variant: "primary"
            onClicked: Qt.openUrlExternally(dlg.url.length > 0 ? dlg.url : "https://www.microsoft.com/link")
        }
    }

    Text {
        visible: text.length > 0
        text: dlg.statusText
        color: Theme.textDim
        font.pixelSize: Theme.caption
        wrapMode: Text.WordWrap
        Layout.preferredWidth: 420
    }
    Text {
        visible: text.length > 0
        text: dlg.errorText
        color: Theme.danger
        font.pixelSize: Theme.body
        wrapMode: Text.WordWrap
        Layout.preferredWidth: 420
    }

    actions: [
        BeeButton {
            text: qsTr("Cancel")
            variant: "secondary"
            onClicked: { accounts.cancelMicrosoft(); dlg.close() }
        }
    ]

    // Closing the dialog early must not leave the flow running.
    onClosed: if (accounts.msaBusy) accounts.cancelMicrosoft()

    Connections {
        target: accounts
        function onMsaStatus(status) { dlg.statusText = status; dlg.errorText = "" }
        function onMsaCodeRequested(url, code) { dlg.url = url; dlg.code = code; dlg.errorText = "" }
        function onMsaFailed(reason) { if (reason.length > 0) dlg.errorText = reason; else dlg.close() }
        function onMsaFinished() { dlg.close() }
    }
}
