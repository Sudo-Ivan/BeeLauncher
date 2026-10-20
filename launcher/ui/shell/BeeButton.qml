import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

Button {
    id: btn
    property string variant: "secondary"   // primary | secondary | ghost | danger
    property string iconSource: ""
    property int btnHeight: 38
    implicitHeight: btnHeight
    leftPadding: Theme.sp16
    rightPadding: Theme.sp16
    Accessible.role: Accessible.Button

    contentItem: RowLayout {
        spacing: Theme.sp8
        IconImage {
            visible: btn.iconSource.length > 0
            source: btn.iconSource
            color: btn.variant === "primary" ? Theme.onAccent : (btn.variant === "danger" ? Theme.onAccent : Theme.text)
            sourceSize: Qt.size(18, 18)
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
        }
        Text {
            text: btn.text
            font.pixelSize: Theme.body
            font.weight: Font.DemiBold
            color: btn.variant === "primary" || btn.variant === "danger" ? Theme.onAccent : Theme.text
            Layout.alignment: Qt.AlignVCenter
        }
    }
    background: Rectangle {
        radius: Theme.r12
        color: {
            if (btn.variant === "primary")
                return btn.down ? Theme.accentPressed : (btn.hovered ? Theme.accentHover : Theme.accent)
            if (btn.variant === "danger")
                return btn.down ? "#c62828" : (btn.hovered ? "#e57373" : Theme.danger)
            if (btn.variant === "ghost")
                return btn.down || btn.hovered ? Theme.surfaceHover : "transparent"
            return btn.down ? Theme.surfaceHover : (btn.hovered ? Theme.surfaceHover : Theme.surface)
        }
        border.color: btn.variant === "secondary" ? Theme.border : "transparent"
        border.width: btn.variant === "secondary" ? 1 : 0
        Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    }
    Rectangle {
        visible: btn.activeFocus
        anchors.fill: parent
        anchors.margins: -3
        radius: Theme.r12 + 3
        color: "transparent"
        border.width: 2
        border.color: Theme.accentAlpha(0.6)
    }
}
