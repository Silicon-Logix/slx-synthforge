// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: lab
    signal editorRequested
    property bool showWorkflow: false
    property var cycle: forge.inspectorCycle
    property var comparison: cycle.comparison || ({})
    property var baselineMetrics: comparison.baseline || ({})
    property var candidateMetrics: comparison.candidate || ({})
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
        width: lab.availableWidth
        spacing: 16
        ActionButton {
            visible: forge.hasInspectorExample
            text: "Show example comparison"
            enabled: !forge.busy
            onClicked: forge.loadInspectorExample()
        }
        RowLayout {
            Layout.fillWidth: true
            Metric {
                Layout.fillWidth: true
                label: "INSPECTION"
                value: "46 references"
                detail: "Editable board · RGB → C++"
            }
            Metric {
                Layout.fillWidth: true
                label: "CONFIRMED CASES"
                value: forge.inspectorCases.length
                detail: "Development separate from test"
            }
            Metric {
                Layout.fillWidth: true
                label: "INDEPENDENT TEST"
                value: cycle.benchmark ? "Frozen" : "Not frozen"
                detail: "Same data and threshold for both versions"
            }
        }
        Panel {
            visible: !!comparison.schema
            Layout.fillWidth: true
            SectionLabel {
                text: "COMPARISON ON THE SAME TEST"
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                ComparisonMetric {
                    Layout.fillWidth: true
                    ready: !!lab.comparison.schema
                    label: "MISSED DEFECTS"
                    before: lab.baselineMetrics.missed_defects_raw || 0
                    after: lab.candidateMetrics.missed_defects_raw || 0
                }
                ComparisonMetric {
                    Layout.fillWidth: true
                    ready: !!lab.comparison.schema
                    label: "FALSE ALARMS"
                    before: lab.baselineMetrics.false_alarms_raw || 0
                    after: lab.candidateMetrics.false_alarms_raw || 0
                }
                ComparisonMetric {
                    Layout.fillWidth: true
                    ready: !!lab.comparison.schema
                    label: "REVIEW"
                    before: lab.baselineMetrics.review || 0
                    after: lab.candidateMetrics.review || 0
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: "#69738C"
                font.pixelSize: 12
                text: "Defects accepted as nominal: " + (lab.baselineMetrics.escaped_defects || 0) + " → " + (lab.candidateMetrics.escaped_defects || 0) + " · Accepted false alarms: " + (lab.baselineMetrics.false_alarms || 0) + " → " + (lab.candidateMetrics.false_alarms || 0) + " · Coverage: " + ((lab.baselineMetrics.coverage || 0) * 100).toFixed(1) + "% → " + ((lab.candidateMetrics.coverage || 0) * 100).toFixed(1) + "%"
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: "#9B691B"
                font.pixelSize: 11
                text: "Reducing REVIEW can expose more escaped defects. The JSON report includes the original control test and per-component results. Evaluation uses observable synthetic examples in a fixed view."
            }
            Text {
                Layout.fillWidth: true
                color: "#69738C"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                text: "Test: " + (lab.baselineMetrics.samples || 0) + " examples, " + (lab.baselineMetrics.actual_defects || 0) + " defects / " + (lab.baselineMetrics.actual_nominal || 0) + " nominal. Threshold " + (lab.comparison.threshold || 0.80) + " · Original control: " + (((lab.comparison.control_baseline || {}).accuracy || 0) * 100).toFixed(1) + "% → " + (((lab.comparison.control_candidate || {}).accuracy || 0) * 100).toFixed(1) + "%"
            }
            RowLayout {
                ActionButton {
                    text: "Use baseline"
                    enabled: !forge.busy
                    onClicked: forge.activateInspector(false)
                }
                ActionButton {
                    text: "Use candidate"
                    filled: true
                    enabled: !forge.busy
                    onClicked: forge.activateInspector(true)
                }
                ActionButton {
                    text: "Open HTML / CSV / JSON report"
                    onClicked: forge.openInspectorReport()
                }
            }
        }
        ActionButton {
            visible: !!comparison.schema
            text: lab.showWorkflow ? "Close data and training" : "Data and training"
            onClicked: lab.showWorkflow = !lab.showWorkflow
        }
        RowLayout {
            visible: !comparison.schema || lab.showWorkflow
            Layout.fillWidth: true
            spacing: 16
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "01 / BASELINE AND TEST"
                }
                Text {
                    text: "Train and freeze the baseline"
                    color: "#18203B"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: "The included model works immediately. Generate data for a new baseline or use the included examples. Generating a dataset starts a new cycle."
                    wrapMode: Text.WordWrap
                    color: "#69738C"
                    font.pixelSize: 12
                }
                RowLayout {
                    Text {
                        text: "Acquisition groups"
                        color: "#69738C"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                    ValueSpinBox {
                        id: groups
                        from: 6
                        to: 100
                        value: 12
                    }
                }
                ActionButton {
                    Layout.fillWidth: true
                    text: "Generate inspector dataset"
                    enabled: !forge.busy
                    onClicked: forge.generateInspector(groups.value)
                }
                ActionButton {
                    Layout.fillWidth: true
                    text: "Train baseline"
                    enabled: !forge.busy && !!cycle.dataset && !cycle.benchmark
                    onClicked: forge.trainInspector()
                }
                ActionButton {
                    Layout.fillWidth: true
                    text: "Freeze independent test"
                    filled: true
                    enabled: !forge.busy && !cycle.benchmark
                    onClicked: forge.freezeInspector(6)
                }
                Text {
                    Layout.fillWidth: true
                    text: cycle.benchmark ? "The test is frozen before cases are collected and never enters training." : "Model selection uses validation; the test is used only for evaluation."
                    wrapMode: Text.WordWrap
                    color: "#69738C"
                    font.pixelSize: 11
                }
            }
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "02 / IMPROVEMENT"
                }
                Text {
                    text: "Turn cases into a new version"
                    color: "#18203B"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: "Edit freely, capture an image and select a result. Confirm the case using the synthetic edit audit to generate relevant examples while preserving the test."
                    wrapMode: Text.WordWrap
                    color: "#69738C"
                    font.pixelSize: 12
                }
                ActionButton {
                    Layout.fillWidth: true
                    text: "Open editor and collect cases"
                    enabled: !forge.busy && !!cycle.benchmark
                    onClicked: lab.editorRequested()
                }
                Repeater {
                    model: forge.inspectorCases
                    delegate: Text {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.reference + " · " + (modelData.removed ? "removed" : "position / appearance") + " · " + modelData.prediction.decision
                        color: "#69738C"
                        font.pixelSize: 11
                    }
                }
                ActionButton {
                    Layout.fillWidth: true
                    text: "Generate targeted examples"
                    enabled: !forge.busy && forge.inspectorCases.length > 0
                    onClicked: forge.augmentInspector(6)
                }
                ActionButton {
                    Layout.fillWidth: true
                    text: "Train and compare candidate"
                    filled: true
                    enabled: !forge.busy && !!cycle.augmentation && !!cycle.dataset
                    onClicked: forge.trainInspector()
                }
                Text {
                    Layout.fillWidth: true
                    text: "Training uses Python and NumPy. The candidate remains separate until you explicitly select it."
                    wrapMode: Text.WordWrap
                    color: "#69738C"
                    font.pixelSize: 11
                }
            }
        }
        Text {
            Layout.fillWidth: true
            text: "Active model: " + forge.inspectorModelId
            color: "#8B95AA"
            font.pixelSize: 10
            elide: Text.ElideMiddle
        }
        ProgressBar {
            Layout.fillWidth: true
            visible: forge.busy
            value: forge.progress
        }
        ActionButton {
            visible: forge.busy
            text: "Cancel job"
            onClicked: forge.cancel()
        }
    }
}
