import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Qt Quick shell window. Home, Library, Servers, Discover, Accounts and
// Settings are fully native; heavyweight wizards still delegate to the
// classic Widgets dialogs.
ApplicationWindow {
    id: root
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    title: "Bee Launcher"
    color: Theme.background
    palette.text: Theme.text
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.buttonText: Theme.text
    palette.placeholderText: Theme.textFaint
    palette.highlight: Theme.accent
    palette.dark: Theme.text

    property int pageIndex: 0
    property bool lightMode: false
    property string selectedId: ""
    property var selectedData: null
    property string detailId: ""

    function toast(message) {
        if (!message || String(message).length === 0)
            return
        toastText.text = message
        toastTimer.restart()
        toastBox.visible = true
    }

    onLightModeChanged: {
        Theme.dark = !lightMode
        shellSettings.setValue("ShellLightMode", lightMode)
    }

    Component.onCompleted: {
        var light = shellSettings.value("ShellLightMode")
        if (light === undefined || light === null)
            light = false
        lightMode = light === true || light === "true"
        Theme.dark = !lightMode
    }

    FontLoader { id: uiFont; source: "fonts/Inter.ttf" }
    font.family: uiFont.status === FontLoader.Ready ? uiFont.name : "sans-serif"

    readonly property var pageTitles: [qsTr("Home"), qsTr("Library"), qsTr("Servers"), qsTr("Discover"), qsTr("News"), qsTr("Accounts"), qsTr("Settings")]
    readonly property var pageIcons: ["home.svg", "collections.svg", "server.svg", "compass.svg", "news.svg", "person.svg", "settings.svg"]

    // Bumped whenever the instance model changes so bindings that read roles
    // through instances.data() re-evaluate.
    property int instancesRev: 0

    function instData(row, role) {
        void root.instancesRev
        return instances.data(instances.index(row, 0), role)
    }

    function groupCount(g) {
        void root.instancesRev
        var n = 0
        for (var i = 0; i < instances.count; i++)
            if (instances.groupAt(i) === g) n++
        return n
    }
    function rowsInGroup(g) {
        void root.instancesRev
        var rows = []
        for (var i = 0; i < instances.count; i++)
            if (instances.groupAt(i) === g) rows.push(i)
        return rows
    }

    function refreshSelected() {
        if (!root.selectedId)
            return
        for (var i = 0; i < instances.count; i++) {
            if (instances.instanceIdAt(i) === root.selectedId) {
                root.pickInstance(i)
                return
            }
        }
        root.selectedId = ""
        root.selectedData = null
    }

    function openCardMenu(card, x, y) {
        cardMenu.targetId = card.instanceId
        cardMenu.targetRunning = card.running
        cardMenu.popup(card, x, y)
    }

    function pickInstance(idx) {
        var iid = instances.instanceIdAt(idx)
        root.selectedId = iid
        root.selectedData = {
            "instanceId": iid,
            "name": instances.data(instances.index(idx, 0), 258),
            "versionText": instances.data(instances.index(idx, 0), 261),
            "iconSource": instances.data(instances.index(idx, 0), 260),
            "lastPlayedMs": instances.data(instances.index(idx, 0), 262),
            "totalPlayTimeMs": instances.data(instances.index(idx, 0), 263),
            "running": instances.data(instances.index(idx, 0), 264)
        }
    }

    function formatPlayTime(ms) {
        var mins = Math.floor(ms / 60000)
        if (mins < 60) return qsTr("%1 min").arg(mins)
        return qsTr("%1 h").arg(Math.floor(mins / 60))
    }
    function formatLastPlayed(ms) {
        if (!ms) return qsTr("Never")
        var diff = Date.now() - ms
        var mins = Math.floor(diff / 60000)
        if (mins < 60) return qsTr("%1m ago").arg(Math.max(1, mins))
        var hrs = Math.floor(mins / 60)
        if (hrs < 24) return qsTr("%1h ago").arg(hrs)
        return qsTr("%1d ago").arg(Math.floor(hrs / 24))
    }

    BeeMenu {
        id: cardMenu
        property string targetId: ""
        property bool targetRunning: false
        BeeMenuItem { text: cardMenu.targetRunning ? qsTr("Stop") : qsTr("Launch"); iconSource: cardMenu.targetRunning ? "icons/stop.svg" : "icons/play.svg"; onTriggered: cardMenu.targetRunning ? controller.kill(cardMenu.targetId) : controller.launch(cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Details"); iconSource: "icons/info.svg"; onTriggered: root.detailId = cardMenu.targetId }
        BeeMenuItem { text: qsTr("Edit"); iconSource: "icons/edit.svg"; onTriggered: controller.edit(cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Open folder"); iconSource: "icons/folder.svg"; onTriggered: controller.openFolder(cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Rename"); iconSource: "icons/edit.svg"; onTriggered: textDlg.openWith(qsTr("Rename instance"), cardMenu.targetId, cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Move to group"); iconSource: "icons/folder.svg"; onTriggered: { textDlg.field2Label = ""; textDlg.openWith(qsTr("Move to group"), cardMenu.targetId, "") } }
        BeeMenuItem { text: qsTr("Copy"); iconSource: "icons/copy.svg"; onTriggered: { textDlg.field2Label = qsTr("Group"); textDlg.openWith(qsTr("Copy instance"), cardMenu.targetId, "") } }
        BeeMenuItem { text: qsTr("Backup now"); iconSource: "icons/history.svg"; onTriggered: controller.backupNow(cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Repair"); iconSource: "icons/wrench.svg"; onTriggered: controller.repair(cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Create shortcut"); iconSource: "icons/add.svg"; onTriggered: controller.createShortcut(cardMenu.targetId) }
        BeeMenuItem { text: qsTr("Delete"); iconSource: "icons/delete.svg"; dangerItem: true; onTriggered: controller.deleteInstance(cardMenu.targetId) }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // navigation rail
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: root.width >= 1100 ? 232 : 76
            color: Theme.rail
            clip: true
            Behavior on Layout.preferredWidth { NumberAnimation { duration: Theme.dur(180) } }
            Rectangle { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.topLevel ? undefined : parent.bottom; height: parent.height; width: 1; color: Theme.border }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: Theme.sp16
                anchors.bottomMargin: Theme.sp16
                spacing: 0

                RowLayout {
                    Layout.leftMargin: 18
                    Layout.bottomMargin: Theme.sp16
                    spacing: Theme.sp12
                    Image { source: "bee_logo.png"; sourceSize: Qt.size(36, 36); Layout.preferredWidth: 36; Layout.preferredHeight: 36 }
                    Text { visible: root.width >= 1100; text: "Bee Launcher"; color: Theme.text; font.pixelSize: Theme.bodyLarge; font.weight: Font.DemiBold }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        spacing: Theme.sp4
                        Repeater {
                            id: navRepeater
                            model: root.pageTitles.length
                            BeeNavButton {
                                Layout.fillWidth: true
                                text: root.pageTitles[index]
                                iconSource: "icons/" + root.pageIcons[index]
                                active: root.pageIndex === index
                                collapsed: root.width < 1100
                                onActivated: root.pageIndex = index
                            }
                        }
                    }
                    // animated selection pill
                    Rectangle {
                        width: 3; height: 20; radius: 2
                        color: Theme.accent
                        x: 8
                        y: navRepeater.count > root.pageIndex ? navRepeater.itemAt(root.pageIndex).y + 14 : 14
                        Behavior on y { NumberAnimation { duration: Theme.dur(220); easing.type: Easing.OutCubic } }
                    }
                }

                // account chip
                Rectangle {
                    Layout.fillWidth: true
                    Layout.margins: Theme.sp8
                    implicitHeight: 52
                    radius: Theme.r12
                    color: acctArea.containsMouse ? Theme.surfaceHover : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp8
                        spacing: Theme.sp8
                        Rectangle {
                            Layout.preferredWidth: 32; Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            radius: 16
                            color: Theme.accentAlpha(0.25)
                            clip: true
                            Image {
                                id: acctFace
                                anchors.fill: parent
                                source: controller.accountFace
                                sourceSize: Qt.size(32, 32)
                                visible: status === Image.Ready
                            }
                            Text {
                                anchors.centerIn: parent
                                visible: acctFace.status !== Image.Ready
                                text: controller.accountName.length > 0 ? controller.accountName.charAt(0).toUpperCase() : "?"
                                color: Theme.accent
                                font.weight: Font.DemiBold
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: root.width >= 1100
                            spacing: 0
                            Text { text: controller.accountName; color: Theme.text; font.pixelSize: Theme.body; font.weight: Font.Medium; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: controller.accountType; color: Theme.textFaint; font.pixelSize: Theme.caption; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                    }
                    MouseArea { id: acctArea; anchors.fill: parent; hoverEnabled: true; onClicked: root.pageIndex = 5 }
                }
            }
        }

        // content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // top bar
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp24
                    anchors.rightMargin: Theme.sp24
                    spacing: Theme.sp12
                    Text {
                        text: root.pageTitles[root.pageIndex]
                        color: Theme.text
                        font.pixelSize: Theme.headline
                        font.weight: Font.DemiBold
                    }
                    Item { Layout.fillWidth: true }
                    BeeTextField { visible: root.pageIndex === 1; Layout.preferredWidth: 320; placeholderText: qsTr("Search instances"); onTextChanged: instances.filterText = text }
                    BeeComboBox { visible: root.pageIndex === 1; Layout.preferredWidth: 170; onActivated: function(i) { instances.sortMode = i } }
                    BeeButton { visible: root.pageIndex <= 3; variant: "primary"; btnHeight: 40; text: qsTr("New instance"); iconSource: "icons/add.svg"; onClicked: newInstanceDialog.open() }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            }

            // pages
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: root.pageIndex

                        // Home
                        Flickable {
                            clip: true
                            contentWidth: width
                            contentHeight: homeCol.implicitHeight + 48
                            ScrollBar.vertical: BeeScrollBar {}
                            ColumnLayout {
                                id: homeCol
                                width: parent.width - Theme.sp32 * 2
                                x: Theme.sp32
                                spacing: Theme.sp24
                                Item { height: Theme.sp24; width: 1 }
                                // hero card
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 280
                                    radius: Theme.r24
                                    visible: instances.count > 0
                                    clip: true
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0.0; color: Theme.coverColors(instances.count ? instances.instanceIdAt(0) : "x")[0] }
                                        GradientStop { position: 1.0; color: Theme.coverColors(instances.count ? instances.instanceIdAt(0) : "x")[1] }
                                    }
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: Theme.sp32
                                        spacing: Theme.sp24
                                        ColumnLayout {
                                            spacing: Theme.sp12
                                            Text { text: qsTr("Continue playing"); color: Theme.accent; font.pixelSize: Theme.caption; font.weight: Font.DemiBold }
                                            Text { text: instances.count ? root.instData(0, 258) : ""; color: "#ffffff"; font.pixelSize: Theme.display; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.maximumWidth: 700 }
                                            RowLayout {
                                                spacing: Theme.sp12
                                                BeeChip { label: instances.count ? root.instData(0, 261) : "" }
                                                Text { text: instances.count ? root.formatLastPlayed(root.instData(0, 262)) : ""; color: Qt.rgba(1,1,1,0.7); font.pixelSize: Theme.caption }
                                            }
                                            RowLayout {
                                                spacing: Theme.sp12
                                                Layout.topMargin: Theme.sp8
                                                BeeButton {
                                                    btnHeight: 48
                                                    variant: "primary"
                                                    text: qsTr("Play")
                                                    iconSource: "icons/play.svg"
                                                    onClicked: if (instances.count) controller.launch(instances.instanceIdAt(0))
                                                }
                                                BeeButton {
                                                    btnHeight: 48
                                                    variant: "ghost"
                                                    text: qsTr("Open instance")
                                                    onClicked: if (instances.count) controller.edit(instances.instanceIdAt(0))
                                                }
                                            }
                                            Item { Layout.fillHeight: true }
                                        }
                                        Item { Layout.fillWidth: true }
                                        Image {
                                            Layout.preferredWidth: 180
                                            Layout.preferredHeight: 180
                                            Layout.alignment: Qt.AlignVCenter
                                            source: instances.count ? root.instData(0, 260) : ""
                                            fillMode: Image.PreserveAspectFit
                                            asynchronous: true
                                            opacity: Theme.dark ? 0.9 : 1.0
                                        }
                                    }
                                }
                                // recent strip
                                ColumnLayout {
                                    visible: instances.count > 0
                                    spacing: Theme.sp12
                                    Layout.fillWidth: true
                                    Text { text: qsTr("Recent instances"); color: Theme.text; font.pixelSize: Theme.title; font.weight: Font.DemiBold }
                                    ListView {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 200
                                        orientation: ListView.Horizontal
                                        spacing: Theme.sp16
                                        clip: true
                                        model: instances
                                        delegate: BeeCard {
                                            compact: true
                                            width: 200
                                            height: 196
                                            controller: root.controller
                                            instanceId: model.instanceId
                                            name: model.name
                                            iconSource: model.iconSource
                                            versionText: model.versionText
                                            lastPlayedMs: model.lastPlayedMs
                                            running: model.running
                                            onClickedCard: root.pickInstance(index)
                                            onPlayClicked: running ? controller.kill(instanceId) : controller.launch(instanceId)
                                            onMenuRequested: function(x, y) { root.openCardMenu(this, x, y) }
                                        }
                                    }
                                }
                                // empty state
                                ColumnLayout {
                                    visible: !instances || instances.count === 0
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.topMargin: 120
                                    spacing: Theme.sp12
                                    Rectangle {
                                        width: 96; height: 96; radius: 48
                                        color: Theme.surface
                                        Layout.alignment: Qt.AlignHCenter
                                        IconImage { anchors.centerIn: parent; source: "icons/cube.svg"; color: Theme.textFaint; sourceSize: Qt.size(44,44); width: 44; height: 44 }
                                    }
                                    Text { text: qsTr("No instances yet"); color: Theme.text; font.pixelSize: Theme.title; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                    Text { text: qsTr("Create your first instance to start playing"); color: Theme.textDim; font.pixelSize: Theme.body; Layout.alignment: Qt.AlignHCenter }
                                    BeeButton { Layout.alignment: Qt.AlignHCenter; Layout.topMargin: Theme.sp8; variant: "primary"; text: qsTr("New instance"); iconSource: "icons/add.svg"; onClicked: controller.addInstance() }
                                }
                            }
                        }

                        // Library
                        Flickable {
                            id: libFlick
                            clip: true
                            contentWidth: width
                            contentHeight: libCol.height + Theme.sp32
                            ScrollBar.vertical: BeeScrollBar {}
                            ColumnLayout {
                                id: libCol
                                x: Theme.sp32
                                y: Theme.sp32
                                width: libFlick.width - Theme.sp32 * 2
                                spacing: Theme.sp24

                                Repeater {
                                    model: instances.groups
                                    delegate: ColumnLayout {
                                        required property string modelData
                                        property string grp: modelData
                                        Layout.fillWidth: true
                                        spacing: Theme.sp12
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: Theme.sp12
                                            Text {
                                                text: grp.length ? grp : qsTr("Ungrouped")
                                                color: Theme.text
                                                font.pixelSize: Theme.bodyLarge
                                                font.weight: Font.DemiBold
                                            }
                                            BeeChip { label: String(groupCount(grp)) }
                                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                                        }
                                        Flow {
                                            Layout.fillWidth: true
                                            spacing: Theme.sp16
                                            Repeater {
                                                model: root.rowsInGroup(grp)
                                                delegate: BeeCard {
                                                    objectName: "instanceCard"
                                                    required property int modelData
                                                    property int row: modelData
                                                    width: libCol.width > 0 ? Math.floor((libCol.width + Theme.sp16) / Math.max(1, Math.floor((libCol.width + Theme.sp16) / 256))) - Theme.sp16 : 240
                                                    height: 228
                                                    instanceId: root.instData(row, 257)
                                                    name: root.instData(row, 258)
                                                    iconSource: root.instData(row, 260)
                                                    versionText: root.instData(row, 261)
                                                    lastPlayedMs: root.instData(row, 262)
                                                    running: root.instData(row, 264)
                                                    selected: root.selectedId === instanceId
                                                    onClickedCard: root.pickInstance(row)
                                                    onPlayClicked: running ? controller.kill(instanceId) : controller.launch(instanceId)
                                                    onMenuRequested: function(x, y) { root.openCardMenu(this, x, y) }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Servers
                        ServersPage { }

                        // Discover
                        DiscoverPage { }

                        // News
                        NewsPage { }

                        // Accounts
                        AccountsPage { }

                        // Settings
                        SettingsPage {
                            onRequestNav: function(index) { root.pageIndex = index }
                        }
                    }

                    // detail panel
                    Rectangle {
                        visible: root.selectedId.length > 0 && root.pageIndex === 1
                        Layout.fillHeight: true
                        Behavior on Layout.preferredWidth { NumberAnimation { duration: Theme.dur(220) } }
                        Layout.preferredWidth: visible ? 340 : 0
                        color: Theme.surface
                        clip: true
                        Rectangle { anchors.left: parent.left; height: parent.height; width: 1; color: Theme.border }
                        Flickable {
                            anchors.fill: parent
                            contentHeight: detailCol.implicitHeight
                            clip: true
                            ColumnLayout {
                                id: detailCol
                                width: parent.width
                                spacing: Theme.sp16

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 190
                                    Rectangle {
                                        anchors.fill: parent
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0.0; color: Theme.coverColors(root.selectedId)[0] }
                                            GradientStop { position: 1.0; color: Theme.coverColors(root.selectedId)[1] }
                                        }
                                    }
                                    Image {
                                        anchors.centerIn: parent
                                        width: 96; height: 96
                                        source: root.selectedData ? root.selectedData.iconSource : ""
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                    }
                                    BeeIconButton {
                                        anchors.right: parent.right; anchors.top: parent.top
                                        anchors.margins: Theme.sp8
                                        iconSource: "icons/dismiss.svg"
                                        onClicked: { root.selectedId = ""; root.selectedData = null }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: Theme.sp24
                                    Layout.rightMargin: Theme.sp24
                                    spacing: Theme.sp8
                                    Text { text: root.selectedData ? root.selectedData.name : ""; color: Theme.text; font.pixelSize: Theme.title; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
                                    BeeChip { label: root.selectedData ? root.selectedData.versionText : "" }
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: Theme.sp24
                                    Layout.rightMargin: Theme.sp24
                                    columns: 2
                                    columnSpacing: Theme.sp16
                                    rowSpacing: Theme.sp12
                                    Repeater {
                                        model: [
                                            [qsTr("Last played"), root.selectedData ? root.formatLastPlayed(root.selectedData.lastPlayedMs) : ""],
                                            [qsTr("Total play time"), root.selectedData ? root.formatPlayTime(root.selectedData.totalPlayTimeMs) : ""]
                                        ]
                                        delegate: ColumnLayout {
                                            required property var modelData
                                            spacing: 2
                                            Text { text: modelData[0]; color: Theme.textDim; font.pixelSize: Theme.caption }
                                            Text { text: modelData[1]; color: Theme.text; font.pixelSize: Theme.body; font.weight: Font.Medium }
                                        }
                                    }
                                }

                                BeeButton {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: Theme.sp24
                                    Layout.rightMargin: Theme.sp24
                                    btnHeight: 48
                                    variant: root.selectedData && root.selectedData.running ? "danger" : "primary"
                                    text: root.selectedData && root.selectedData.running ? qsTr("Stop") : qsTr("Play")
                                    iconSource: root.selectedData && root.selectedData.running ? "icons/stop.svg" : "icons/play.svg"
                                    onClicked: root.selectedData && root.selectedData.running ? controller.kill(root.selectedId) : controller.launch(root.selectedId)
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: Theme.sp24
                                    Layout.rightMargin: Theme.sp24
                                    columns: 4
                                    columnSpacing: Theme.sp8
                                    Repeater {
                                        model: [
                                            [qsTr("Edit"), "icons/edit.svg", "edit"],
                                            [qsTr("Folder"), "icons/folder.svg", "folder"],
                                            [qsTr("Backup"), "icons/history.svg", "backup"],
                                            [qsTr("Repair"), "icons/wrench.svg", "repair"]
                                        ]
                                        delegate: ColumnLayout {
                                            required property var modelData
                                            spacing: Theme.sp4
                                            Layout.alignment: Qt.AlignHCenter
                                            BeeIconButton {
                                                Layout.alignment: Qt.AlignHCenter
                                                iconSource: modelData[1]
                                                onClicked: {
                                                    if (modelData[2] === "edit") controller.edit(root.selectedId)
                                                    else if (modelData[2] === "folder") controller.openFolder(root.selectedId)
                                                    else if (modelData[2] === "backup") controller.backupNow(root.selectedId)
                                                    else controller.repair(root.selectedId)
                                                }
                                            }
                                            Text { text: modelData[0]; color: Theme.textDim; font.pixelSize: Theme.caption; Layout.alignment: Qt.AlignHCenter }
                                        }
                                    }
                                }

                                BeeButton {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: Theme.sp24
                                    Layout.rightMargin: Theme.sp24
                                    Layout.bottomMargin: Theme.sp24
                                    variant: "secondary"
                                    text: qsTr("More actions")
                                    onClicked: cardMenu.popup(moreBtn, 0, moreBtn.height + 4)
                                    id: moreBtn
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // instance detail overlay
    Loader {
        id: detailLoader
        anchors.fill: parent
        active: root.detailId.length > 0
        sourceComponent: InstanceDetailPage {
            instanceId: root.detailId
            onClosed: root.detailId = ""
        }
    }

    // new instance wizard
    NewInstanceDialog {
        id: newInstanceDialog
    }

    // rename / group / copy prompt
    InstanceTextDialog {
        id: textDlg
        property string mode: ""
        onAcceptedText: function(v1, v2) {
            if (mode === "copy")
                createCtrl.copyInstance(targetId, v1, v2)
            else if (mode === "group")
                controller.changeGroup(targetId, v1)
            else
                createCtrl.renameInstance(targetId, v1)
        }
        function openWith(t, id, v1) {
            if (t === qsTr("Copy instance")) mode = "copy"
            else if (t === qsTr("Move to group")) mode = "group"
            else mode = "rename"
            title = t
            targetId = id
            field1Text = v1
            field2Text = ""
            open()
        }
    }

    // background task banner
    Rectangle {
        visible: taskRunner.busy
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 44
        color: Theme.surfaceRaised
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp16
            anchors.rightMargin: Theme.sp16
            spacing: Theme.sp12
            Text { text: taskRunner.statusText; color: Theme.text; font.pixelSize: Theme.body; elide: Text.ElideRight; Layout.fillWidth: true }
            ProgressBar {
                Layout.preferredWidth: 200
                indeterminate: taskRunner.progress < 0
                value: taskRunner.progress < 0 ? 0 : taskRunner.progress
            }
            BeeIconButton { iconSource: "icons/dismiss.svg"; onClicked: taskRunner.abort() }
        }
    }

    // toast
    Rectangle {
        id: toastBox
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: taskRunner.busy ? 60 : Theme.sp24
        width: toastText.implicitWidth + Theme.sp32
        height: toastText.implicitHeight + Theme.sp16
        radius: Theme.r12
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.border
        Text {
            id: toastText
            anchors.centerIn: parent
            color: Theme.text
            font.pixelSize: Theme.body
        }
        Timer { id: toastTimer; interval: 4000; onTriggered: toastBox.visible = false }
    }

    Connections {
        target: controller
        function onToast(message) { root.toast(message) }
    }
    Connections {
        target: instances
        function onDataChanged() { root.instancesRev++; root.refreshSelected() }
        function onModelReset() { root.instancesRev++; root.refreshSelected() }
        function onRowsInserted() { root.instancesRev++ }
        function onRowsRemoved() { root.instancesRev++ }
        function onLayoutChanged() { root.instancesRev++ }
    }
    Connections {
        target: news
        function onFetchFailed(reason) { root.toast(reason) }
    }
    Connections {
        target: servers
        function onErrorOccurred(message) { root.toast(message) }
    }
    Connections {
        target: accounts
        function onMsaFailed(reason) { root.toast(reason) }
        function onElybyFailed(reason) { root.toast(reason) }
    }
    Connections {
        target: instanceBridge
        function onActionFailed(message) { root.toast(message) }
    }
    Connections {
        target: taskRunner
        function onTaskFailed(reason) { root.toast(reason) }
    }
    Connections {
        target: createCtrl
        function onCreateFailed(reason) { root.toast(reason) }
        function onInstanceCreated() { root.toast(qsTr("Instance created")) }
    }
    Connections {
        target: discover
        function onSearchFailed(reason) { root.toast(reason) }
        function onInstallFailed(reason) { root.toast(reason) }
    }
}
