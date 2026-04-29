import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: statusBar
    Layout.fillWidth: true
    height: 72
    radius: 12
    color: palette.base

    required property QtObject device

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        // Squirrel icon + name
        ColumnLayout {
            spacing: 2
            Image {
                source: "qrc:/icons/icons/squirrel.svg"
                sourceSize.width: 28
                sourceSize.height: 28
            }
            Row {
                spacing: 0
                Label {
                    text: "A50 Gen 5: "
                    color: palette.text
                    font.pixelSize: 13
                    font.bold: true
                }
                Label {
                    text: device.connected ? "Connected" : "Disconnected"
                    color: device.connected ? "#5a9a6a" : "#9a5a5a"
                    font.pixelSize: 13
                    font.bold: true
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Mic status: headphones = muted, headset = unmuted
        Rectangle {
            width: 36; height: 36; radius: 18
            color: device.micMuted ? "#4a2030" : "#1a3a2a"
            border.width: 2
            border.color: device.micMuted ? "#7a3040" : "#2a5a3a"
            Image {
                anchors.centerIn: parent
                source: device.micMuted ? "qrc:/icons/icons/headphones.svg" : "qrc:/icons/icons/headset.svg"
                sourceSize.width: 18
                sourceSize.height: 18
            }
        }

        // Battery. Naive estimate from Logitech `max_life_span=24h` spec
        // (G HUB battery.xml). DeviceProxy logs every sample to
        // ~/.config/ratatoskr/gui-drain-history.jsonl — same JSONL schema as
        // the plugin (intentionally separate file). Future fase 2-3 will add
        // session-detector + drain-rate calibration; until then the "(kalibrerer)"
        // marker tells the user the value is the spec, not measured data.
        ColumnLayout {
            spacing: 1
            readonly property int batteryLifeSpanHours: 24
            function timeRemaining() {
                if (device.battery <= 0) return ""
                let totalMin = Math.round((device.battery / 100) * batteryLifeSpanHours * 60)
                let h = Math.floor(totalMin / 60)
                let m = totalMin % 60
                return h > 0 ? "~" + h + "h " + m + "min" : "~" + m + "min"
            }
            Label {
                text: device.battery >= 0 ? device.battery + "%" : "--"
                color: device.battery >= 0 && device.battery <= 15 ? "#e74c3c" : "#e0e0e0"
                font.pixelSize: 22
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
            Label {
                text: device.charging ? "Charging"
                    : (device.battery > 0 ? parent.timeRemaining() + " (kalibrerer)" : "Battery")
                color: device.charging ? "#27ae60" : palette.disabled.text
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
