pragma Singleton
import QtQuick

QtObject {
    property bool dark: true
    property bool animations: true

    function dur(ms) { return animations ? ms : 0 }

    readonly property color background: dark ? "#0f0f10" : "#f6f5f2"
    readonly property color rail: dark ? "#141415" : "#ffffff"
    readonly property color surface: dark ? "#1a1a1c" : "#ffffff"
    readonly property color surfaceHover: dark ? "#212124" : "#f0eeea"
    readonly property color surfaceRaised: dark ? "#26262a" : "#ffffff"
    readonly property color border: dark ? "#2a2a2e" : "#e4e1da"
    readonly property color text: dark ? "#f2f2f3" : "#1b1a17"
    readonly property color textDim: dark ? "#a1a1a8" : "#5c5a54"
    readonly property color textFaint: dark ? "#6e6e76" : "#8f8c84"
    readonly property color accent: dark ? "#e8a33d" : "#c98418"
    readonly property color accentHover: dark ? "#f0b258" : "#b87510"
    readonly property color accentPressed: dark ? "#cf8f2c" : "#9c630d"
    readonly property color onAccent: dark ? "#1a1206" : "#ffffff"
    readonly property color danger: "#ef5350"
    readonly property color success: "#66bb6a"

    readonly property int sp4: 4
    readonly property int sp8: 8
    readonly property int sp12: 12
    readonly property int sp16: 16
    readonly property int sp24: 24
    readonly property int sp32: 32

    readonly property int r8: 8
    readonly property int r12: 12
    readonly property int r16: 16
    readonly property int r24: 24

    readonly property int caption: 12
    readonly property int body: 14
    readonly property int bodyLarge: 16
    readonly property int title: 20
    readonly property int headline: 28
    readonly property int display: 40

    function accentAlpha(a) { return Qt.rgba(accent.r, accent.g, accent.b, a) }
    function withAlpha(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }
    function shadow(alpha) { return Qt.rgba(0, 0, 0, dark ? alpha : alpha * 0.6) }

    // per instance cover gradient hue derived from a string
    function coverColors(id) {
        var h = 0
        for (var i = 0; i < id.length; i++) h = (h * 31 + id.charCodeAt(i)) % 360
        var s = dark ? 0.35 : 0.45
        var v1 = dark ? 0.30 : 0.82
        var v2 = dark ? 0.18 : 0.68
        return [Qt.hsva(h / 360, s, v2, 1), Qt.hsva((h + 40) % 360 / 360, s, v1, 1)]
    }
}
