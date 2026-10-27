import QtQuick
import Bee.Shell

// Boolean setting row: label on the left, checkbox and reset on the right.
BeeFieldRow {
    id: row
    property string key: ""
    property bool ready: false

    function backend() { return typeof shellSettings !== "undefined" ? shellSettings : null }
    function reload() {
        var b = backend()
        chk.checked = b ? b.value(key) === true : false
    }
    Component.onCompleted: { reload(); ready = true }
    Connections {
        target: row.backend()
        function onChanged(k) { if (k === row.key) row.reload() }
    }

    BeeCheckBox {
        id: chk
        onToggled: {
            var b = row.backend()
            if (row.ready && b)
                b.setValue(row.key, checked)
        }
    }
    BeeIconButton {
        iconSource: "icons/refresh.svg"
        Accessible.name: qsTr("Reset")
        onClicked: { var b = row.backend(); if (b) b.resetValue(row.key) }
    }
}
