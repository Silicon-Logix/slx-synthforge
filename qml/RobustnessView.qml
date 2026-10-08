// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: bench
    signal cycleRequested
    spacing: 18
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 12
        DecisionCard {
            Layout.fillWidth: true
        }
        SceneViewport {
            id: viewport
            Layout.fillWidth: true
            Layout.fillHeight: true
            showEvidence: evidence.checked
        }
        RowLayout {
            Layout.fillWidth: true
            ActionButton {
                id: evidence
                text: "Model evidence"
                checkable: true
                checked: initialModelEvidence
                filled: checked
                enabled: !!forge.modelPath
            }
            ActionButton {
                text: "Reset acquisition"
                onClicked: forge.resetCamera()
            }
            Item {
                Layout.fillWidth: true
            }
            ActionButton {
                text: "Collect case"
                enabled: forge.canCollect && !forge.busy && !forge.rendering && !!forge.cycle.benchmark
                onClicked: forge.collectCase()
            }
        }
        Text {
            Layout.fillWidth: true
            text: evidence.checked ? "Relative contributions to the predicted class in C1 / U4 crops. This overlay is model evidence, not an anomaly segmentation." : "Reacquisition checks use focus, clipped regions, observability, supported acquisition ranges and the score threshold."
            color: "#69738C"
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }
    }
    ScrollView {
        id: controls
        Layout.preferredWidth: 310
        Layout.fillHeight: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: controls.availableWidth
            spacing: 14
            Panel {
                Layout.fillWidth: true
                padding: 16
                bodySpacing: 9
                SectionLabel {
                    text: "OPTICAL STRESS CONTROLS"
                }
                ChoiceBox {
                    Layout.fillWidth: true
                    model: forge.labels
                    currentIndex: forge.sceneIndex
                    onActivated: forge.sceneIndex = currentIndex
                }
                ControlSlider {
                    Layout.fillWidth: true
                    title: "Light exposure"
                    from: .4
                    to: 1.8
                    value: forge.exposure
                    suffix: "×"
                    decimals: 2
                    onEdited: nextValue => forge.exposure = nextValue
                }
                ControlSlider {
                    Layout.fillWidth: true
                    title: "Light direction"
                    from: -110
                    to: 90
                    value: forge.lightAzimuth
                    suffix: "°"
                    decimals: 0
                    onEdited: nextValue => forge.lightAzimuth = nextValue
                }
                ControlSlider {
                    Layout.fillWidth: true
                    title: "Metal reflection gain"
                    from: 0
                    to: 8
                    value: forge.reflection
                    suffix: "×"
                    onEdited: nextValue => forge.reflection = nextValue
                }
                ControlSlider {
                    Layout.fillWidth: true
                    title: "Defocus · σ at 960 px"
                    from: 0
                    to: 6
                    value: forge.blur
                    suffix: " px"
                    onEdited: nextValue => forge.blur = nextValue
                }
                ControlSlider {
                    Layout.fillWidth: true
                    title: "Camera inclination"
                    from: 0
                    to: 60
                    value: forge.tilt
                    suffix: "°"
                    decimals: 0
                    onEdited: nextValue => forge.tilt = nextValue
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#DFE4F0"
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Clipped ROI pixels"
                        color: "#69738C"
                        font.pixelSize: 10
                        Layout.fillWidth: true
                    }
                    Text {
                        text: forge.annotations.quality ? (forge.annotations.quality.roi_clipped_fraction * 100).toFixed(1) + "%" : "—"
                        color: "#18203B"
                        font.pixelSize: 11
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Supported acquisition"
                        color: "#69738C"
                        font.pixelSize: 10
                        Layout.fillWidth: true
                    }
                    Text {
                        text: !forge.modelPath ? "—" : forge.prediction.in_acquisition_domain ? "IN RANGE" : "OUT OF RANGE"
                        color: forge.prediction.in_acquisition_domain ? "#167565" : "#A26E1E"
                        font.pixelSize: 9
                    }
                }
            }
            ComponentCard {
                Layout.fillWidth: true
            }
            Panel {
                Layout.fillWidth: true
                padding: 16
                SectionLabel {
                    text: "TURN FAILURES INTO DATA"
                }
                Text {
                    Layout.fillWidth: true
                    text: "Freeze a separate challenge set, scan development recipes and generate examples around eligible failures."
                    color: "#69738C"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "Open improvement loop →"
                    filled: true
                    onClicked: bench.cycleRequested()
                }
            }
        }
    }
}
