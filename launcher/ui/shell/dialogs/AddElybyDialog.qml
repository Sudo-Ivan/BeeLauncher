import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// Ely.by sign-in, mirroring ElybyLoginDialog: a browser OAuth flow races a
// device code flow. Elyby has no password grant, so this dialog shows the
// device code path exactly like MsaSignInDialog.
BeeDialog {
    id: dlg
    title: qsTr("Sign in with Ely.by")

    property string url: ""
    property string code: ""
    property string statusText: ""
    property string errorText: ""

    Text {
        text: qsTr("A sign-in page may also open in your browser. Otherwise open %1 and enter the code below.").arg(dlg.url.length > 0 ? dlg.url : "account.ely.by/code")
        color: Theme.textDim
        font.pixelSize: Theme.body
        wrapMode: Text.WordWrap
        Layout.preferredWidth: 420
    }

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
            text: qsTr("Open sign-in page")
            iconSource: "icons/link.svg"
            variant: "primary"
            onClicked: Qt.openUrlExternally(dlg.url.length > 0 ? dlg.url : "https://account.ely.by/code")
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
            onClicked: { accounts.cancelElyby(); dlg.close() }
        }
    ]

    onClosed: if (accounts.elybyBusy) accounts.cancelElyby()

    Connections {
        target: accounts
        function onElybyStatus(status) { dlg.statusText = status; dlg.errorText = "" }
        function onElybyCodeRequested(url, code) { dlg.url = url; dlg.code = code; dlg.errorText = "" }
        function onElybyFailed(reason) { if (reason.length > 0) dlg.errorText = reason; else dlg.close() }
        function onElybyFinished() { dlg.close() }
    }
}
