import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

// Integer setting row with a slider and a spin box kept in sync.
BeeFieldRow {
    id: row
    property string key: ""
    property int from: 0
    property int to: 100
    property int stepSize: 1
    property string suffix: ""
    property bool ready: false

    function backend() { return typeof shellSettings !== "undefined" ? shellSettings : null }
    function commit(v) {
        var b = row.backend()
        if (row.ready && b)
            b.setValue(row.key, v)
    }
    function reload() {
        var b = backend()
        var v = b ? b.value(key) : null
        var n = v === null || v === undefined ? row.from : Math.round(Number(v))
        if (isNaN(n))
            n = row.from
        n = Math.max(row.from, Math.min(row.to, n))
        slider.value = n
        spin.value = n
    }
    Component.onCompleted: { reload(); ready = true }
    Connections {
        target: row.backend()
        function onChanged(k) { if (k === row.key) row.reload() }
    }

    BeeSlider {
        id: slider
        Layout.preferredWidth: 200
        from: row.from
        to: row.to
        stepSize: row.stepSize
        snapMode: Slider.SnapAlways
        onMoved: {
            spin.value = Math.round(value)
            row.commit(Math.round(value))
        }
    }
    BeeSpinBox {
        id: spin
        from: row.from
        to: row.to
        suffix: row.suffix
        onValueModified: {
            slider.value = value
            row.commit(value)
        }
    }
    BeeIconButton {
        iconSource: "icons/refresh.svg"
        Accessible.name: qsTr("Reset")
        onClicked: { var b = row.backend(); if (b) b.resetValue(row.key) }
    }
}
