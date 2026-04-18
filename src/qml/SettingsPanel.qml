import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: settingsPanel
    color: palette.base
    radius: 12

    required property QtObject device

    Connections {
        target: device
        function onSleepModeChanged() { sleepState.current = device.sleepMode }
        function onNotificationSoundChanged() { notifState.current = device.notificationSound }
        function onLedBrightnessChanged() { ledSlider.value = device.ledBrightness }
    }

    // Track active state locally for instant visual feedback
    QtObject { id: notifState; property int current: device.notificationSound }
    QtObject { id: sleepState; property int current: device.sleepMode }

    component OptionBtn: Rectangle {
        property string label
        property bool active: false
        signal clicked()
        Layout.fillWidth: true
        height: 32; radius: 6
        color: active ? palette.highlight : (btnMa.containsMouse ? palette.button : palette.dark)
        border.color: active ? Qt.darker(palette.highlight, 1.3) : palette.mid; border.width: 1
        Label {
            anchors.centerIn: parent; text: parent.label
            color: parent.active ? palette.highlightedText : palette.disabled.text; font.pixelSize: 12
        }
        MouseArea {
            id: btnMa; anchors.fill: parent; hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    component SectionLabel: Label {
        color: palette.text; font.pixelSize: 13; font.bold: true
        Layout.topMargin: 4
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 14
        contentHeight: col.implicitHeight
        clip: true

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 10

            // Device Info
            SectionLabel { text: "Device Info" }
            GridLayout {
                columns: 2; columnSpacing: 16; rowSpacing: 6; Layout.fillWidth: true
                Label { text: "Name"; color: palette.disabled.text; font.pixelSize: 12 }
                Label { text: device.deviceName || "--"; color: palette.text; font.pixelSize: 12 }
                Label { text: "Serial"; color: palette.disabled.text; font.pixelSize: 12 }
                Label { text: device.serialNumber || "--"; color: palette.text; font.pixelSize: 12 }
                Label { text: "Firmware"; color: palette.disabled.text; font.pixelSize: 12 }
                Label { text: device.firmware || "--"; color: palette.text; font.pixelSize: 12 }
                Label { text: "Uptime"; color: palette.disabled.text; font.pixelSize: 12 }
                Label {
                    color: palette.text; font.pixelSize: 12
                    text: {
                        if (device.uptime < 0) return "--"
                        let s = device.uptime
                        let h = Math.floor(s / 3600)
                        let m = Math.floor((s % 3600) / 60)
                        return h > 0 ? h + "h " + m + "m" : m + "m"
                    }
                }
            }

            // Bluetooth
            SectionLabel { text: "Bluetooth" }
            GridLayout {
                columns: 2; columnSpacing: 16; rowSpacing: 6; Layout.fillWidth: true
                Label { text: "Status"; color: palette.disabled.text; font.pixelSize: 12 }
                Label { text: device.btConnected ? "Connected" : "Disconnected"
                    color: device.btConnected ? "#5a9a6a" : palette.disabled.text; font.pixelSize: 12 }
                Label { text: "Device"; color: palette.disabled.text; font.pixelSize: 12 }
                Label { text: device.btName || "--"; color: palette.text; font.pixelSize: 12 }
            }

            // Notifications
            SectionLabel { text: "Notifications" }
            RowLayout {
                spacing: 6; Layout.fillWidth: true
                Repeater {
                    model: [{ label: "None", value: 0 }, { label: "Minimal", value: 1 }, { label: "All", value: 2 }]
                    OptionBtn {
                        label: modelData.label
                        active: notifState.current === modelData.value
                        onClicked: { device.setNotificationSound(modelData.value); notifState.current = modelData.value }
                    }
                }
            }

            // Sleep
            SectionLabel { text: "Suspend After" }
            RowLayout {
                spacing: 6; Layout.fillWidth: true
                Repeater {
                    model: [{ label: "Never", value: 0 }, { label: "15 min", value: 15 }, { label: "30 min", value: 30 }, { label: "60 min", value: 60 }]
                    OptionBtn {
                        label: modelData.label
                        active: sleepState.current === modelData.value
                        onClicked: { device.setSleepMode(modelData.value); sleepState.current = modelData.value }
                    }
                }
            }

            // LED Brightness
            SectionLabel { text: "LED Brightness" }
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                Slider {
                    id: ledSlider; Layout.fillWidth: true
                    from: 0; to: 100; stepSize: 1
                    value: device.ledBrightness >= 0 ? device.ledBrightness : 50
                    onMoved: device.setLedBrightness(value)

                    background: Rectangle {
                        x: ledSlider.leftPadding
                        y: ledSlider.topPadding + ledSlider.availableHeight / 2 - height / 2
                        implicitWidth: 200; implicitHeight: 6
                        width: ledSlider.availableWidth; height: implicitHeight
                        radius: 3; color: palette.dark
                        Rectangle {
                            width: ledSlider.visualPosition * parent.width
                            height: parent.height; radius: 3; color: palette.highlight
                        }
                    }
                    handle: Rectangle {
                        x: ledSlider.leftPadding + ledSlider.visualPosition * (ledSlider.availableWidth - width)
                        y: ledSlider.topPadding + ledSlider.availableHeight / 2 - height / 2
                        implicitWidth: 20; implicitHeight: 20; radius: 10
                        color: ledSlider.pressed ? Qt.darker(palette.highlight, 1.3) : palette.highlight
                        border.color: palette.mid; border.width: 1
                    }
                }
                Label { text: ledSlider.value + "%"; color: palette.highlight; font.pixelSize: 14; font.bold: true; Layout.preferredWidth: 40 }
            }
        }
    }
}
