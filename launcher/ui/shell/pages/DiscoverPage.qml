import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Modrinth modpack discovery. Uses the discover context property
// (ShellDiscoverModel) for search/install and taskRunner for install
// progress. Drop into the StackLayout in place of the Discover placeholder.
Item {
    id: page

    property string errorText: ""
    property string noticeText: ""
    property bool searched: false

    function formatDownloads(n) {
        if (n >= 1000000)
            return (n / 1000000).toFixed(1) + "M"
        if (n >= 1000)
            return (n / 1000).toFixed(1) + "K"
        return String(n)
    }

    Connections {
        target: discover
        function onSearchFinished() {
            page.searched = true
            page.errorText = ""
        }
        function onSearchFailed(reason) {
            page.searched = true
            page.errorText = reason
        }
        function onInstallFinished() {
            page.errorText = ""
            page.noticeText = qsTr("Modpack installed")
            noticeTimer.restart()
        }
        function onInstallFailed(reason) {
            page.errorText = reason
        }
    }

    Timer {
        id: debounce
        interval: 350
        onTriggered: discover.search(searchField.text)
    }

    Timer {
        id: noticeTimer
        interval: 4000
        onTriggered: page.noticeText = ""
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp24
        spacing: Theme.sp12

        // search bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp12
            BeeTextField {
                id: searchField
                Layout.fillWidth: true
                Layout.maximumWidth: 480
                placeholderText: qsTr("Search Modrinth modpacks")
                onTextChanged: debounce.restart()
                onAccepted: discover.search(text)
            }
            BusyIndicator {
                running: discover.searching
                visible: running
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
            }
            Item { Layout.fillWidth: true }
        }

        // error banner
        Rectangle {
            visible: page.errorText.length > 0
            Layout.fillWidth: true
            implicitHeight: errorLabel.implicitHeight + Theme.sp12
            radius: Theme.r12
            color: Theme.withAlpha(Theme.danger, 0.12)
            border.width: 1
            border.color: Theme.withAlpha(Theme.danger, 0.4)
            Text {
                id: errorLabel
                anchors.fill: parent
                anchors.margins: Theme.sp8
                text: page.errorText
                color: Theme.danger
                font.pixelSize: Theme.caption
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }
        }

        // transient notice
        Rectangle {
            visible: page.noticeText.length > 0
            Layout.fillWidth: true
            implicitHeight: noticeLabel.implicitHeight + Theme.sp12
            radius: Theme.r12
            color: Theme.withAlpha(Theme.success, 0.12)
            border.width: 1
            border.color: Theme.withAlpha(Theme.success, 0.4)
            Text {
                id: noticeLabel
                anchors.fill: parent
                anchors.margins: Theme.sp8
                text: page.noticeText
                color: Theme.success
                font.pixelSize: Theme.caption
                verticalAlignment: Text.AlignVCenter
            }
        }

        // results grid
        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: discover
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 380)))
            cellHeight: 176
            ScrollBar.vertical: BeeScrollBar {}

            delegate: Item {
                width: grid.cellWidth
                height: grid.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: Theme.sp8
                    radius: Theme.r16
                    color: cardArea.containsMouse ? Theme.surfaceHover : Theme.surface
                    border.width: 1
                    border.color: Theme.border

                    MouseArea {
                        id: cardArea
                        anchors.fill: parent
                        hoverEnabled: true
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.sp12
                        spacing: Theme.sp8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp12
                            Rectangle {
                                Layout.preferredWidth: 56
                                Layout.preferredHeight: 56
                                radius: Theme.r12
                                color: Theme.surfaceHover
                                clip: true
                                Image {
                                    id: packIcon
                                    anchors.fill: parent
                                    source: model.iconUrl
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectCrop
                                    visible: model.iconUrl.length > 0 && status !== Image.Error
                                }
                                Text {
                                    anchors.centerIn: parent
                                    visible: !packIcon.visible
                                    text: model.name.length > 0 ? model.name.charAt(0).toUpperCase() : "?"
                                    color: Theme.textFaint
                                    font.pixelSize: Theme.title
                                    font.weight: Font.DemiBold
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: model.name
                                    color: Theme.text
                                    font.pixelSize: Theme.body
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: model.author.length > 0 ? qsTr("by %1").arg(model.author) : model.slug
                                    color: Theme.textDim
                                    font.pixelSize: Theme.caption
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        Text {
                            text: model.description
                            color: Theme.textDim
                            font.pixelSize: Theme.caption
                            wrapMode: Text.WordWrap
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            verticalAlignment: Text.AlignTop
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp8
                            IconImage {
                                source: "../icons/download.svg"
                                color: Theme.textFaint
                                sourceSize: Qt.size(14, 14)
                            }
                            Text {
                                text: page.formatDownloads(model.downloads)
                                color: Theme.textFaint
                                font.pixelSize: Theme.caption
                            }
                            Text {
                                visible: model.categoriesText.length > 0
                                text: model.categoriesText
                                color: Theme.textFaint
                                font.pixelSize: Theme.caption
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Item { Layout.fillWidth: true; visible: model.categoriesText.length === 0 }
                            BeeButton {
                                btnHeight: 30
                                variant: "secondary"
                                text: qsTr("Install")
                                enabled: !taskRunner.busy
                                opacity: enabled ? 1.0 : 0.5
                                onClicked: discover.install(index)
                            }
                        }
                    }
                }
            }
        }

        // empty state
        ColumnLayout {
            visible: !discover.searching && discover.count === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.sp12
            Item { Layout.fillHeight: true }
            Rectangle {
                width: 96
                height: 96
                radius: 48
                color: Theme.surface
                Layout.alignment: Qt.AlignHCenter
                IconImage {
                    anchors.centerIn: parent
                    source: "../icons/compass.svg"
                    color: Theme.textFaint
                    sourceSize: Qt.size(44, 44)
                    width: 44
                    height: 44
                }
            }
            Text {
                text: page.searched ? qsTr("No modpacks found") : qsTr("Search Modrinth modpacks")
                color: Theme.text
                font.pixelSize: Theme.title
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: page.searched ? qsTr("Try a different search term") : qsTr("Find and install community modpacks")
                color: Theme.textDim
                font.pixelSize: Theme.body
                Layout.alignment: Qt.AlignHCenter
            }
            Item { Layout.fillHeight: true }
        }

        // install progress
        RowLayout {
            visible: taskRunner.busy
            Layout.fillWidth: true
            spacing: Theme.sp12
            ProgressBar {
                Layout.fillWidth: true
                indeterminate: taskRunner.progress < 0
                value: taskRunner.progress >= 0 ? taskRunner.progress : 0
            }
            Text {
                text: taskRunner.statusText
                color: Theme.textDim
                font.pixelSize: Theme.caption
                elide: Text.ElideRight
                Layout.maximumWidth: 320
            }
        }
    }
}
