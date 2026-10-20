import QtQuick
import Bee.Shell

Canvas {
    id: patch
    property bool mirrored: false
    property real r: Theme.r16
    property color fillColor: Theme.background
    width: r
    height: r
    antialiasing: true
    onFillColorChanged: requestPaint()
    onRChanged: requestPaint()
    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.fillStyle = fillColor
        ctx.beginPath()
        if (mirrored) {
            ctx.moveTo(0, 0)
            ctx.lineTo(r, 0)
            ctx.lineTo(r, r)
            ctx.arc(0, r, r, 0, -Math.PI / 2, true)
        } else {
            ctx.moveTo(r, 0)
            ctx.lineTo(0, 0)
            ctx.lineTo(0, r)
            ctx.arc(r, r, r, Math.PI, -Math.PI / 2, false)
            ctx.lineTo(r, 0)
        }
        ctx.closePath()
        ctx.fill()
    }
}
