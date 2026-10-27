import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

SpinBox {
    id: sb
    property string suffix: ""
    implicitHeight: 40
    editable: true
    contentItem: TextInput {
        color: Theme.text
        font.pixelSize: Theme.body
        selectionColor: Theme.accent
        selectedTextColor: Theme.onAccent
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !sb.editable
        validator: sb.validator
        inputMethodHints: sb.inputMethodHints
        Component.onCompleted: text = sb.displayText + (sb.suffix.length ? " " + sb.suffix : "")
        Connections {
            target: sb
            function onValueModified() {
                if (!activeFocus)
                    text = sb.displayText + (sb.suffix.length ? " " + sb.suffix : "")
            }
        }
        onActiveFocusChanged: {
            if (!activeFocus)
                text = sb.displayText + (sb.suffix.length ? " " + sb.suffix : "")
        }
        onTextEdited: {
            var cleaned = text.replace(/[^0-9\-]/g, "")
            if (cleaned.length) {
                var v = parseInt(cleaned)
                if (!isNaN(v))
                    sb.value = v
            }
        }
        onAccepted: sb.valueModified()
    }
    up.indicator: Rectangle {
        x: sb.mirrored ? 0 : parent.width - width
        width: 32
        height: parent.height
        radius: Theme.r8
        color: sb.up.hovered ? Theme.surfaceHover : "transparent"
        Text { anchors.centerIn: parent; text: "+"; color: Theme.textDim; font.pixelSize: Theme.bodyLarge }
    }
    down.indicator: Rectangle {
        x: sb.mirrored ? parent.width - width : 0
        width: 32
        height: parent.height
        radius: Theme.r8
        color: sb.down.hovered ? Theme.surfaceHover : "transparent"
        Text { anchors.centerIn: parent; text: "-"; color: Theme.textDim; font.pixelSize: Theme.bodyLarge }
    }
    background: Rectangle {
        radius: Theme.r12
        color: Theme.surface
        border.width: 1
        border.color: sb.activeFocus ? Theme.accentAlpha(0.6) : Theme.border
    }
}
