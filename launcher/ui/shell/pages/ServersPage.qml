import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Servers tab: saved multiplayer servers with live status from the
// `servers` context model (ShellServerModel).
Item {
    id: page

    function openAdd() {
        serverDlg.editRow = -1
        serverDlg.initialName = ""
        serverDlg.initialAddress = ""
        serverDlg.open()
    }
    function openEdit(row, name, address) {
        serverDlg.editRow = row
        serverDlg.initialName = name
        serverDlg.initialAddress = address
        serverDlg.open()
    }
    function openJoinMenu(row, anchor) {
        joinMenu.targetRow = row
        while (joinMenu.count > 0) {
            var item = joinMenu.itemAt(0)
            joinMenu.removeItem(item)
            item.destroy()
        }
        if (typeof instances !== "undefined" && instances) {
            for (var i = 0; i < instances.count; i++) {
                var item = joinItemComponent.createObject(joinMenu, {
                    text: instances.data(instances.index(i, 0), 258),
                    instanceId: instances.instanceIdAt(i)
                })
                joinMenu.addItem(item)
            }
        }
        joinMenu.popup(anchor, 0, anchor.height + 4)
    }

    Component.onCompleted: {
        if (typeof servers !== "undefined" && servers)
            servers.pingAll()
    }

    Connections {
        target: (typeof servers !== "undefined") ? servers : null
        function onErrorOccurred(message) {
            banner.text = message
            errTimer.restart()
        }
    }
    Timer {
        id: errTimer
        interval: 8000
        onTriggered: banner.text = ""
    }

    ServerEditDialog {
        id: serverDlg
        parent: page
    }

    BeeMenu {
        id: joinMenu
        property int targetRow: -1
    }
    Component {
        id: joinItemComponent
        BeeMenuItem {
            property string instanceId: ""
            iconSource: "icons/play.svg"
            onTriggered: if (typeof servers !== "undefined" && servers) servers.join(joinMenu.targetRow, instanceId)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // action row
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 68
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp32
                anchors.rightMargin: Theme.sp32
                spacing: Theme.sp12
                Text {
                    text: qsTr("Saved servers")
                    color: Theme.text
                    font.pixelSize: Theme.title
                    font.weight: Font.DemiBold
                }
                BeeChip {
                    label: (typeof servers !== "undefined" && servers) ? String(servers.count) : "0"
                }
                Item { Layout.fillWidth: true }
                BeeButton {
                    variant: "secondary"
                    btnHeight: 40
                    text: qsTr("Refresh all")
                    iconSource: "icons/refresh.svg"
                    onClicked: if (typeof servers !== "undefined" && servers) servers.pingAll()
                }
                BeeButton {
                    variant: "primary"
                    btnHeight: 40
                    text: qsTr("Add server")
                    iconSource: "icons/add.svg"
                    onClicked: page.openAdd()
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
        }

        // error banner
        Rectangle {
            visible: banner.text.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: banner.implicitHeight + Theme.sp16
            color: Theme.withAlpha(Theme.danger, 0.12)
            Text {
                id: banner
                anchors.centerIn: parent
                width: parent.width - Theme.sp32 * 2
                color: Theme.danger
                font.pixelSize: Theme.body
                wrapMode: Text.WordWrap
            }
        }

        // server list
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.sp8
            model: (typeof servers !== "undefined") ? servers : null
            ScrollBar.vertical: BeeScrollBar {}
            header: Item { height: Theme.sp16; width: 1 }
            footer: Item { height: Theme.sp16; width: 1 }

            delegate: Item {
                id: row
                required property int index
                required property string serverId
                required property string name
                required property string address
                required property string motdText
                required property string versionName
                required property bool online
                required property int playersOnline
                required property int playersMax
                required property int latencyMs
                required property bool pinging
                required property string faviconUrl

                width: ListView.view.width
                implicitHeight: card.implicitHeight

                BeeMenu {
                    id: rowMenu
                    BeeMenuItem {
                        text: qsTr("Edit")
                        iconSource: "icons/edit.svg"
                        onTriggered: page.openEdit(row.index, row.name, row.address)
                    }
                    BeeMenuItem {
                        text: qsTr("Remove")
                        iconSource: "icons/delete.svg"
                        dangerItem: true
                        onTriggered: if (typeof servers !== "undefined" && servers) servers.removeServer(row.index)
                    }
                }

                Rectangle {
                    id: card
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - Theme.sp32 * 2
                    implicitHeight: inner.implicitHeight + Theme.sp24
                    radius: Theme.r16
                    color: cardHover.hovered ? Theme.surfaceHover : Theme.surface
                    border.color: Theme.border
                    Behavior on color { ColorAnimation { duration: Theme.dur(120) } }
                    HoverHandler { id: cardHover }

                    RowLayout {
                        id: inner
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.sp12
                        spacing: Theme.sp12

                        // 48px favicon square
                        Rectangle {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            Layout.alignment: Qt.AlignVCenter
                            radius: Theme.r8
                            color: Theme.surfaceRaised
                            border.color: Theme.border
                            clip: true
                            Image {
                                anchors.fill: parent
                                visible: row.faviconUrl.length > 0
                                source: row.faviconUrl
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                            }
                            IconImage {
                                anchors.centerIn: parent
                                visible: row.faviconUrl.length === 0
                                source: "../icons/globe.svg"
                                color: Theme.textFaint
                                sourceSize: Qt.size(26, 26)
                                width: 26
                                height: 26
                            }
                        }

                        // name / address / motd
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            RowLayout {
                                spacing: Theme.sp8
                                Layout.fillWidth: true
                                Text {
                                    text: row.name
                                    color: Theme.text
                                    font.pixelSize: Theme.bodyLarge
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                BeeChip {
                                    visible: row.online && row.versionName.length > 0
                                    label: row.versionName
                                }
                            }
                            Text {
                                text: row.address
                                color: Theme.textFaint
                                font.pixelSize: Theme.caption
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Text {
                                text: row.pinging ? qsTr("Checking status...")
                                    : (row.motdText.length > 0 ? row.motdText
                                    : (row.online ? "" : qsTr("Offline")))
                                visible: text.length > 0
                                color: row.online ? Theme.textDim : Theme.textFaint
                                font.pixelSize: Theme.caption
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        // live status column
                        ColumnLayout {
                            spacing: Theme.sp4
                            Layout.alignment: Qt.AlignVCenter
                            BeeChip {
                                visible: row.online && row.playersOnline >= 0
                                label: row.playersMax >= 0 ? row.playersOnline + "/" + row.playersMax : String(row.playersOnline)
                                Layout.alignment: Qt.AlignRight
                            }
                            Text {
                                visible: row.online && row.latencyMs >= 0
                                text: qsTr("%1 ms").arg(row.latencyMs)
                                color: Theme.textFaint
                                font.pixelSize: Theme.caption
                                Layout.alignment: Qt.AlignRight
                            }
                            Rectangle {
                                visible: !row.online && !row.pinging
                                implicitWidth: offTxt.implicitWidth + Theme.sp16
                                implicitHeight: 22
                                radius: Theme.r8
                                color: Theme.withAlpha(Theme.danger, 0.12)
                                border.color: Theme.withAlpha(Theme.danger, 0.5)
                                Layout.alignment: Qt.AlignRight
                                Text {
                                    id: offTxt
                                    anchors.centerIn: parent
                                    text: qsTr("Offline")
                                    color: Theme.danger
                                    font.pixelSize: Theme.caption
                                }
                            }
                            Text {
                                visible: row.pinging
                                text: qsTr("Pinging...")
                                color: Theme.textDim
                                font.pixelSize: Theme.caption
                                Layout.alignment: Qt.AlignRight
                            }
                        }

                        // row actions
                        RowLayout {
                            spacing: Theme.sp4
                            Layout.alignment: Qt.AlignVCenter
                            BeeButton {
                                id: joinBtn
                                variant: "primary"
                                btnHeight: 32
                                text: qsTr("Join")
                                iconSource: "icons/play.svg"
                                onClicked: page.openJoinMenu(row.index, joinBtn)
                            }
                            BeeIconButton {
                                id: moreBtn
                                iconSource: "icons/more_horizontal.svg"
                                onClicked: rowMenu.popup(moreBtn, 0, moreBtn.height + 4)
                            }
                        }
                    }
                }
            }

            // empty state
            ColumnLayout {
                visible: list.count === 0
                anchors.centerIn: parent
                spacing: Theme.sp12
                Rectangle {
                    width: 96; height: 96; radius: 48
                    color: Theme.surface
                    Layout.alignment: Qt.AlignHCenter
                    IconImage {
                        anchors.centerIn: parent
                        source: "../icons/server.svg"
                        color: Theme.textFaint
                        sourceSize: Qt.size(44, 44)
                        width: 44; height: 44
                    }
                }
                Text {
                    text: qsTr("No saved servers")
                    color: Theme.text
                    font.pixelSize: Theme.title
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: qsTr("Save a multiplayer server to see its live status here")
                    color: Theme.textDim
                    font.pixelSize: Theme.body
                    Layout.alignment: Qt.AlignHCenter
                }
                BeeButton {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.sp8
                    variant: "primary"
                    text: qsTr("Add server")
                    iconSource: "icons/add.svg"
                    onClicked: page.openAdd()
                }
            }
        }
    }
}
