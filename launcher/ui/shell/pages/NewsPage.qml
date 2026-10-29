import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// News page. Expects the ShellNewsModel context property "news".
Item {
    id: page

    Component.onCompleted: {
        if (news.count === 0 && !news.busy)
            news.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp32
        spacing: Theme.sp16

        // page header
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp12
            Text {
                text: qsTr("Latest Minecraft news")
                color: Theme.text
                font.pixelSize: Theme.headline
                font.weight: Font.DemiBold
            }
            BusyIndicator {
                visible: news.busy
                running: visible
                implicitWidth: 24
                implicitHeight: 24
                palette.dark: Theme.accent
            }
            Item { Layout.fillWidth: true }
            BeeButton {
                variant: "secondary"
                text: qsTr("Refresh")
                iconSource: "icons/refresh.svg"
                onClicked: news.refresh()
            }
        }

        // article list
        ListView {
            id: newsList
            visible: news.count > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.sp16
            model: news
            ScrollBar.vertical: BeeScrollBar {}

            delegate: ItemDelegate {
                id: article
                width: newsList.width
                implicitHeight: 150
                contentItem: RowLayout {
                    spacing: Theme.sp16
                    Rectangle {
                        Layout.preferredWidth: 240
                        Layout.fillHeight: true
                        radius: Theme.r12
                        color: Theme.surfaceRaised
                        clip: true
                        Image {
                            id: articleImage
                            anchors.fill: parent
                            source: model.imageUrl
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            visible: status === Image.Ready
                        }
                        IconImage {
                            anchors.centerIn: parent
                            visible: articleImage.status !== Image.Ready
                            source: "../icons/image.svg"
                            color: Theme.textFaint
                            sourceSize: Qt.size(28, 28)
                            width: 28
                            height: 28
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Theme.sp4
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp8
                            BeeChip {
                                label: model.tag.length > 0 ? model.tag : model.category
                                visible: label.length > 0
                            }
                            Text {
                                text: model.date
                                color: Theme.textFaint
                                font.pixelSize: Theme.caption
                            }
                            Item { Layout.fillWidth: true }
                        }
                        Text {
                            text: model.title
                            color: Theme.text
                            font.pixelSize: Theme.bodyLarge
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: model.text
                            color: Theme.textDim
                            font.pixelSize: Theme.body
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Item { Layout.fillHeight: true }
                        Text {
                            text: qsTr("Read more")
                            color: Theme.accent
                            font.pixelSize: Theme.caption
                            font.weight: Font.DemiBold
                            visible: model.link.length > 0
                        }
                    }
                }
                background: Rectangle {
                    radius: Theme.r12
                    color: article.hovered ? Theme.surfaceHover : Theme.surface
                    border.width: 1
                    border.color: article.hovered ? Theme.accentAlpha(0.45) : Theme.border
                }
                onClicked: if (model.link.length > 0) news.openLink(model.link)
            }
        }

        // empty state
        ColumnLayout {
            visible: news.count === 0 && !news.busy
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
                    source: "../icons/news.svg"
                    color: Theme.textFaint
                    sourceSize: Qt.size(44, 44)
                    width: 44
                    height: 44
                }
            }
            Text {
                text: news.lastError.length > 0 ? qsTr("Could not load news") : qsTr("No news yet")
                color: Theme.text
                font.pixelSize: Theme.title
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: news.lastError.length > 0 ? news.lastError : qsTr("Check back later")
                color: Theme.textDim
                font.pixelSize: Theme.body
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 420
            }
            BeeButton {
                visible: news.lastError.length > 0
                variant: "secondary"
                text: qsTr("Try again")
                iconSource: "icons/refresh.svg"
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Theme.sp8
                onClicked: news.refresh()
            }
            Item { Layout.fillHeight: true }
        }
    }
}
