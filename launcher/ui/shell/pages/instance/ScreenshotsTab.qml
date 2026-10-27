import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Screenshot grid for the instance .minecraft/screenshots folder. Clicking a
// tile opens a lightbox; from there the file can be opened in the system
// viewer.
Item {
    id: root
    property var bridge: null
    property var shots: []

    function refresh() {
        root.shots = root.bridge ? root.bridge.screenshots() : []
    }
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()
    Connections {
        target: root.bridge
        function onInstanceChanged() { root.refresh() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.sp16
            spacing: Theme.sp8
            Text {
                text: qsTr("Screenshots")
                color: Theme.text
                font.pixelSize: Theme.bodyLarge
                font.weight: Font.DemiBold
            }
            Text {
                text: qsTr("%1 files").arg(root.shots.length)
                color: Theme.textFaint
                font.pixelSize: Theme.caption
            }
            Item { Layout.fillWidth: true }
            BeeIconButton {
                iconSource: "icons/refresh.svg"
                Accessible.name: qsTr("Refresh")
                onClicked: root.refresh()
            }
            BeeButton {
                variant: "secondary"
                btnHeight: 36
                text: qsTr("Open folder")
                iconSource: "icons/folder.svg"
                onClicked: if (root.bridge) root.bridge.openScreenshotsFolder()
            }
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.sp16
            Layout.rightMargin: Theme.sp16
            Layout.bottomMargin: Theme.sp16
            clip: true
            cellWidth: Math.max(160, Math.floor(width / Math.max(1, Math.floor(width / 220))))
            cellHeight: Math.floor(cellWidth * 0.62)
            model: root.shots
            ScrollBar.vertical: BeeScrollBar {}

            delegate: Item {
                width: grid.cellWidth
                height: grid.cellHeight
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: Theme.sp4
                    radius: Theme.r12
                    color: Theme.surface
                    border.color: Theme.border
                    clip: true
                    Image {
                        anchors.fill: parent
                        // ?t= busts the image cache when the file changes
                        source: root.bridge ? root.bridge.screenshotUrl(modelData.path) + "?t=" + modelData.mtime : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: label.implicitHeight + Theme.sp8
                        color: Theme.shadow(0.6)
                        Text {
                            id: label
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: Theme.sp8
                            text: modelData.name
                            color: Theme.text
                            font.pixelSize: Theme.caption
                            elide: Text.ElideMiddle
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            lightbox.path = modelData.path
                            lightbox.name = modelData.name
                            lightbox.open()
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.shots.length === 0
            spacing: Theme.sp8
            IconImage {
                source: "../../icons/image.svg"
                color: Theme.textFaint
                sourceSize: Qt.size(40, 40)
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: qsTr("No screenshots yet")
                color: Theme.textDim
                font.pixelSize: Theme.body
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    BeeDialog {
        id: lightbox
        property string path: ""
        property string name: ""
        title: name
        Rectangle {
            width: Math.max(200, Math.min(root.width - 160, 860))
            height: Math.max(160, Math.min(root.height - 260, 560))
            radius: Theme.r8
            color: Theme.shadow(0.4)
            clip: true
            Image {
                anchors.fill: parent
                source: lightbox.path.length && root.bridge ? root.bridge.screenshotUrl(lightbox.path) : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
        }
        actions: [
            BeeButton {
                text: qsTr("Open in viewer")
                iconSource: "icons/image.svg"
                onClicked: { if (root.bridge) root.bridge.openFile(lightbox.path) }
            },
            BeeButton {
                text: qsTr("Open folder")
                variant: "ghost"
                onClicked: { if (root.bridge) root.bridge.openScreenshotsFolder() }
            },
            BeeButton { text: qsTr("Close"); variant: "secondary"; onClicked: lightbox.close() }
        ]
    }
}
