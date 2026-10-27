import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell
import "settings"

// Full settings surface. Left column picks a category, right side shows the
// matching form. All edits are applied to the SettingsObject immediately.
Item {
    id: page
    signal requestNav(int index)

    // Nav index of the Accounts page in the shell StackLayout.
    property int accountsNavIndex: 4
    property int category: 0

    function backend() { return typeof shellSettings !== "undefined" ? shellSettings : null }

    // iconThemes()/appThemes() return { id, name } maps; ComboRow wants { name, value }
    function themeOptions(which) {
        var b = backend()
        var list = []
        if (b)
            list = which === "icon" ? b.iconThemes() : b.appThemes()
        var out = []
        for (var i = 0; i < list.length; i++) {
            var t = list[i]
            out.push({ "name": String(t.name), "value": String(t.id) })
        }
        return out
    }

    function currentContentHeight() {
        var item = catStack.children[catStack.currentIndex]
        return item ? item.implicitHeight + Theme.sp32 : 0
    }

    readonly property var cats: [
        { "name": qsTr("General"), "icon": "../icons/settings.svg" },
        { "name": qsTr("Appearance"), "icon": "../icons/image.svg" },
        { "name": qsTr("Java and Memory"), "icon": "../icons/server.svg" },
        { "name": qsTr("Minecraft"), "icon": "../icons/cube.svg" },
        { "name": qsTr("Native and Performance"), "icon": "../icons/puzzle.svg" },
        { "name": qsTr("Console"), "icon": "../icons/log.svg" },
        { "name": qsTr("Downloads and Folders"), "icon": "../icons/download.svg" },
        { "name": qsTr("Proxy"), "icon": "../icons/globe.svg" },
        { "name": qsTr("Integrations"), "icon": "../icons/link.svg" },
        { "name": qsTr("Accounts"), "icon": "../icons/person.svg" },
        { "name": qsTr("About"), "icon": "../icons/info.svg" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // category column
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            color: Theme.rail
            Rectangle { anchors.right: parent.right; height: parent.height; width: 1; color: Theme.border }
            ListView {
                id: catList
                anchors.fill: parent
                anchors.margins: Theme.sp8
                clip: true
                model: page.cats
                spacing: Theme.sp4
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: catList.width
                    height: 40
                    onClicked: page.category = index
                    contentItem: RowLayout {
                        spacing: Theme.sp12
                        IconImage {
                            source: modelData.icon
                            color: page.category === index ? Theme.accent : Theme.textDim
                            sourceSize: Qt.size(18, 18)
                            Layout.leftMargin: Theme.sp8
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                        }
                        Text {
                            text: modelData.name
                            color: page.category === index ? Theme.text : Theme.textDim
                            font.pixelSize: Theme.body
                            font.weight: page.category === index ? Font.DemiBold : Font.Normal
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    background: Rectangle {
                        radius: Theme.r8
                        color: page.category === index ? Theme.accentAlpha(0.14)
                             : (hovered ? Theme.surfaceHover : "transparent")
                    }
                }
            }
        }

        // form area
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: page.currentContentHeight()
            ScrollBar.vertical: BeeScrollBar {}

            StackLayout {
                id: catStack
                x: Theme.sp32
                width: parent.width - Theme.sp32 * 2
                currentIndex: page.category

                // 0: General
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Folders and language") }
                    PathRow {
                        key: "InstanceDir"
                        label: qsTr("Instance folder")
                        subtitle: qsTr("Where instances are stored. Relative paths resolve under the data root.")
                    }
                    BoolRow {
                        key: "UseSystemLocale"
                        label: qsTr("Use system locale")
                        subtitle: qsTr("Use the operating system language instead of an override.")
                    }
                    TextRow {
                        key: "Language"
                        label: qsTr("Language override")
                        subtitle: qsTr("Language code used when the system locale is disabled.")
                        placeholder: qsTr("e.g. en_US")
                    }

                    BeeSectionHeader { text: qsTr("Interface") }
                    ComboRow {
                        key: "UiMode"
                        label: qsTr("Interface mode")
                        subtitle: qsTr("Takes effect after restart.")
                        options: [
                            { "name": qsTr("New shell"), "value": "shell" },
                            { "name": qsTr("Classic"), "value": "classic" }
                        ]
                    }
                    BeeFieldRow {
                        label: qsTr("Classic interface")
                        subtitle: qsTr("Switch back to the classic interface after restart.")
                        BeeButton {
                            variant: "secondary"
                            text: qsTr("Switch to classic now")
                            onClicked: { var b = page.backend(); if (b) b.useClassicUi() }
                        }
                    }
                    BoolRow {
                        key: "StatusBarVisible"
                        label: qsTr("Show the status bar")
                    }
                    BoolRow {
                        key: "MenuBarInsteadOfToolBar"
                        label: qsTr("Use a menu bar instead of the toolbar")
                    }

                    BeeSectionHeader { text: qsTr("Tasks and network") }
                    IntRow {
                        key: "NumberOfConcurrentTasks"
                        label: qsTr("Concurrent tasks")
                        from: 1; to: 64
                    }
                    IntRow {
                        key: "NumberOfConcurrentDownloads"
                        label: qsTr("Concurrent downloads")
                        from: 1; to: 32
                    }
                    IntRow {
                        key: "NumberOfManualRetries"
                        label: qsTr("Manual retries")
                        from: 0; to: 10
                    }
                    IntRow {
                        key: "NumberOfAutoRetries"
                        label: qsTr("Automatic retries")
                        from: 0; to: 20
                    }
                    IntRow {
                        key: "RequestTimeout"
                        label: qsTr("Request timeout")
                        from: 5; to: 600; suffix: qsTr("s")
                    }

                    BeeSectionHeader { text: qsTr("Instances") }
                    ComboRow {
                        key: "InstRenamingMode"
                        label: qsTr("Instance folder renaming")
                        subtitle: qsTr("What to do when an instance is renamed.")
                        options: [
                            { "name": qsTr("Ask every time"), "value": "AskEverytime" },
                            { "name": qsTr("Always rename the folder"), "value": "PhysicalDir" },
                            { "name": qsTr("Never rename the folder"), "value": "MetadataOnly" }
                        ]
                    }
                    BoolRow {
                        key: "CloseAfterLaunch"
                        label: qsTr("Close the launcher after the game starts")
                    }
                    BoolRow {
                        key: "QuitAfterGameStop"
                        label: qsTr("Quit the launcher when the game stops")
                    }
                    BoolRow {
                        key: "SkipModpackUpdatePrompt"
                        label: qsTr("Skip modpack update prompts")
                    }
                }

                // 1: Appearance
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Shell") }
                    BeeFieldRow {
                        label: qsTr("Dark mode")
                        subtitle: qsTr("Applies to the shell interface immediately.")
                        BeeCheckBox {
                            id: darkChk
                            checked: Theme.dark
                            onToggled: {
                                Theme.dark = checked
                                var b = page.backend()
                                if (b)
                                    b.setValue("ShellLightMode", !checked)
                            }
                            Connections {
                                target: Theme
                                function onDarkChanged() { darkChk.checked = Theme.dark }
                            }
                        }
                    }

                    BeeSectionHeader { text: qsTr("Classic interface themes") }
                    ComboRow {
                        key: "IconTheme"
                        label: qsTr("Icon theme")
                        subtitle: qsTr("Used by the classic interface.")
                        options: page.themeOptions("icon")
                    }
                    ComboRow {
                        key: "ApplicationTheme"
                        label: qsTr("Application theme")
                        subtitle: qsTr("Widget style used by the classic interface.")
                        options: page.themeOptions("app")
                    }

                    BeeSectionHeader { text: qsTr("Instances") }
                    ComboRow {
                        key: "InstSortMode"
                        label: qsTr("Sort instances by")
                        options: [
                            { "name": qsTr("Name"), "value": "Name" },
                            { "name": qsTr("Last launch"), "value": "LastLaunch" }
                        ]
                    }
                    BoolRow {
                        key: "Snow"
                        label: qsTr("Snow effect")
                        subtitle: qsTr("Seasonal snow animation in the instance view.")
                    }
                }

                // 2: Java and Memory
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Memory") }
                    SliderRow {
                        key: "MinMemAlloc"
                        label: qsTr("Minimum memory")
                        from: 256; to: 16384; stepSize: 128; suffix: qsTr("MiB")
                    }
                    SliderRow {
                        key: "MaxMemAlloc"
                        label: qsTr("Maximum memory")
                        from: 256; to: 16384; stepSize: 128; suffix: qsTr("MiB")
                    }
                    IntRow {
                        key: "PermGen"
                        label: qsTr("PermGen size")
                        subtitle: qsTr("Only used by Java 7 and older.")
                        from: 32; to: 2048; suffix: qsTr("MiB")
                    }

                    BeeSectionHeader { text: qsTr("Java runtime") }
                    TextRow {
                        key: "JavaPath"
                        label: qsTr("Java path")
                        subtitle: qsTr("Leave empty to select Java automatically.")
                        placeholder: "/usr/bin/java"
                    }
                    TextRow {
                        key: "JvmArgs"
                        label: qsTr("JVM arguments")
                        placeholder: "-XX:+UseG1GC"
                    }
                    TextRow {
                        key: "JsonEditor"
                        label: qsTr("External JSON editor")
                        subtitle: qsTr("Path to the program used to edit JSON files.")
                    }

                    BeeSectionHeader { text: qsTr("Java management") }
                    BoolRow {
                        key: "IgnoreJavaCompatibility"
                        label: qsTr("Ignore Java compatibility checks")
                    }
                    BoolRow {
                        key: "IgnoreJavaWizard"
                        label: qsTr("Skip the Java setup wizard")
                    }
                    BoolRow {
                        key: "AutomaticJavaSwitch"
                        label: qsTr("Switch Java automatically")
                    }
                    BoolRow {
                        key: "AutomaticJavaDownload"
                        label: qsTr("Download Java automatically")
                    }
                }

                // 3: Minecraft
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Window") }
                    BoolRow {
                        key: "LaunchMaximized"
                        label: qsTr("Start Minecraft maximized")
                    }
                    IntRow {
                        key: "MinecraftWinWidth"
                        label: qsTr("Window width")
                        from: 256; to: 7680; suffix: qsTr("px")
                    }
                    IntRow {
                        key: "MinecraftWinHeight"
                        label: qsTr("Window height")
                        from: 256; to: 4320; suffix: qsTr("px")
                    }

                    BeeSectionHeader { text: qsTr("Game time") }
                    BoolRow {
                        key: "ShowGameTime"
                        label: qsTr("Show time played per instance")
                    }
                    BoolRow {
                        key: "ShowGlobalGameTime"
                        label: qsTr("Show total time played")
                    }
                    BoolRow {
                        key: "RecordGameTime"
                        label: qsTr("Record time played")
                    }
                    BoolRow {
                        key: "ShowGameTimeWithoutDays"
                        label: qsTr("Show game time without days")
                    }

                    BeeSectionHeader { text: qsTr("Miscellaneous") }
                    BoolRow {
                        key: "CopyIngameScreenshots"
                        label: qsTr("Copy in-game screenshots to the instance")
                    }
                    TextRow {
                        key: "WrapperCommand"
                        label: qsTr("Wrapper command")
                        subtitle: qsTr("Command wrapped around the game process.")
                    }
                    TextRow {
                        key: "PreLaunchCommand"
                        label: qsTr("Pre-launch command")
                    }
                    TextRow {
                        key: "PostExitCommand"
                        label: qsTr("Post-exit command")
                    }
                }

                // 4: Native and Performance
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Native libraries") }
                    BoolRow {
                        key: "UseNativeOpenAL"
                        label: qsTr("Use system OpenAL")
                    }
                    TextRow {
                        key: "CustomOpenALPath"
                        label: qsTr("Custom OpenAL path")
                    }
                    BoolRow {
                        key: "UseNativeGLFW"
                        label: qsTr("Use system GLFW")
                    }
                    TextRow {
                        key: "CustomGLFWPath"
                        label: qsTr("Custom GLFW path")
                    }

                    BeeSectionHeader { text: qsTr("Performance") }
                    BoolRow {
                        key: "EnableFeralGamemode"
                        label: qsTr("Enable Feral GameMode")
                    }
                    BoolRow {
                        key: "EnableMangoHud"
                        label: qsTr("Enable MangoHud")
                    }
                    BoolRow {
                        key: "UseDiscreteGpu"
                        label: qsTr("Use the discrete GPU")
                    }
                    BoolRow {
                        key: "UseZink"
                        label: qsTr("Use Zink (OpenGL on Vulkan)")
                    }
                }

                // 5: Console
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Console window") }
                    BoolRow {
                        key: "ShowConsole"
                        label: qsTr("Show the console while the game is running")
                    }
                    BoolRow {
                        key: "AutoCloseConsole"
                        label: qsTr("Close the console when the game quits")
                    }
                    BoolRow {
                        key: "ShowConsoleOnError"
                        label: qsTr("Show the console when the game crashes")
                    }
                    BoolRow {
                        key: "LogPrePostOutput"
                        label: qsTr("Log pre-launch and post-exit command output")
                    }
                    BoolRow {
                        key: "ConsoleOverflowStop"
                        label: qsTr("Stop logging when the line limit is hit")
                    }

                    BeeSectionHeader { text: qsTr("Appearance") }
                    TextRow {
                        key: "ConsoleFont"
                        label: qsTr("Console font")
                        placeholder: qsTr("e.g. Monospace")
                    }
                    IntRow {
                        key: "ConsoleFontSize"
                        label: qsTr("Console font size")
                        from: 6; to: 32; suffix: qsTr("pt")
                    }
                    IntRow {
                        key: "ConsoleMaxLines"
                        label: qsTr("Maximum console lines")
                        from: 100; to: 5000000
                    }
                }

                // 6: Downloads and Folders
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Folders") }
                    PathRow {
                        key: "DownloadsDir"
                        label: qsTr("Downloads folder")
                    }
                    PathRow {
                        key: "CentralModsDir"
                        label: qsTr("Central mods folder")
                    }
                    PathRow {
                        key: "IconsDir"
                        label: qsTr("Icons folder")
                    }
                    PathRow {
                        key: "SkinsDir"
                        label: qsTr("Skins folder")
                    }
                    PathRow {
                        key: "JavaDir"
                        label: qsTr("Downloaded Java runtimes folder")
                    }

                    BeeSectionHeader { text: qsTr("Downloaded mods") }
                    BoolRow {
                        key: "DownloadsDirWatchRecursive"
                        label: qsTr("Watch the downloads folder recursively")
                    }
                    BoolRow {
                        key: "MoveModsFromDownloadsDir"
                        label: qsTr("Move mods from the downloads folder into instances")
                    }
                }

                // 7: Proxy
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Proxy") }
                    ComboRow {
                        key: "ProxyType"
                        label: qsTr("Proxy type")
                        options: [
                            { "name": qsTr("System default"), "value": "Default" },
                            { "name": qsTr("None"), "value": "None" },
                            { "name": "HTTP", "value": "HTTP" },
                            { "name": "SOCKS5", "value": "SOCKS5" }
                        ]
                    }
                    TextRow {
                        key: "ProxyAddr"
                        label: qsTr("Proxy address")
                        placeholder: "127.0.0.1"
                    }
                    IntRow {
                        key: "ProxyPort"
                        label: qsTr("Proxy port")
                        from: 0; to: 65535
                    }
                    TextRow {
                        key: "ProxyUser"
                        label: qsTr("Proxy username")
                    }
                    SecretRow {
                        key: "ProxyPass"
                        label: qsTr("Proxy password")
                    }
                }

                // 8: Integrations
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    BeeSectionHeader { text: qsTr("Log upload") }
                    ComboRow {
                        key: "PastebinType"
                        label: qsTr("Paste service")
                        options: [
                            { "name": "0x0.st", "value": 0 },
                            { "name": "hastebin", "value": 1 },
                            { "name": "paste.gg", "value": 2 },
                            { "name": "mclo.gs", "value": 3 }
                        ]
                    }
                    TextRow {
                        key: "PastebinURL"
                        label: qsTr("Paste URL")
                        subtitle: qsTr("Base URL of the paste service.")
                    }
                    TextRow {
                        key: "PastebinCustomAPIBase"
                        label: qsTr("Custom API base")
                    }

                    BeeSectionHeader { text: qsTr("Service credentials") }
                    TextRow {
                        key: "MSAClientIDOverride"
                        label: qsTr("Microsoft client ID override")
                    }
                    SecretRow {
                        key: "FlameKeyOverride"
                        label: qsTr("CurseForge API key override")
                    }
                    BoolRow {
                        key: "FlameKeyShouldBeFetchedOnStartup"
                        label: qsTr("Fetch the CurseForge key on startup")
                    }
                    SecretRow {
                        key: "ModrinthToken"
                        label: qsTr("Modrinth token")
                    }
                    TextRow {
                        key: "UserAgentOverride"
                        label: qsTr("User agent override")
                    }

                    BeeSectionHeader { text: qsTr("Metadata URLs") }
                    TextRow {
                        key: "MetaURLOverride"
                        label: qsTr("Meta URL override")
                    }
                    TextRow {
                        key: "ResourceURLOverride"
                        label: qsTr("Resource URL override")
                    }
                    TextRow {
                        key: "LegacyFMLLibsURLOverride"
                        label: qsTr("Legacy FML libs URL override")
                    }
                    BoolRow {
                        key: "MvaEnabled"
                        label: qsTr("Use an MVA mirror")
                    }
                    TextRow {
                        key: "MvaURL"
                        label: qsTr("MVA mirror URL")
                    }
                    TextRow {
                        key: "SavesURL"
                        label: qsTr("Saves server URL")
                    }

                    BeeSectionHeader { text: qsTr("Authlib injector") }
                    TextRow {
                        key: "InjectorUid"
                        label: qsTr("Injector UID")
                    }
                    TextRow {
                        key: "InjectorVersion"
                        label: qsTr("Injector version")
                        subtitle: qsTr("Leave empty for the latest version.")
                    }
                    ComboRow {
                        key: "UseElySkins"
                        label: qsTr("Ely.by skins")
                        options: [
                            { "name": qsTr("Never"), "value": 0 },
                            { "name": qsTr("Always"), "value": 1 },
                            { "name": qsTr("Only with an Ely.by account"), "value": 2 },
                            { "name": qsTr("With Ely.by or offline accounts"), "value": 3 }
                        ]
                    }

                    BeeSectionHeader { text: qsTr("Importers") }
                    TextRow {
                        key: "TechnicClientID"
                        label: qsTr("Technic client ID")
                    }
                    PathRow {
                        key: "FTBAppInstancesPath"
                        label: qsTr("FTB App instances path")
                    }
                }

                // 9: Accounts
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: acctCol.implicitHeight + Theme.sp24 * 2
                        radius: Theme.r16
                        color: Theme.surface
                        border.width: 1
                        border.color: Theme.border
                        ColumnLayout {
                            id: acctCol
                            anchors.fill: parent
                            anchors.margins: Theme.sp24
                            spacing: Theme.sp12
                            Text {
                                text: qsTr("Accounts")
                                color: Theme.text
                                font.pixelSize: Theme.title
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: qsTr("Manage Microsoft, Ely.by and offline accounts on the Accounts page.")
                                color: Theme.textDim
                                font.pixelSize: Theme.body
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            BeeButton {
                                variant: "primary"
                                text: qsTr("Manage accounts")
                                iconSource: "icons/person.svg"
                                onClicked: page.requestNav(page.accountsNavIndex)
                            }
                        }
                    }
                }

                // 10: About
                ColumnLayout {
                    spacing: Theme.sp16
                    Item { height: Theme.sp24; width: 1 }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: aboutCol.implicitHeight + Theme.sp24 * 2
                        radius: Theme.r16
                        color: Theme.surface
                        border.width: 1
                        border.color: Theme.border
                        ColumnLayout {
                            id: aboutCol
                            anchors.fill: parent
                            anchors.margins: Theme.sp24
                            spacing: Theme.sp12
                            RowLayout {
                                spacing: Theme.sp16
                                Image {
                                    source: "../bee_logo.png"
                                    sourceSize: Qt.size(56, 56)
                                    Layout.preferredWidth: 56
                                    Layout.preferredHeight: 56
                                }
                                ColumnLayout {
                                    spacing: 2
                                    Text {
                                        text: "Bee Launcher"
                                        color: Theme.text
                                        font.pixelSize: Theme.title
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        text: qsTr("Version %1").arg(page.backend() ? page.backend().appVersion() : "?")
                                        color: Theme.textDim
                                        font.pixelSize: Theme.body
                                    }
                                }
                            }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: Theme.sp24
                                rowSpacing: Theme.sp8
                                Repeater {
                                    model: [
                                        [qsTr("Qt version"), page.backend() ? page.backend().qtVersion() : "?"],
                                        [qsTr("Git ref"), page.backend() ? page.backend().gitRef() : "?"],
                                        [qsTr("Data root"), page.backend() ? page.backend().dataRoot() : "?"]
                                    ]
                                    delegate: ColumnLayout {
                                        required property var modelData
                                        spacing: 2
                                        Layout.fillWidth: true
                                        Text { text: modelData[0]; color: Theme.textFaint; font.pixelSize: Theme.caption }
                                        Text { text: modelData[1]; color: Theme.text; font.pixelSize: Theme.body; elide: Text.ElideMiddle; Layout.fillWidth: true }
                                    }
                                }
                            }
                            RowLayout {
                                spacing: Theme.sp12
                                Layout.topMargin: Theme.sp8
                                BeeButton {
                                    variant: "secondary"
                                    text: qsTr("Open data folder")
                                    iconSource: "icons/folder.svg"
                                    onClicked: { var b = page.backend(); if (b) b.openPath(b.dataRoot()) }
                                }
                                BeeButton {
                                    variant: "secondary"
                                    text: qsTr("Switch to classic interface")
                                    iconSource: "icons/settings.svg"
                                    onClicked: { var b = page.backend(); if (b) b.useClassicUi() }
                                }
                            }
                            Text {
                                text: qsTr("Switching interfaces takes effect after restart.")
                                color: Theme.textFaint
                                font.pixelSize: Theme.caption
                            }
                        }
                    }
                }
            }
        }
    }
}
