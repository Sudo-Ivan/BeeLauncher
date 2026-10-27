import QtQuick
import Bee.Shell

// Integer setting row backed by a spin box.
BeeFieldRow {
    id: row
    property string key: ""
    property int from: 0
    property int to: 100
    property string suffix: ""
    property bool ready: false

    function backend() { return typeof shellSettings !== "undefined" ? shellSettings : null }
    function reload() {
        var b = backend()
        var v = b ? b.value(key) : null
        var n = v === null || v === undefined ? 0 : Math.round(Number(v))
        if (isNaN(n))
            n = 0
        spin.value = Math.max(row.from, Math.min(row.to, n))
    }
    Component.onCompleted: { reload(); ready = true }
    Connections {
        target: row.backend()
        function onChanged(k) { if (k === row.key) row.reload() }
    }

    BeeSpinBox {
        id: spin
        from: row.from
        to: row.to
        suffix: row.suffix
        onValueModified: {
            var b = row.backend()
            if (row.ready && b)
                b.setValue(row.key, value)
        }
    }
    BeeIconButton {
        iconSource: "icons/refresh.svg"
        Accessible.name: qsTr("Reset")
        onClicked: { var b = row.backend(); if (b) b.resetValue(row.key) }
    }
}
