import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ratatoskr

Rectangle {
    id: volumePanel
    color: palette.base
    radius: 12

    required property QtObject device

    PulseAudioSource { id: micInput }

    Connections {
        target: device
        function onVolumeChanged() { volumeSlider.value = device.volume }
        function onMixampChanged() { mixampSlider.value = device.mixamp }
        function onSidetoneChanged() { sidetoneSlider.value = device.sidetone }
    }

    // Horizontal styled slider
    component AccentSlider: Slider {
        id: ctrl
        Layout.fillWidth: true

        background: Rectangle {
            x: ctrl.leftPadding
            y: ctrl.topPadding + ctrl.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 6
            width: ctrl.availableWidth
            height: implicitHeight
            radius: 3
            color: palette.dark

            Rectangle {
                width: ctrl.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: palette.highlight
            }
        }

        handle: Rectangle {
            x: ctrl.leftPadding + ctrl.visualPosition * (ctrl.availableWidth - width)
            y: ctrl.topPadding + ctrl.availableHeight / 2 - height / 2
            implicitWidth: 20
            implicitHeight: 20
            radius: 10
            color: ctrl.pressed ? Qt.darker(palette.highlight, 1.3) : palette.highlight
            border.color: palette.mid
            border.width: 1
        }
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 16
        contentHeight: col.implicitHeight
        clip: true

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                RowLayout {
                    Label { text: "Volume"; color: palette.text; font.pixelSize: 14; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label { text: Math.round(volumeSlider.value / 31 * 100) + "%"; color: palette.highlight; font.pixelSize: 14; font.bold: true }
                }
                AccentSlider {
                    id: volumeSlider
                    from: 0; to: 31; stepSize: 1
                    value: device.volume >= 0 ? device.volume : 0
                    onMoved: device.setVolume(Math.round(value * 21 / 31))
                }
            }

            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                RowLayout {
                    Label { text: "MixAmp"; color: palette.text; font.pixelSize: 14; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: { let v = mixampSlider.value; return "Voice " + Math.round((12-v)/12*100) + "% / Game " + Math.round(v/12*100) + "%" }
                        color: palette.highlight; font.pixelSize: 14; font.bold: true
                    }
                }
                AccentSlider {
                    id: mixampSlider
                    from: 0; to: 12; stepSize: 1
                    value: device.mixamp >= 0 ? device.mixamp : 6
                    onMoved: device.setMixamp(value)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                RowLayout {
                    Label { text: "Sidetone"; color: palette.text; font.pixelSize: 14; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label { text: Math.round(sidetoneSlider.value / 6 * 100) + "%"; color: palette.highlight; font.pixelSize: 14; font.bold: true }
                }
                AccentSlider {
                    id: sidetoneSlider
                    from: 0; to: 6; stepSize: 1
                    value: device.sidetone >= 0 ? device.sidetone : 0
                    onMoved: device.setSidetone(value)
                }
            }

            // Mic Input (PipeWire/PulseAudio source volume)
            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                visible: micInput.available
                RowLayout {
                    Label { text: "Mic Input"; color: palette.text; font.pixelSize: 14; font.bold: true }
                    Label { text: "(system)"; color: palette.disabled.text; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Label { text: micInputSlider.value + "%"; color: palette.highlight; font.pixelSize: 14; font.bold: true }
                }
                RowLayout {
                    spacing: 8
                    // Mute toggle -- matches StatusBar mic indicator pattern
                    Rectangle {
                        width: 28; height: 28; radius: 14
                        color: micInput.muted ? "#4a2030" : "#1a3a2a"
                        border.width: 1; border.color: micInput.muted ? "#7a3040" : "#2a5a3a"
                        Image {
                            anchors.centerIn: parent
                            source: micInput.muted ? "qrc:/icons/icons/headphones.svg" : "qrc:/icons/icons/headset.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: micInput.setMuted(!micInput.muted)
                        }
                    }
                    AccentSlider {
                        id: micInputSlider
                        from: 0; to: 100; stepSize: 1
                        value: micInput.volume
                        onMoved: micInput.setVolume(value)
                        opacity: micInput.muted ? 0.4 : 1.0
                    }
                }
            }
        }
    }
}
