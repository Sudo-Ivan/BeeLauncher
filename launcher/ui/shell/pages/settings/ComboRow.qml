import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Combo setting row. options is a list of strings or { name, value } maps.
// Strings are used as both display text and stored value.
BeeFieldRow {
    id: row
    property string key: ""
    property var options: []
    property var texts: []
    property var storedValues: []
    property string rawValue: ""
    property bool ready: false

    function backend() { return typeof shellSettings !== "undefined" ? shellSettings : null }

    function norm() {
        var t = []
        var v = []
        for (var i = 0; i < options.length; i++) {
            var o = options[i]
            if (o !== null && typeof o === "object" && o.name !== undefined) {
                t.push(String(o.name))
                v.push(o.value !== undefined ? o.value : o.name)
            } else {
                t.push(String(o))
                v.push(o)
            }
        }
        texts = t
        storedValues = v
    }

    function reload() {
        var b = backend()
        var v = b ? b.value(key) : null
        var idx = v === null || v === undefined ? -1 : storedValues.indexOf(v)
        // QVariant scalars may arrive as a different JS type, retry loosely
        if (idx < 0 && v !== null && v !== undefined) {
            var sv = String(v)
            for (var i = 0; i < storedValues.length; i++) {
                if (String(storedValues[i]) === sv) {
                    idx = i
                    break
                }
            }
        }
        row.rawValue = v === null || v === undefined ? "" : String(v)
        combo.currentIndex = idx
    }

    onOptionsChanged: norm()
    Component.onCompleted: { norm(); reload(); ready = true }
    Connections {
        target: row.backend()
        function onChanged(k) { if (k === row.key) row.reload() }
    }

    ComboBox {
        id: combo
        Layout.preferredWidth: 240
        implicitHeight: 40
        model: row.texts
        onActivated: function(i) {
            var b = row.backend()
            if (row.ready && b && i >= 0 && i < row.storedValues.length)
                b.setValue(row.key, row.storedValues[i])
        }
        contentItem: RowLayout {
            spacing: Theme.sp8
            Text {
                text: combo.currentIndex >= 0 ? combo.displayText : row.rawValue
                color: Theme.text
                font.pixelSize: Theme.body
                elide: Text.ElideRight
                leftPadding: Theme.sp12
                Layout.fillWidth: true
            }
            IconImage {
                source: "../../icons/chevron_down.svg"
                color: Theme.textDim
                sourceSize: Qt.size(14, 14)
                Layout.rightMargin: Theme.sp12
            }
        }
        background: Rectangle {
            radius: Theme.r12
            color: combo.hovered ? Theme.surfaceHover : Theme.surface
            border.width: 1
            border.color: combo.activeFocus ? Theme.accentAlpha(0.6) : Theme.border
        }
        popup: Popup {
            y: combo.height + 4
            width: combo.width
            padding: Theme.sp8
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
            }
            background: Rectangle {
                radius: Theme.r12
                color: Theme.surfaceRaised
                border.color: Theme.border
            }
        }
        delegate: ItemDelegate {
            width: combo.width
            height: 36
            contentItem: Text {
                text: modelData
                color: Theme.text
                font.pixelSize: Theme.body
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: Theme.r8
                color: highlighted ? Theme.surfaceHover : "transparent"
            }
        }
    }
    BeeIconButton {
        iconSource: "icons/refresh.svg"
        Accessible.name: qsTr("Reset")
        onClicked: { var b = row.backend(); if (b) b.resetValue(row.key) }
    }
}
