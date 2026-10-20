import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

TextField {
    id: tf
    implicitHeight: 44
    leftPadding: 40
    rightPadding: tf.text.length ? 40 : Theme.sp16
    placeholderTextColor: Theme.textFaint
    color: Theme.text
    font.pixelSize: Theme.body
    Accessible.role: Accessible.EditableText

    IconImage {
        source: "icons/search.svg"
        color: Theme.textFaint
        sourceSize: Qt.size(18, 18)
        anchors.left: parent.left
        anchors.leftMargin: Theme.sp12
        anchors.verticalCenter: parent.verticalCenter
        width: 18; height: 18
    }
    IconImage {
        visible: tf.text.length > 0
        source: "icons/dismiss.svg"
        color: clearArea.containsMouse ? Theme.text : Theme.textDim
        sourceSize: Qt.size(16, 16)
        anchors.right: parent.right
        anchors.rightMargin: Theme.sp12
        anchors.verticalCenter: parent.verticalCenter
        width: 16; height: 16
        MouseArea { id: clearArea; anchors.fill: parent; hoverEnabled: true; onClicked: tf.text = "" }
    }
    background: Rectangle {
        radius: Theme.r12
        color: Theme.surface
        border.width: tf.activeFocus ? 2 : 1
        border.color: tf.activeFocus ? Theme.accentAlpha(0.6) : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.dur(120) } }
    }
}
