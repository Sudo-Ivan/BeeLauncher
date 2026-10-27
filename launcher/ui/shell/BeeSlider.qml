import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Bee.Shell

Slider {
    id: sl
    implicitHeight: 28
    handle: Rectangle {
        x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
        y: sl.topPadding + sl.availableHeight / 2 - height / 2
        width: 20
        height: 20
        radius: 10
        color: Theme.accent
        border.width: sl.pressed ? 0 : 2
        border.color: Theme.withAlpha(Theme.onAccent, 0.35)
        scale: sl.pressed ? 0.85 : 1
        Behavior on scale { NumberAnimation { duration: Theme.dur(120) } }
    }
    background: Rectangle {
        x: sl.leftPadding
        y: sl.topPadding + sl.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 6
        width: sl.availableWidth
        height: implicitHeight
        radius: 3
        color: Theme.surfaceHover
        Rectangle {
            width: sl.visualPosition * parent.width
            height: parent.height
            radius: 3
            color: Theme.accent
        }
    }
}
