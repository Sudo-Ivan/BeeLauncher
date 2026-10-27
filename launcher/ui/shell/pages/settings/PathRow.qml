import QtQuick
import QtQuick.Layouts
import Bee.Shell

// Path setting row with a text field, an open-folder button and a reset.
BeeFieldRow {
    id: row
    property string key: ""
    property string placeholder: ""
    property bool ready: false
    property string lastLoaded: ""

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

    BeeTextField {
        id: field
        Layout.preferredWidth: 280
        placeholderText: row.placeholder
        onTextChanged: {
            if (!row.ready || text === row.lastLoaded)
                return
            var b = row.backend()
            if (b) {
                row.lastLoaded = text
                b.setValue(row.key, text)
            }
        }
    }
    BeeIconButton {
        iconSource: "icons/folder.svg"
        Accessible.name: qsTr("Open folder")
        onClicked: { var b = row.backend(); if (b) b.openSettingPath(row.key) }
    }
    BeeIconButton {
        iconSource: "icons/refresh.svg"
        Accessible.name: qsTr("Reset")
        onClicked: { var b = row.backend(); if (b) b.resetValue(row.key) }
    }
}
