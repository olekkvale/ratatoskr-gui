import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ratatoskr

ApplicationWindow {
    id: root
    width: 480
    height: 680
    minimumWidth: 420
    minimumHeight: 560
    title: "Ratatoskr"
    visible: true
    color: "#12121f"

    DeviceProxy { id: device }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        StatusBar { device: device }

        // Custom styled tab bar
        Rectangle {
            Layout.fillWidth: true
            height: 40
            radius: 10
            color: "#1a1a2e"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 3
                spacing: 3

                Repeater {
                    model: ["Volume", "Stream", "EQ", "Settings"]
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: tabIndex === index ? "#4a6fa5" : "transparent"

                        property int tabIndex: 0
                        Component.onCompleted: tabIndex = Qt.binding(() => stack.currentIndex)

                        Label {
                            anchors.centerIn: parent
                            text: modelData
                            color: tabIndex === index ? "#ffffff" : "#667799"
                            font.pixelSize: 13
                            font.bold: tabIndex === index
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: stack.currentIndex = index
                        }
                    }
                }
            }
        }

        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true

            VolumePanel { device: device }
            StreamPanel { device: device }
            EqPanel { device: device }
            SettingsPanel { device: device }
        }
    }
}
