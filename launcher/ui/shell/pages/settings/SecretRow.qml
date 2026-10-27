import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// Masked text setting row with a reveal toggle. Commits on every change.
BeeFieldRow {
    id: row
    property string key: ""
    property string placeholder: ""
    property bool ready: false
    property string lastLoaded: ""
    property bool revealed: false

    function backend() { return typeof shellSettings !== "undefined" ? shellSettings : null }
    function reload() {
        var b = backend()
        var v = b ? b.value(key) : null
        row.lastLoaded = v === null || v === undefined ? "" : String(v)
        field.text = row.lastLoaded
    }
    Component.onCompleted: { reload(); ready = true }
    Connections {
        target: row.backend()
        function onChanged(k) { if (k === row.key) row.reload() }
    }

    TextField {
        id: field
        Layout.preferredWidth: 280
        implicitHeight: 40
        echoMode: row.revealed ? TextInput.Normal : TextInput.Password
        placeholderText: row.placeholder
        placeholderTextColor: Theme.textFaint
        color: Theme.text
        font.pixelSize: Theme.body
        selectionColor: Theme.accent
        selectedTextColor: Theme.onAccent
        onTextChanged: {
            if (!row.ready || text === row.lastLoaded)
                return
            var b = row.backend()
            if (b) {
                row.lastLoaded = text
                b.setValue(row.key, text)
            }
        }
        background: Rectangle {
            radius: Theme.r12
            color: Theme.surface
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.accentAlpha(0.6) : Theme.border
        }
    }
    BeeIconButton {
        iconSource: row.revealed ? "icons/eye_off.svg" : "icons/eye.svg"
        Accessible.name: row.revealed ? qsTr("Hide") : qsTr("Show")
        onClicked: row.revealed = !row.revealed
    }
    BeeIconButton {
        iconSource: "icons/refresh.svg"
        Accessible.name: qsTr("Reset")
        onClicked: { var b = row.backend(); if (b) b.resetValue(row.key) }
    }
}
