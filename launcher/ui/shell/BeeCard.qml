import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Effects
import Bee.Shell

// Instance card used in the Library grid and Home recents.
Rectangle {
    id: card
    property string instanceId: ""
    property string name: ""
    property string iconSource: ""
    property string versionText: ""
    property real lastPlayedMs: 0
    property real totalPlayTimeMs: 0
    property bool running: false
    property bool selected: false
    property bool compact: false
    property var controller: null
    readonly property bool coverBlur: GraphicsInfo.api !== GraphicsInfo.Software

    signal clickedCard()
    signal playClicked()
    signal menuRequested(real x, real y)

    implicitWidth: 240
    implicitHeight: compact ? 180 : 228
    radius: Theme.r16
    color: cardHover.hovered ? Theme.surfaceHover : Theme.surface
    border.width: card.selected ? 2 : 1
    border.color: card.selected ? Theme.accent : (cardHover.hovered ? Theme.accentAlpha(0.45) : Theme.border)
    clip: true
    Accessible.role: Accessible.Button
    Accessible.name: card.name

    transform: Translate { y: cardHover.hovered ? -4 : 0 }
    Behavior on transform { PropertyAnimation { duration: Theme.dur(160); easing.type: Easing.OutCubic } }
    Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
    scale: cardMouse.pressed ? 0.985 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.dur(100) } }

    // Whole-card click area sits under the content so overlay buttons like
    // play and more still receive their own clicks.
    MouseArea {
        id: cardMouse
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) card.menuRequested(mouse.x, mouse.y)
            else card.clickedCard()
        }
    }
    HoverHandler { id: cardHover }

    function lastPlayedText() {
        if (!lastPlayedMs) return qsTr("Never played")
        var diff = Date.now() - lastPlayedMs
        var mins = Math.floor(diff / 60000)
        if (mins < 60) return qsTr("Played %1m ago").arg(Math.max(1, mins))
        var hrs = Math.floor(mins / 60)
        if (hrs < 24) return qsTr("Played %1h ago").arg(hrs)
        return qsTr("Played %1d ago").arg(Math.floor(hrs / 24))
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: card.border.width
        spacing: 0

        Item {
            id: cover
            Layout.fillWidth: true
            Layout.preferredHeight: card.compact ? 96 : 136
            Image {
                id: coverArt
                anchors.fill: parent
                source: card.iconSource
                sourceSize: Qt.size(96, 96)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                visible: false
            }
            Loader {
                anchors.fill: parent
                active: GraphicsInfo.api !== GraphicsInfo.Software && coverArt.status === Image.Ready
                sourceComponent: MultiEffect {
                    source: coverArt
                    blurEnabled: true
                    blurMax: 48
                    blur: 1.0
                    brightness: Theme.dark ? -0.35 : -0.05
                    saturation: 0.2
                    autoPaddingEnabled: false
                }
            }
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.withAlpha(Theme.coverColors(card.instanceId)[0], coverBlur ? 0.55 : 1.0) }
                    GradientStop { position: 1.0; color: Theme.withAlpha(Theme.coverColors(card.instanceId)[1], coverBlur ? 0.55 : 1.0) }
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: Theme.r16
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(1,1,1, Theme.dark ? 0.10 : 0.35) }
                    GradientStop { position: 0.6; color: "transparent" }
                }
            }
            Image {
                anchors.centerIn: parent
                width: card.compact ? 56 : 72
                height: card.compact ? 56 : 72
                source: card.iconSource
                sourceSize: Qt.size(72, 72)
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
            // running indicator
            Rectangle {
                visible: card.running
                anchors.left: parent.left; anchors.top: parent.top
                anchors.margins: Theme.sp8
                width: runLabel.implicitWidth + Theme.sp16
                height: runLabel.implicitHeight + Theme.sp4
                radius: height / 2
                color: Qt.rgba(0, 0, 0, 0.55)
                Text {
                    id: runLabel
                    anchors.centerIn: parent
                    text: qsTr("Running")
                    color: "#ffffff"
                    font.pixelSize: Theme.caption
                    font.weight: Font.DemiBold
                }
            }
            // more button
            BeeIconButton {
                anchors.right: parent.right; anchors.top: parent.top
                anchors.margins: Theme.sp8
                iconSource: "icons/more_horizontal.svg"
                visible: cardHover.hovered || activeFocus
                onClicked: card.menuRequested(parent.width, height)
                z: 2
            }
            // hover play button
            Rectangle {
                width: 44; height: 44; radius: 22
                anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.margins: Theme.sp12
                color: card.running ? Theme.danger : Theme.accent
                opacity: cardHover.hovered ? 1 : 0
                y: cardHover.hovered ? 0 : 6
                transform: Translate { y: cardHover.hovered ? 0 : 6 }
                Behavior on opacity { NumberAnimation { duration: Theme.dur(140) } }
                IconImage {
                    anchors.centerIn: parent
                    source: card.running ? "icons/stop.svg" : "icons/play.svg"
                    color: card.running ? "#ffffff" : Theme.onAccent
                    sourceSize: Qt.size(20,20)
                    width: 20; height: 20
                }
                MouseArea { anchors.fill: parent; onClicked: { mouse.accepted = true; card.playClicked() } }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.sp16
            Layout.topMargin: Theme.sp12
            spacing: Theme.sp8
            Text {
                text: card.name
                color: Theme.text
                font.pixelSize: Theme.bodyLarge
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                BeeChip { label: card.versionText; visible: card.versionText.length > 0 }
                Item { Layout.fillWidth: true }
                Text { visible: !card.compact; text: card.lastPlayedText(); color: Theme.textFaint; font.pixelSize: Theme.caption }
            }
            Item { Layout.fillHeight: true }
        }
    }

    BeeCornerPatch { x: 0; y: 0; z: 5 }
    BeeCornerPatch { mirrored: true; x: card.width - width; y: 0; z: 5 }
}
