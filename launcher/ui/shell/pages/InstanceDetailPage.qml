import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell
import "instance"

// Full-page overlay showing one instance: header with launch controls plus
// tabbed bodies for content, notes, logs and per-instance settings.
// API: set instanceId, listen to closed() and launched().
Item {
    id: page
    property string instanceId: ""
    // Resolved lazily so the page still loads when the instanceBridge context
    // property has not been wired into the engine yet (tests, previews).
    property var bridge: null
    property int currentTab: 0
    property string errorText: ""

    signal closed()
    signal launched()

    Component.onCompleted: {
        try {
            if (typeof instanceBridge !== "undefined")
                page.bridge = instanceBridge
        } catch (e) {
            page.bridge = null
        }
        if (page.bridge && page.instanceId.length)
            page.bridge.setInstanceId(page.instanceId)
    }
    onInstanceIdChanged: if (page.bridge) page.bridge.setInstanceId(page.instanceId)

    Rectangle { anchors.fill: parent; color: Theme.background }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // header strip
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            color: Theme.rail
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp24
                anchors.rightMargin: Theme.sp24
                spacing: Theme.sp16

                BeeIconButton {
                    iconSource: "icons/dismiss.svg"
                    Accessible.name: qsTr("Back")
                    onClicked: page.closed()
                }
                Rectangle {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    radius: Theme.r16
                    color: Theme.surface
                    border.color: Theme.border
                    clip: true
                    Image {
                        anchors.fill: parent
                        anchors.margins: Theme.sp4
                        source: page.bridge ? page.bridge.iconSource : ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }
                }
                ColumnLayout {
                    spacing: Theme.sp4
                    Layout.fillWidth: true
                    RowLayout {
                        spacing: Theme.sp12
                        Text {
                            text: page.bridge ? page.bridge.name : ""
                            color: Theme.text
                            font.pixelSize: Theme.title
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.maximumWidth: 520
                        }
                        BeeChip { label: page.bridge ? page.bridge.versionText : "" }
                        BeeChip { label: page.bridge ? page.bridge.typeDisplay : "" }
                    }
                    Text {
                        text: page.bridge && page.bridge.running ? qsTr("Running") : qsTr("Not running")
                        color: page.bridge && page.bridge.running ? Theme.success : Theme.textFaint
                        font.pixelSize: Theme.caption
                    }
                }
                BeeButton {
                    variant: page.bridge && page.bridge.running ? "danger" : "primary"
                    btnHeight: 44
                    text: page.bridge && page.bridge.running ? qsTr("Stop") : qsTr("Play")
                    iconSource: page.bridge && page.bridge.running ? "icons/stop.svg" : "icons/play.svg"
                    enabled: page.bridge !== null && page.bridge.hasInstance
                    onClicked: {
                        if (!page.bridge)
                            return
                        if (page.bridge.running)
                            page.bridge.kill()
                        else {
                            page.bridge.launch()
                            page.launched()
                        }
                    }
                }
                BeeButton {
                    variant: "secondary"
                    btnHeight: 44
                    text: qsTr("Classic editor")
                    iconSource: "icons/edit.svg"
                    enabled: page.bridge !== null && page.bridge.hasInstance
                    onClicked: if (page.bridge) page.bridge.openClassic()
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
        }

        // inline error banner fed by instanceBridge.actionFailed
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            visible: page.errorText.length > 0
            color: Theme.withAlpha(Theme.danger, 0.15)
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp24
                anchors.rightMargin: Theme.sp16
                IconImage { source: "../icons/warning.svg"; color: Theme.danger; sourceSize: Qt.size(16, 16) }
                Text {
                    text: page.errorText
                    color: Theme.danger
                    font.pixelSize: Theme.body
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                BeeIconButton { iconSource: "icons/dismiss.svg"; onClicked: page.errorText = "" }
            }
        }

        // tab bar
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp24
                spacing: Theme.sp4
                Repeater {
                    model: [qsTr("Overview"), qsTr("Mods"), qsTr("Worlds"), qsTr("Screenshots"), qsTr("Notes"), qsTr("Logs"), qsTr("Settings")]
                    BeeTabButton {
                        text: modelData
                        iconSource: "icons/" + ["home.svg", "puzzle.svg", "world.svg", "image.svg", "note.svg", "log.svg", "settings.svg"][index]
                        active: page.currentTab === index
                        onActivated: page.currentTab = index
                    }
                }
                Item { Layout.fillWidth: true }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: page.currentTab

            OverviewTab {
                bridge: page.bridge
                onGotoTab: function(i) { page.currentTab = i }
            }
            ModsTab { bridge: page.bridge }
            WorldsTab { bridge: page.bridge }
            ScreenshotsTab { bridge: page.bridge }
            NotesTab { bridge: page.bridge }
            LogsTab { bridge: page.bridge }
            InstanceSettingsTab { bridge: page.bridge }
        }
    }

    Connections {
        target: page.bridge
        function onActionFailed(msg) { page.errorText = msg }
    }
}
