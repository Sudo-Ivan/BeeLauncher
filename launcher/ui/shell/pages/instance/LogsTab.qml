import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Two-pane log browser mirroring OtherLogsPage: the same folder set is
// enumerated by the bridge, the right side shows the tail of the selected
// file.
Item {
    id: root
    property var bridge: null
    property var logs: []
    property int currentLog: -1

    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KiB"
        return (bytes / (1024 * 1024)).toFixed(1) + " MiB"
    }
    function formatTime(ms) {
        if (!ms) return ""
        var d = new Date(ms)
        return d.toLocaleDateString() + " " + d.toLocaleTimeString()
    }
    function refresh() {
        root.logs = root.bridge ? root.bridge.logFiles() : []
        if (root.currentLog >= root.logs.length)
            root.currentLog = -1
        if (root.currentLog >= 0)
            viewer.text = root.bridge.readLogTail(root.logs[root.currentLog].path, 2000)
        else
            viewer.text = ""
    }
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()
    Connections {
        target: root.bridge
        function onInstanceChanged() { root.refresh() }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // file list
        ColumnLayout {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            spacing: 0
            ListView {
                id: logList
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Theme.sp12
                clip: true
                spacing: 2
                model: root.logs
                ScrollBar.vertical: BeeScrollBar {}
                delegate: BeeListDelegate {
                    width: logList.width
                    title: modelData.name
                    subtitle: root.formatSize(modelData.sizeBytes) + "  " + root.formatTime(modelData.mtime)
                    iconSource: "icons/log.svg"
                    selected: index === root.currentLog
                    onClicked: {
                        root.currentLog = index
                        if (root.bridge)
                            viewer.text = root.bridge.readLogTail(modelData.path, 2000)
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        }

        Rectangle { Layout.fillHeight: true; Layout.preferredWidth: 1; color: Theme.border }

        // viewer
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.sp12
                spacing: Theme.sp8
                Text {
                    text: root.currentLog >= 0 ? root.logs[root.currentLog].name : qsTr("Select a log file")
                    color: root.currentLog >= 0 ? Theme.text : Theme.textFaint
                    font.pixelSize: Theme.body
                    font.weight: Font.Medium
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                BeeIconButton {
                    iconSource: "icons/refresh.svg"
                    Accessible.name: qsTr("Refresh")
                    onClicked: root.refresh()
                }
                BeeButton {
                    variant: "secondary"
                    btnHeight: 36
                    text: qsTr("Open file")
                    iconSource: "icons/folder.svg"
                    enabled: root.currentLog >= 0
                    onClicked: if (root.bridge) root.bridge.openLog(root.logs[root.currentLog].path)
                }
                BeeButton {
                    variant: "ghost"
                    btnHeight: 36
                    text: qsTr("Classic log viewer")
                    onClicked: if (root.bridge) root.bridge.openClassicPage("logs")
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            TextArea.flickable: TextArea {
                id: viewer
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Theme.sp12
                readOnly: true
                wrapMode: TextEdit.NoWrap
                font.family: "monospace"
                font.pixelSize: Theme.caption
                color: Theme.text
                selectionColor: Theme.accent
                selectedTextColor: Theme.onAccent
                placeholderText: qsTr("Log contents appear here")
                placeholderTextColor: Theme.textFaint
                ScrollBar.vertical: BeeScrollBar {}
                ScrollBar.horizontal: BeeScrollBar {}
                background: Rectangle {
                    radius: Theme.r12
                    color: Theme.surface
                    border.color: Theme.border
                }
            }
        }
    }
}
