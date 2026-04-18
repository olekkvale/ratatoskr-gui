import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: streamPanel
    color: palette.base
    radius: 12

    required property QtObject device

    // Local state for optimistic updates
    property int sStreamVol: device.routeStreamVol
    property bool sStreamMute: device.routeStreamMute
    property int sMicVol: device.routeMicVol
    property bool sMicMute: device.routeMicMute
    property int sGameVol: device.routeGameVol
    property bool sGameMute: device.routeGameMute
    property int sBtVol: device.routeBtVol
    property bool sBtMute: device.routeBtMute
    property int sVoiceVol: device.routeVoiceVol
    property bool sVoiceMute: device.routeVoiceMute

    Connections {
        target: device
        function onRoutingChanged() {
            sStreamVol = device.routeStreamVol; sStreamMute = device.routeStreamMute
            sMicVol = device.routeMicVol;       sMicMute = device.routeMicMute
            sGameVol = device.routeGameVol;     sGameMute = device.routeGameMute
            sBtVol = device.routeBtVol;         sBtMute = device.routeBtMute
            sVoiceVol = device.routeVoiceVol;   sVoiceMute = device.routeVoiceMute
        }
    }

    function sendRouting() {
        device.setRouting(sStreamVol, sStreamMute, sMicVol, sMicMute,
                          sGameVol, sGameMute, sBtVol, sBtMute,
                          sVoiceVol, sVoiceMute)
    }

    // Styled slider
    component AccentSlider: Slider {
        id: ctrl
        Layout.fillWidth: true
        background: Rectangle {
            x: ctrl.leftPadding
            y: ctrl.topPadding + ctrl.availableHeight / 2 - height / 2
            implicitWidth: 200; implicitHeight: 6
            width: ctrl.availableWidth; height: implicitHeight
            radius: 3; color: palette.dark
            Rectangle {
                width: ctrl.visualPosition * parent.width
                height: parent.height; radius: 3; color: palette.highlight
            }
        }
        handle: Rectangle {
            x: ctrl.leftPadding + ctrl.visualPosition * (ctrl.availableWidth - width)
            y: ctrl.topPadding + ctrl.availableHeight / 2 - height / 2
            implicitWidth: 20; implicitHeight: 20; radius: 10
            color: ctrl.pressed ? Qt.darker(palette.highlight, 1.3) : palette.highlight
            border.color: palette.mid; border.width: 1
        }
    }

    // Mute toggle
    component MuteBtn: Rectangle {
        property bool muted: false
        signal toggled()
        width: 28; height: 28; radius: 14
        color: muted ? "#4a2030" : "#1a3a2a"
        border.width: 1; border.color: muted ? "#7a3040" : "#2a5a3a"
        Label {
            anchors.centerIn: parent
            text: parent.muted ? "\u{1F507}" : "\u{1F50A}"
            font.pixelSize: 11
        }
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: parent.toggled()
        }
    }

    // Channel row component
    component ChannelRow: ColumnLayout {
        property string label
        property string desc
        property int vol: 16
        property bool muted: false
        signal volumeChanged(int v)
        signal muteToggled()

        Layout.fillWidth: true; spacing: 4
        RowLayout {
            Label { text: parent.parent.label; color: palette.text; font.pixelSize: 14; font.bold: true }
            Label { text: parent.parent.desc; color: palette.disabled.text; font.pixelSize: 11; visible: parent.parent.desc !== "" }
            Item { Layout.fillWidth: true }
            Label { text: Math.round(chSlider.value / 32 * 100) + "%"; color: palette.highlight; font.pixelSize: 14; font.bold: true }
        }
        RowLayout {
            spacing: 8
            MuteBtn {
                muted: parent.parent.muted
                onToggled: parent.parent.muteToggled()
            }
            AccentSlider {
                id: chSlider
                from: 0; to: 32; stepSize: 1
                value: parent.parent.vol >= 0 ? parent.parent.vol : 16
                onMoved: parent.parent.volumeChanged(value)
                opacity: parent.parent.muted ? 0.4 : 1.0
            }
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
            spacing: 18

            Label { text: "Stream Routing"; color: palette.text; font.pixelSize: 15; font.bold: true }
            Label { text: "Controls what your streaming audience hears"; color: palette.disabled.text; font.pixelSize: 11; Layout.topMargin: -14 }

            ChannelRow {
                label: "Stream"; desc: "(master)"
                vol: sStreamVol; muted: sStreamMute
                onVolumeChanged: (v) => { sStreamVol = v; sendRouting() }
                onMuteToggled: { sStreamMute = !sStreamMute; sendRouting() }
            }

            ChannelRow {
                label: "Mic Out"; desc: "(mic in stream)"
                vol: sMicVol; muted: sMicMute
                onVolumeChanged: (v) => { sMicVol = v; sendRouting() }
                onMuteToggled: { sMicMute = !sMicMute; sendRouting() }
            }

            ChannelRow {
                label: "Game"; desc: "(game audio)"
                vol: sGameVol; muted: sGameMute
                onVolumeChanged: (v) => { sGameVol = v; sendRouting() }
                onMuteToggled: { sGameMute = !sGameMute; sendRouting() }
            }

            ChannelRow {
                label: "Bluetooth"; desc: "(BT audio)"
                vol: sBtVol; muted: sBtMute
                onVolumeChanged: (v) => { sBtVol = v; sendRouting() }
                onMuteToggled: { sBtMute = !sBtMute; sendRouting() }
            }

            ChannelRow {
                label: "Voice"; desc: "(voice chat)"
                vol: sVoiceVol; muted: sVoiceMute
                onVolumeChanged: (v) => { sVoiceVol = v; sendRouting() }
                onMuteToggled: { sVoiceMute = !sVoiceMute; sendRouting() }
            }
        }
    }
}
