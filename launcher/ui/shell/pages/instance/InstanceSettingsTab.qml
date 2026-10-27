import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Bee.Shell

// Per-instance settings overrides, mirroring the classic InstanceSettingsPage.
// Each card is gated by an Override* passthrough setting; rows write through
// instanceBridge.instValue/setInstValue and reload on settingsChanged.
Item {
    id: root
    property var bridge: null

    // A card with a master gate checkbox; children land in the dimmed body.
    component GateCard: Rectangle {
        id: card
        property string title: ""
        property string subtitle: ""
        property string gate: ""
        property alias gateChecked: gateBox.checked
        default property alias rows: body.data
        Layout.fillWidth: true
        radius: Theme.r16
        color: Theme.surface
        border.color: Theme.border
        implicitHeight: outer.implicitHeight + Theme.sp32
        ColumnLayout {
            id: outer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.sp16
            spacing: Theme.sp8
            BeeCheckBox {
                id: gateBox
                text: card.title
                subtitle: card.subtitle
                function reload() { checked = root.bridge ? root.bridge.instGate(card.gate) : false }
                Component.onCompleted: reload()
                Connections {
                    target: root.bridge
                    function onInstanceChanged() { gateBox.reload() }
                    function onSettingsChanged() { gateBox.reload() }
                }
                onToggled: if (root.bridge) root.bridge.setInstGate(card.gate, checked)
            }
            ColumnLayout {
                id: body
                Layout.fillWidth: true
                spacing: Theme.sp4
                enabled: gateBox.checked
                opacity: enabled ? 1.0 : 0.45
            }
        }
    }

    // Setting value controls. All reload imperatively so editing never fights
    // a live binding.
    component TextCtl: BeeTextField {
        property string key: ""
        Layout.preferredWidth: 300
        function reload() {
            var v = root.bridge ? root.bridge.instValue(key) : null
            text = (v === undefined || v === null) ? "" : String(v)
        }
        Component.onCompleted: reload()
        Connections {
            target: root.bridge
            function onInstanceChanged() { reload() }
            function onSettingsChanged() { if (!activeFocus) reload() }
        }
        onEditingFinished: if (root.bridge) root.bridge.setInstValue(key, text)
    }
    component BoolCtl: BeeCheckBox {
        property string key: ""
        text: ""
        function reload() {
            var v = root.bridge ? root.bridge.instValue(key) : false
            checked = (v === true || v === "true" || v === 1)
        }
        Component.onCompleted: reload()
        Connections {
            target: root.bridge
            function onInstanceChanged() { reload() }
            function onSettingsChanged() { reload() }
        }
        onToggled: if (root.bridge) root.bridge.setInstValue(key, checked)
    }
    component IntCtl: BeeSpinBox {
        property string key: ""
        from: 0
        to: 262144
        Layout.preferredWidth: 130
        function reload() {
            var v = root.bridge ? root.bridge.instValue(key) : 0
            var n = parseInt(v)
            value = isNaN(n) ? 0 : n
        }
        Component.onCompleted: reload()
        Connections {
            target: root.bridge
            function onInstanceChanged() { reload() }
            function onSettingsChanged() { reload() }
        }
        onValueModified: if (root.bridge) root.bridge.setInstValue(key, value)
        // fallback so typed values persist even where valueModified is absent
        onActiveFocusChanged: if (!activeFocus && root.bridge) root.bridge.setInstValue(key, value)
    }
    component ResetBtn: BeeIconButton {
        property string key: ""
        iconSource: "icons/dismiss.svg"
        Accessible.name: qsTr("Reset to global")
        onClicked: if (root.bridge) root.bridge.resetInstValue(key)
    }

    component FormRow: BeeFieldRow {
        Layout.fillWidth: true
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: col.implicitHeight + Theme.sp32 * 2
        ScrollBar.vertical: BeeScrollBar {}

        ColumnLayout {
            id: col
            x: Theme.sp32
            width: parent.width - Theme.sp32 * 2
            spacing: Theme.sp16
            Item { width: 1; height: Theme.sp8 }

            Text {
                text: qsTr("Enable a group to override the global launcher settings for this instance.")
                color: Theme.textDim
                font.pixelSize: Theme.body
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            GateCard {
                title: qsTr("Java installation")
                subtitle: qsTr("JavaPath, compatibility check and automatic Java download")
                gate: "OverrideJavaLocation"
                FormRow { label: qsTr("Java path"); TextCtl { key: "JavaPath" } ResetBtn { key: "JavaPath" } }
                FormRow { label: qsTr("Skip Java compatibility check"); BoolCtl { key: "IgnoreJavaCompatibility" } ResetBtn { key: "IgnoreJavaCompatibility" } }
                FormRow { label: qsTr("Auto-download Mojang Java"); BoolCtl { key: "AutomaticJavaDownload" } ResetBtn { key: "AutomaticJavaDownload" } }
                FormRow { label: qsTr("Auto-switch to Mojang Java"); BoolCtl { key: "AutomaticJavaSwitch" } ResetBtn { key: "AutomaticJavaSwitch" } }
            }

            GateCard {
                title: qsTr("Memory")
                gate: "OverrideMemory"
                FormRow { label: qsTr("Minimum memory"); IntCtl { key: "MinMemAlloc"; suffix: qsTr("MiB"); to: 262144 } ResetBtn { key: "MinMemAlloc" } }
                FormRow { label: qsTr("Maximum memory"); IntCtl { key: "MaxMemAlloc"; suffix: qsTr("MiB"); to: 262144 } ResetBtn { key: "MaxMemAlloc" } }
                FormRow { label: qsTr("PermGen (old Java)"); IntCtl { key: "PermGen"; suffix: qsTr("MiB"); to: 8192 } ResetBtn { key: "PermGen" } }
            }

            GateCard {
                title: qsTr("Java arguments")
                gate: "OverrideJavaArgs"
                FormRow { label: qsTr("JVM arguments"); TextCtl { key: "JvmArgs"; Layout.preferredWidth: 420 } ResetBtn { key: "JvmArgs" } }
            }

            GateCard {
                title: qsTr("Game window")
                gate: "OverrideWindow"
                FormRow { label: qsTr("Launch maximized"); BoolCtl { key: "LaunchMaximized" } ResetBtn { key: "LaunchMaximized" } }
                FormRow { label: qsTr("Window width"); IntCtl { key: "MinecraftWinWidth"; suffix: qsTr("px"); to: 16384 } ResetBtn { key: "MinecraftWinWidth" } }
                FormRow { label: qsTr("Window height"); IntCtl { key: "MinecraftWinHeight"; suffix: qsTr("px"); to: 16384 } ResetBtn { key: "MinecraftWinHeight" } }
            }

            GateCard {
                title: qsTr("Console")
                gate: "OverrideConsole"
                FormRow { label: qsTr("Show console while the game runs"); BoolCtl { key: "ShowConsole" } ResetBtn { key: "ShowConsole" } }
                FormRow { label: qsTr("Close console when the game quits"); BoolCtl { key: "AutoCloseConsole" } ResetBtn { key: "AutoCloseConsole" } }
                FormRow { label: qsTr("Show console when the game crashes"); BoolCtl { key: "ShowConsoleOnError" } ResetBtn { key: "ShowConsoleOnError" } }
            }

            GateCard {
                title: qsTr("Custom commands")
                gate: "OverrideCommands"
                FormRow { label: qsTr("Wrapper command"); TextCtl { key: "WrapperCommand"; Layout.preferredWidth: 420 } ResetBtn { key: "WrapperCommand" } }
                FormRow { label: qsTr("Pre-launch command"); TextCtl { key: "PreLaunchCommand"; Layout.preferredWidth: 420 } ResetBtn { key: "PreLaunchCommand" } }
                FormRow { label: qsTr("Post-exit command"); TextCtl { key: "PostExitCommand"; Layout.preferredWidth: 420 } ResetBtn { key: "PostExitCommand" } }
            }

            GateCard {
                title: qsTr("Native workarounds")
                gate: "OverrideNativeWorkarounds"
                FormRow { label: qsTr("Use system OpenAL"); BoolCtl { key: "UseNativeOpenAL" } ResetBtn { key: "UseNativeOpenAL" } }
                FormRow { label: qsTr("Custom OpenAL path"); TextCtl { key: "CustomOpenALPath"; Layout.preferredWidth: 420 } ResetBtn { key: "CustomOpenALPath" } }
                FormRow { label: qsTr("Use system GLFW"); BoolCtl { key: "UseNativeGLFW" } ResetBtn { key: "UseNativeGLFW" } }
                FormRow { label: qsTr("Custom GLFW path"); TextCtl { key: "CustomGLFWPath"; Layout.preferredWidth: 420 } ResetBtn { key: "CustomGLFWPath" } }
            }

            GateCard {
                title: qsTr("Performance")
                gate: "OverridePerformance"
                FormRow { label: qsTr("Enable Feral Gamemode"); BoolCtl { key: "EnableFeralGamemode" } ResetBtn { key: "EnableFeralGamemode" } }
                FormRow { label: qsTr("Enable MangoHud"); BoolCtl { key: "EnableMangoHud" } ResetBtn { key: "EnableMangoHud" } }
                FormRow { label: qsTr("Use discrete GPU"); BoolCtl { key: "UseDiscreteGpu" } ResetBtn { key: "UseDiscreteGpu" } }
                FormRow { label: qsTr("Use Zink"); BoolCtl { key: "UseZink" } ResetBtn { key: "UseZink" } }
            }

            GateCard {
                title: qsTr("Game time")
                gate: "OverrideGameTime"
                FormRow { label: qsTr("Show time played"); BoolCtl { key: "ShowGameTime" } ResetBtn { key: "ShowGameTime" } }
                FormRow { label: qsTr("Record time played"); BoolCtl { key: "RecordGameTime" } ResetBtn { key: "RecordGameTime" } }
                FormRow { label: qsTr("Show time without days"); BoolCtl { key: "ShowGameTimeWithoutDays" } ResetBtn { key: "ShowGameTimeWithoutDays" } }
                FormRow { label: qsTr("Show total time across instances"); BoolCtl { key: "ShowGlobalGameTime" } ResetBtn { key: "ShowGlobalGameTime" } }
            }

            Item { width: 1; height: Theme.sp16 }
        }
    }
}
