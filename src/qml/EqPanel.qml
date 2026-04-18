import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: eqPanel
    color: palette.base
    radius: 12

    required property QtObject device

    property int eqType: 1
    property bool advancedMode: false
    property var defaultHeadphoneFreqs: [20, 50, 125, 250, 500, 1000, 2500, 5000, 10000, 20000]
    property var defaultMicFreqs: [80, 150, 250, 400, 800, 1500, 2500, 5000, 10000, 19000]

    property var currentBands: defaultBands()
    property var customPresets: []

    function defaultBands() {
        let freqs = eqType === 1 ? defaultHeadphoneFreqs : defaultMicFreqs
        return freqs.map(f => ({gain: 0, q: 1.0, freq: f}))
    }
    function defaultFreqForBand(index) {
        return (eqType === 1 ? defaultHeadphoneFreqs : defaultMicFreqs)[index]
    }

    property var builtinPresets: ({
        headphone: {
            "Standard": [0,0,0,0,0,0,0,0,0,0],
            "Gaming":   [-10,-9,-7,-4,0,4,6,6,6,6],
            "Media":    [8,6,4,-1,-1,2,4,3,1,-1]
        },
        mic: {
            "Standard":    [0,0,0,0,0,0,0,0,0,0],
            "Broadcast":   [8,8,5,3,0,0,0,1,2,2],
            "Competition": [-6,-6,-3,0,0,2,3,3,3,4]
        }
    })

    // Community presets — seeded to JSON on first run, then fully editable
    property var communitySeeds: [
        { name: "Music+Media", type: 1, bands: [
            {gain:8,q:1.367,freq:30},{gain:6,q:1.474,freq:60},
            {gain:3,q:1.367,freq:120},{gain:3,q:1.580,freq:250},
            {gain:3,q:1.474,freq:1000},{gain:4,q:1.367,freq:2500},
            {gain:6,q:1.223,freq:4000},{gain:5,q:1.186,freq:8000},
            {gain:6,q:1.000,freq:12000},{gain:4,q:0.885,freq:16000}
        ]},
        { name: "Tarkov Footsteps", type: 1, bands: [
            {gain:-12,q:1.000,freq:20},{gain:-2,q:1.000,freq:50},
            {gain:8,q:3.463,freq:125},{gain:-8,q:2.966,freq:250},
            {gain:-2,q:4.424,freq:800},{gain:-6,q:3.463,freq:1215},
            {gain:8,q:0.349,freq:1800},{gain:-6,q:1.822,freq:2650},
            {gain:8,q:0.481,freq:4580},{gain:-6,q:2.454,freq:8000}
        ]},
        { name: "Gaming+Media", type: 1, bands: [
            {gain:2,q:1.075,freq:20},{gain:2,q:1.856,freq:45},
            {gain:0,q:1.367,freq:150},{gain:0,q:1.000,freq:221},
            {gain:1,q:1.924,freq:526},{gain:1,q:1.000,freq:1198},
            {gain:1,q:2.454,freq:2283},{gain:5,q:1.000,freq:5000},
            {gain:2,q:1.924,freq:10000},{gain:4,q:1.000,freq:17154}
        ]},
        { name: "Stream", type: 0, bands: [
            {gain:0,q:1.0,freq:60},{gain:4,q:1.0,freq:125},
            {gain:5,q:1.0,freq:64},{gain:2,q:1.0,freq:250},
            {gain:-6,q:1.0,freq:500},{gain:-4,q:1.0,freq:1000},
            {gain:4,q:1.0,freq:2000},{gain:6,q:1.0,freq:4000},
            {gain:4,q:1.0,freq:8000},{gain:4,q:1.0,freq:16000}
        ]},
        { name: "Clear", type: 0, bands: [
            {gain:10,q:0.031,freq:80},{gain:8,q:0.031,freq:150},
            {gain:6,q:0.031,freq:250},{gain:6,q:0.031,freq:400},
            {gain:6,q:0.031,freq:800},{gain:10,q:0.031,freq:1500},
            {gain:12,q:0.031,freq:2500},{gain:10,q:0.031,freq:5000},
            {gain:7,q:0.031,freq:10000},{gain:6,q:0.031,freq:19000}
        ]}
    ]

    component ToggleBtn: Rectangle {
        property string label; property bool active; signal clicked()
        implicitWidth: lbl.implicitWidth + 24; height: 32; radius: 8
        color: active ? palette.highlight : palette.dark
        border.color: active ? Qt.darker(palette.highlight, 1.3) : palette.mid; border.width: 1
        Label { id: lbl; anchors.centerIn: parent; text: parent.label
            color: active ? palette.highlightedText : palette.disabled.text; font.pixelSize: 12; font.bold: active }
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }

    component PresetBtn: Rectangle {
        property string label; property color textColor: palette.disabled.text; property color borderColor: palette.mid
        property bool active: activePreset === label
        signal clicked()
        Layout.fillWidth: true; height: 30; radius: 6
        color: active ? palette.highlight : (ma.containsMouse ? palette.button : palette.dark)
        border.color: active ? Qt.darker(palette.highlight, 1.3) : borderColor; border.width: 1
        Label { anchors.centerIn: parent; text: parent.label; color: parent.active ? palette.highlightedText : parent.textColor; font.pixelSize: 11 }
        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }

    Component.onCompleted: { seedCommunityPresets(); reloadCustomPresets() }
    Timer { id: debounce; interval: 300; onTriggered: device.setCustomEqualizer(eqType, currentBands) }

    function seedCommunityPresets() {
        let existing = device.loadEqPresets()
        let existingNames = existing.map(p => p.name)
        let deleted = device.deletedEqPresets()
        for (let seed of communitySeeds) {
            if (!existingNames.includes(seed.name) && !deleted.includes(seed.name))
                device.saveEqPreset(seed.name, seed.type, seed.bands)
        }
    }

    function formatFreq(hz) {
        if (hz >= 1000) { let k = hz / 1000; return (hz % 1000 === 0 ? k.toFixed(0) : k.toFixed(1)) + "k" }
        return hz.toString()
    }
    function reloadCustomPresets() { customPresets = device.loadEqPresets().filter(p => p.type === eqType) }
    function applyBuiltinPreset(gains) {
        let freqs = eqType === 1 ? defaultHeadphoneFreqs : defaultMicFreqs
        applyBands(gains.map((g, i) => ({gain: g, q: 1.0, freq: freqs[i]})))
    }
    function applyBands(bands) {
        currentBands = bands.map((b, i) => ({
            gain: b.gain, q: b.q !== undefined ? b.q : 1.0,
            freq: b.freq !== undefined ? b.freq : defaultFreqForBand(i)
        }))
        savedBands = JSON.parse(JSON.stringify(currentBands))
        for (let i = 0; i < 10; i++) {
            let item = bandRepeater.itemAt(i)
            if (item) { item.gainValue = currentBands[i].gain; item.qValue = currentBands[i].q }
        }
        debounce.restart()
    }
    function updateBand(index, gain, q) {
        let b = currentBands.slice(); b[index] = {gain: gain, q: q, freq: b[index].freq}
        currentBands = b; debounce.restart()
    }
    function resetToSaved() { if (savedBands) applyBands(savedBands) }
    function savePreset() {
        let name = saveNameField.text.trim(); if (name === "") return
        device.saveEqPreset(name, eqType, currentBands)
        activePreset = name; savedBands = JSON.parse(JSON.stringify(currentBands))
        saveNameField.text = ""; savingMode = false; reloadCustomPresets()
    }
    function overwritePreset() {
        if (activePreset === "") return
        device.saveEqPreset(activePreset, eqType, currentBands)
        savedBands = JSON.parse(JSON.stringify(currentBands)); reloadCustomPresets()
    }
    function renamePreset() {
        let newName = renameField.text.trim(); if (newName === "") return
        device.renameEqPreset(activePreset, newName)
        activePreset = newName; renameField.text = ""; renamingMode = false; reloadCustomPresets()
    }

    property bool savingMode: false
    property bool renamingMode: false
    property string activePreset: ""
    property var savedBands: null

    property var hpState: null
    property var micState: null

    property int noiseGateState: device.noiseGate
    property bool isCustomPreset: customPresets.some(p => p.name === activePreset)
    property bool isModified: {
        if (!savedBands) return false
        return JSON.stringify(currentBands) !== JSON.stringify(savedBands)
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 14
        contentHeight: mainCol.implicitHeight
        clip: true

        ColumnLayout {
            id: mainCol
            width: parent.width
            spacing: 10

            // Type toggle + Advanced
            RowLayout {
                spacing: 8
                ToggleBtn { label: "Headphone"; active: eqType === 1
                    onClicked: {
                        micState = { bands: currentBands.slice(), preset: activePreset }
                        eqType = 1; reloadCustomPresets()
                        if (hpState) { activePreset = hpState.preset; applyBands(hpState.bands) }
                        else applyBuiltinPreset(builtinPresets.headphone["Standard"])
                    } }
                ToggleBtn { label: "Mic"; active: eqType === 0
                    onClicked: {
                        hpState = { bands: currentBands.slice(), preset: activePreset }
                        eqType = 0; reloadCustomPresets()
                        if (micState) { activePreset = micState.preset; applyBands(micState.bands) }
                        else applyBuiltinPreset(builtinPresets.mic["Standard"])
                    } }
                Item { Layout.fillWidth: true }
                ToggleBtn { label: advancedMode ? "Simple" : "Advanced"; active: advancedMode
                    onClicked: advancedMode = !advancedMode }
            }

            // Built-in presets
            GridLayout {
                Layout.fillWidth: true; columns: 3; columnSpacing: 6; rowSpacing: 6
                Repeater {
                    model: eqType === 1 ? Object.keys(builtinPresets.headphone) : Object.keys(builtinPresets.mic)
                    PresetBtn { label: modelData
                        onClicked: { let p = eqType === 1 ? builtinPresets.headphone : builtinPresets.mic; activePreset = modelData; applyBuiltinPreset(p[modelData]) } }
                }
            }

            // Custom presets (includes seeded community presets)
            GridLayout {
                Layout.fillWidth: true; columns: 3; columnSpacing: 6; rowSpacing: 6
                visible: customPresets.length > 0
                Repeater {
                    model: customPresets
                    PresetBtn { label: modelData.name; textColor: "#5a9a6a"; borderColor: "#2a5a3a"
                        onClicked: { activePreset = modelData.name; applyBands(modelData.bands) } }
                }
            }

            // EQ bands — absolute positioning to keep sliders aligned with dB scale
            Item {
                id: bandArea
                Layout.fillWidth: true
                Layout.preferredHeight: advancedMode ? 310 : 250

                // Layout constants
                readonly property int sliderTop: 16      // gain label height
                readonly property int sliderH: 200       // matches 5 × 40px dB labels
                readonly property int dbLabelH: 40       // per dB label
                readonly property int freqY: sliderTop + sliderH + 2
                readonly property int qLabelY: freqY + 32
                readonly property int qSliderY: qLabelY + 16
                readonly property real bandW: (width - 40) / 10

                // dB scale — 5 labels at slider tick positions
                Repeater {
                    id: dbLabels
                    model: ["+12", "+6", "0", "-6", "-12"]
                    Label {
                        x: 0; width: 28
                        y: bandArea.sliderTop + index * (bandArea.sliderH / 4) - 6
                        text: modelData; color: palette.disabled.text; font.pixelSize: 9
                        horizontalAlignment: Text.AlignRight
                    }
                }

                // Zero line (at midpoint of slider = 0 dB)
                Rectangle {
                    x: 34; width: parent.width - 34
                    y: bandArea.sliderTop + bandArea.sliderH / 2
                    height: 1; color: palette.mid; opacity: 0.5
                }

                // Band columns (absolute positioning per element)
                Repeater {
                    id: bandRepeater
                    model: 10

                    Item {
                        x: 34 + index * bandArea.bandW
                        width: bandArea.bandW
                        height: parent.height

                        property real gainValue: currentBands[index].gain
                        property real qValue: currentBands[index].q
                        onGainValueChanged: gainSlider.value = gainValue
                        onQValueChanged: qSlider.value = qValue

                        // Gain value label
                        Label {
                            y: 0
                            text: { let v = Math.round(gainSlider.value); return v > 0 ? "+" + v : v.toString() }
                            color: gainSlider.value === 0 ? "#445566" : (gainSlider.value > 0 ? "#6699cc" : "#cc8866")
                            font.pixelSize: 11; font.bold: gainSlider.value !== 0
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        // Vertical gain slider — always same height, aligned with dB scale
                        Slider {
                            id: gainSlider
                            orientation: Qt.Vertical
                            y: bandArea.sliderTop
                            height: bandArea.sliderH
                            anchors.horizontalCenter: parent.horizontalCenter
                            from: -12; to: 12; stepSize: 1
                            value: gainValue
                            onMoved: updateBand(index, value, qSlider.value)

                            background: Rectangle {
                                x: gainSlider.leftPadding + gainSlider.availableWidth / 2 - width / 2
                                y: gainSlider.topPadding
                                implicitWidth: 6; implicitHeight: 200
                                width: implicitWidth; height: gainSlider.availableHeight
                                radius: 3; color: palette.dark
                                Rectangle {
                                    y: gainSlider.visualPosition * parent.height
                                    width: parent.width
                                    height: (1.0 - gainSlider.visualPosition) * parent.height
                                    radius: 3; color: palette.highlight
                                }
                            }
                            handle: Rectangle {
                                x: gainSlider.leftPadding + gainSlider.availableWidth / 2 - width / 2
                                y: gainSlider.topPadding + gainSlider.visualPosition * (gainSlider.availableHeight - height)
                                implicitWidth: 16; implicitHeight: 16; radius: 8
                                color: gainSlider.pressed ? Qt.darker(palette.highlight, 1.3) : palette.highlight
                                border.color: palette.mid; border.width: 1
                            }
                        }

                        // Frequency label
                        Label {
                            y: bandArea.freqY
                            text: formatFreq(currentBands[index].freq)
                            color: palette.disabled.text; font.pixelSize: 10
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        // Q label (advanced)
                        Label {
                            visible: advancedMode
                            y: bandArea.qLabelY
                            text: "Q " + (qSlider.value % 1 === 0 ? qSlider.value.toFixed(1) : parseFloat(qSlider.value.toFixed(3)).toString())
                            color: palette.disabled.text; font.pixelSize: 10
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        // Q slider (advanced)
                        Slider {
                            id: qSlider; visible: advancedMode
                            y: bandArea.qSliderY
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - 2
                            from: 0.031; to: 8.0; stepSize: 0.001
                            value: qValue
                            onMoved: updateBand(index, gainSlider.value, value)

                            background: Rectangle {
                                x: qSlider.leftPadding
                                y: qSlider.topPadding + qSlider.availableHeight / 2 - height / 2
                                implicitWidth: 40; implicitHeight: 4
                                width: qSlider.availableWidth; height: implicitHeight
                                radius: 2; color: palette.dark
                                Rectangle {
                                    width: qSlider.visualPosition * parent.width
                                    height: parent.height; radius: 2; color: Qt.darker(palette.highlight, 1.8)
                                }
                            }
                            handle: Rectangle {
                                x: qSlider.leftPadding + qSlider.visualPosition * (qSlider.availableWidth - width)
                                y: qSlider.topPadding + qSlider.availableHeight / 2 - height / 2
                                implicitWidth: 12; implicitHeight: 12; radius: 6
                                color: qSlider.pressed ? Qt.darker(palette.highlight, 1.3) : Qt.darker(palette.highlight, 1.8)
                                border.color: palette.mid; border.width: 1
                            }
                        }
                    }
                }
            }

            // Actions under sliders
            RowLayout {
                Layout.fillWidth: true; spacing: 6; Layout.topMargin: -14

                ToggleBtn { label: savingMode ? "Cancel" : "Save as..."; active: false
                    onClicked: { savingMode = !savingMode; renamingMode = false } }

                ToggleBtn { label: "Update"; active: true
                    visible: isCustomPreset && isModified
                    onClicked: overwritePreset() }

                ToggleBtn { label: "Reset"; active: false
                    visible: isModified && savedBands !== null
                    onClicked: resetToSaved() }

                ToggleBtn { label: renamingMode ? "Cancel" : "Rename"; active: false
                    opacity: isCustomPreset ? 1.0 : 0.3
                    onClicked: { if (isCustomPreset) { renamingMode = !renamingMode; savingMode = false } } }

                ToggleBtn { label: "Delete"; active: false
                    opacity: isCustomPreset ? 1.0 : 0.3
                    onClicked: { if (isCustomPreset) { device.deleteEqPreset(activePreset); activePreset = ""; savedBands = null; reloadCustomPresets() } } }

                ToggleBtn { label: "\u25C0"; active: false
                    opacity: isCustomPreset ? 1.0 : 0.3
                    implicitWidth: 32
                    onClicked: { if (isCustomPreset) { device.moveEqPreset(activePreset, -1); reloadCustomPresets() } } }

                ToggleBtn { label: "\u25B6"; active: false
                    opacity: isCustomPreset ? 1.0 : 0.3
                    implicitWidth: 32
                    onClicked: { if (isCustomPreset) { device.moveEqPreset(activePreset, 1); reloadCustomPresets() } } }

                Item { Layout.fillWidth: true }
            }

            // Save name input
            RowLayout {
                visible: savingMode; spacing: 6
                Rectangle {
                    Layout.fillWidth: true; height: 32; radius: 6
                    color: palette.dark; border.color: palette.highlight; border.width: 1
                    TextInput {
                        id: saveNameField; anchors.fill: parent; anchors.margins: 8
                        color: palette.text; font.pixelSize: 12; clip: true
                        onAccepted: savePreset()
                        Text { text: "Preset name"; color: palette.disabled.text; font.pixelSize: 12
                            visible: !parent.text && !parent.activeFocus }
                    }
                }
                ToggleBtn { label: "Save"; active: true; onClicked: savePreset() }
            }

            // Rename input
            RowLayout {
                visible: renamingMode; spacing: 6
                Rectangle {
                    Layout.fillWidth: true; height: 32; radius: 6
                    color: palette.dark; border.color: palette.highlight; border.width: 1
                    TextInput {
                        id: renameField; anchors.fill: parent; anchors.margins: 8
                        color: palette.text; font.pixelSize: 12; clip: true
                        onAccepted: renamePreset()
                        Text { text: activePreset; color: palette.disabled.text; font.pixelSize: 12
                            visible: !parent.text && !parent.activeFocus }
                    }
                }
                ToggleBtn { label: "Rename"; active: true; onClicked: renamePreset() }
            }

            // Noise Gate (only visible in Mic mode)
            ColumnLayout {
                visible: eqType === 0
                Layout.fillWidth: true; spacing: 6; Layout.topMargin: 4
                Label { text: "Noise Gate"; color: palette.text; font.pixelSize: 13; font.bold: true }
                RowLayout {
                    spacing: 6; Layout.fillWidth: true
                    Repeater {
                        model: [{ label: "Home", value: 0x01 }, { label: "Night", value: 0x02 }, { label: "Tournament", value: 0x04 }]
                        Rectangle {
                            property bool isActive: noiseGateState === modelData.value
                            Layout.fillWidth: true; height: 30; radius: 6
                            color: isActive ? palette.highlight : (ngMa.containsMouse ? palette.button : palette.dark)
                            border.color: isActive ? Qt.darker(palette.highlight, 1.3) : palette.mid; border.width: 1
                            Label { anchors.centerIn: parent; text: modelData.label
                                color: parent.isActive ? palette.highlightedText : palette.disabled.text; font.pixelSize: 11 }
                            MouseArea { id: ngMa; anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { device.setNoiseGate(index); noiseGateState = modelData.value } }
                        }
                    }
                }
            }
        }
    }
}
