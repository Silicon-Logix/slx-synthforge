// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: improvement
    signal inspectRequested(int caseIndex)
    signal probeRequested(int probeIndex)
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
        width: improvement.availableWidth
        spacing: 18
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: heroContent.implicitHeight + 38
            radius: 12
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0
                    color: "#18213D"
                }
                GradientStop {
                    position: 1
                    color: "#363486"
                }
            }
            RowLayout {
                id: heroContent
                anchors.fill: parent
                anchors.margins: 19
                spacing: 20
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7
                    Text {
                        text: "MAKE EACH FAILURE USEFUL"
                        color: "#AAAFF7"
                        font.pixelSize: 9
                        font.letterSpacing: 1.3
                    }
                    Text {
                        text: "From inspection errors to better coverage"
                        color: "white"
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: "Collect development cases. Add targeted examples. Compare immutable model versions on the same separate test."
                        color: "#BDC6E4"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }
                ColumnLayout {
                    Layout.preferredWidth: 168
                    Text {
                        text: forge.cycle.benchmark ? "TEST FROZEN" : "TEST TO PREPARE"
                        color: forge.cycle.benchmark ? "#6CDEC3" : "#FFCC66"
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: forge.cycle.benchmark ? forge.cycle.benchmark_samples + " independent images" : "A separate challenge set"
                        color: "white"
                        font.pixelSize: 12
                    }
                    Text {
                        text: forge.cases.length + " development cases"
                        color: "#BDC6E4"
                        font.pixelSize: 11
                    }
                    Text {
                        text: forge.cycle.threshold !== undefined ? "Fixed comparison threshold " + forge.cycle.threshold.toFixed(2) : "Freeze before collecting cases"
                        color: "#AAAFF7"
                        font.pixelSize: 9
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 18
            Panel {
                Layout.preferredWidth: 295
                Layout.alignment: Qt.AlignTop
                bodySpacing: 12
                SectionLabel {
                    text: "IMPROVEMENT WORKFLOW"
                }
                Text {
                    text: "1 · Freeze the benchmark"
                    color: "#18203B"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Independent randomized acquisitions, reserved exclusively for comparing both versions."
                    color: "#69738C"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: forge.cycle.benchmark ? "Benchmark frozen" : "Freeze 100 test images"
                    enabled: !forge.busy && !!forge.modelPath && !forge.cycle.benchmark
                    onClicked: forge.freezeBenchmark(20)
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#DFE4F0"
                }
                Text {
                    text: "2 · Discover the failures"
                    color: "#18203B"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Probe 12 optical recipes across all five states. Collect errors and supported-range gaps; severe blur and unreadable views require reacquisition."
                    color: "#69738C"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "Scan & collect cases"
                    enabled: !forge.busy && !!forge.modelPath && !!forge.cycle.benchmark
                    onClicked: forge.scanStress()
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#DFE4F0"
                }
                Text {
                    text: "3 · Add pertinent examples"
                    color: "#18203B"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "225 new images: neighbors of failed recipes plus broad optical variations. Grouped train / validation only."
                    color: "#69738C"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "Generate targeted data"
                    enabled: !forge.busy && forge.cases.length > 0 && !!forge.cycle.benchmark
                    onClicked: forge.prepareImprovement(45)
                }
                Text {
                    visible: !!forge.cycle.augmentation
                    text: "Prepared from " + forge.cycle.prepared_case_count + " cases"
                    color: "#167565"
                    font.pixelSize: 10
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#DFE4F0"
                }
                Text {
                    text: "4 · Train and compare"
                    color: "#18203B"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Keep the previous version. Measure missed defects, false alarms and reacquisition requests on the frozen challenge."
                    color: "#69738C"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "Train candidate & compare"
                    filled: true
                    enabled: !forge.busy && !!forge.modelPath && !!forge.cycle.augmentation && forge.cycle.prepared_case_count === forge.cases.length
                    onClicked: forge.trainImprovement(100)
                }
                ActionButton {
                    text: "Cycle artifacts"
                    quiet: true
                    enabled: !!forge.cycle.benchmark
                    onClicked: forge.openCycle()
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 16
                Panel {
                    Layout.fillWidth: true
                    padding: 18
                    bodySpacing: 10
                    SectionLabel {
                        text: "MODEL VERSIONS"
                    }
                    ChoiceBox {
                        Layout.fillWidth: true
                        enabled: !forge.busy && forge.versions.length > 0
                        model: forge.versions.map(v => v.label)
                        currentIndex: {
                            for (let i = 0; i < forge.versions.length; ++i)
                                if (forge.versions[i].id === forge.prediction.model_id)
                                    return i;
                            return -1;
                        }
                        displayText: currentIndex >= 0 ? forge.versions[currentIndex].label : forge.modelPath ? forge.prediction.model_id : "No model loaded"
                        onActivated: forge.loadModel("file:///" + forge.versions[currentIndex].path.replace(/\\/g, "/"))
                    }
                    Text {
                        Layout.fillWidth: true
                        text: forge.prediction.model_id || "Load a dataset with its model or train a baseline first."
                        color: "#69738C"
                        font.pixelSize: 10
                        elide: Text.ElideMiddle
                    }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 14
                    ComparisonMetric {
                        Layout.fillWidth: true
                        label: "MISSED DEFECTS"
                        field: "missed_defects"
                        detail: forge.comparison.after ? "Raw defect → nominal / " + forge.comparison.after.defect_samples + " defective images" : "Previous → candidate on a separate test"
                    }
                    ComparisonMetric {
                        Layout.fillWidth: true
                        label: "FALSE ALARMS"
                        field: "false_alarms"
                        detail: forge.comparison.after ? "Raw nominal → defect / " + forge.comparison.after.nominal_samples + " nominal images" : "Same nominal images for both versions"
                    }
                    ComparisonMetric {
                        Layout.fillWidth: true
                        label: "BRIDGES NOT RECOGNIZED"
                        field: "missed_bridges"
                        detail: forge.comparison.after ? "Bridge → another class / " + forge.comparison.after.bridge_samples + " bridges" : "Includes wrong defect types"
                    }
                    ComparisonMetric {
                        Layout.fillWidth: true
                        label: "REACQUISITION REQUESTS"
                        field: "reacquire"
                        detail: forge.comparison.after ? "Accepted coverage " + (forge.comparison.before.coverage * 100).toFixed(0) + "% → " + (forge.comparison.after.coverage * 100).toFixed(0) + "%" : "Deferred images counted separately"
                    }
                }
                Panel {
                    Layout.fillWidth: true
                    padding: 18
                    bodySpacing: 9
                    SectionLabel {
                        text: "COMPARISON EVIDENCE"
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: !!forge.comparison.after
                        text: forge.comparison.after && forge.comparison.after.escaped_defects > 0 ? "Candidate still has accepted misses" : "Assess errors together with accepted coverage"
                        color: forge.comparison.after && forge.comparison.after.escaped_defects > 0 ? "#BB4052" : "#69738C"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: forge.comparison.after ? "Escaped defects on accepted decisions: " + forge.comparison.before.escaped_defects + " → " + forge.comparison.after.escaped_defects + ". Accepted false alarms: " + forge.comparison.before.accepted_false_alarms + " → " + forge.comparison.after.accepted_false_alarms + "." : "The report will show the measured result, including regressions. No improvement is assumed in advance."
                        color: "#18203B"
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Raw class errors include deferred images. A missed bridge may be assigned another defect type. Model scores are uncalibrated; results concern this synthetic C1 / U4 demonstrator."
                        color: "#69738C"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: !!forge.comparison.reference_test
                        text: forge.comparison.reference_test ? "Original acquisition test: " + (forge.comparison.reference_test.before.accuracy * 100).toFixed(1) + "% → " + (forge.comparison.reference_test.after.accuracy * 100).toFixed(1) + "% accuracy on " + forge.comparison.reference_test.after.samples + " images. Missed defects " + forge.comparison.reference_test.before.missed_defects + " → " + forge.comparison.reference_test.after.missed_defects + "." : ""
                        color: forge.comparison.reference_test && forge.comparison.reference_test.after.missed_defects > forge.comparison.reference_test.before.missed_defects ? "#BB4052" : "#69738C"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                    ActionButton {
                        text: "Open shareable report ↗"
                        enabled: !!forge.comparison.after
                        onClicked: forge.openComparison()
                    }
                }
                Panel {
                    Layout.fillWidth: true
                    padding: 18
                    bodySpacing: 9
                    SectionLabel {
                        text: "DEVELOPMENT STRESS SCAN"
                    }
                    Text {
                        Layout.fillWidth: true
                        text: forge.stress.samples ? forge.stress.correct + " / " + forge.stress.samples + " correct classes · " + forge.stress.accepted + " accepted · " + forge.stress.reacquire + " reacquisitions" : "Run the scan to find acquisition limits and collect examples."
                        color: "#18203B"
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: forge.stress.samples ? "Raw missed defects " + forge.stress.missed_defects + " · false alarms " + forge.stress.false_alarms + " · unrecognized bridges " + forge.stress.missed_bridges : "Development probes remain separate from the frozen challenge."
                        color: "#69738C"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                    ProbeMatrix {
                        Layout.fillWidth: true
                        visible: !!forge.stress.samples
                        onProbeRequested: index => improvement.probeRequested(index)
                    }
                }
            }
        }
        Panel {
            Layout.fillWidth: true
            SectionLabel {
                text: "COLLECTED DEVELOPMENT CASES / " + forge.cases.length
            }
            Text {
                Layout.fillWidth: true
                text: "Each case retains the synthetic label, acquisition recipe and original model result. Select a case to inspect it with the current version."
                color: "#69738C"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            GridView {
                id: caseGrid
                Layout.fillWidth: true
                Layout.preferredHeight: forge.cases.length ? Math.min(420, Math.ceil(forge.cases.length / Math.max(1, Math.floor(width / 205))) * cellHeight) : 70
                clip: true
                cellWidth: width / Math.max(1, Math.floor(width / 205))
                cellHeight: 194
                model: improvement.visible ? forge.cases : []
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: caseGrid.cellWidth - 12
                    height: 182
                    radius: 8
                    color: "#F6F7FC"
                    border.color: caseMouse.containsMouse ? "#7476FF" : "#DFE4F0"
                    Image {
                        x: 8
                        y: 8
                        width: parent.width - 16
                        height: 106
                        source: forge.caseUrl(modelData.image)
                        sourceSize.width: 280
                        fillMode: Image.PreserveAspectFit
                    }
                    Text {
                        x: 10
                        y: 122
                        text: modelData.category
                        color: modelData.category === "Reacquisition" ? "#A26E1E" : "#BB4052"
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }
                    Text {
                        x: 10
                        y: 141
                        width: parent.width - 20
                        text: forge.labels[modelData.true_index] + " → " + forge.labels[modelData.prediction.class_index]
                        color: "#69738C"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Text {
                        x: 10
                        y: 158
                        text: "Collected score " + (modelData.prediction.confidence * 100).toFixed(1) + "%"
                        color: "#8791AA"
                        font.pixelSize: 9
                    }
                    MouseArea {
                        id: caseMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        enabled: !forge.busy
                        onClicked: improvement.inspectRequested(index)
                    }
                }
                Text {
                    visible: !forge.cases.length
                    anchors.centerIn: parent
                    text: "No development cases collected yet"
                    color: "#8791AA"
                    font.pixelSize: 12
                }
            }
        }
    }
}
