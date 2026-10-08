// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

// Interactive CAD view with scene labels kept distinct from learned predictions.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: studio
    property bool showRegions: true
    property bool showInstances: false
    property bool showScores: false
    signal saveRequested
    spacing: 18
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 12
        SceneViewport {
            Layout.fillWidth: true
            Layout.fillHeight: true
            showRegions: studio.showRegions
            showInstances: studio.showInstances
        }
        RowLayout {
            Layout.fillWidth: true
            ActionButton {
                text: "Inspection regions"
                filled: studio.showRegions && !studio.showInstances
                onClicked: {
                    studio.showRegions = !studio.showRegions;
                    studio.showInstances = false;
                }
            }
            ActionButton {
                text: "All components"
                filled: studio.showInstances
                onClicked: studio.showInstances = !studio.showInstances
            }
            Item {
                Layout.fillWidth: true
            }
            ActionButton {
                text: "Top"
                onClicked: forge.topCamera()
            }
            ActionButton {
                text: "Reset"
                onClicked: forge.resetCamera()
            }
            ActionButton {
                text: "Save PNG"
                onClicked: studio.saveRequested()
            }
        }
        Text {
            Layout.fillWidth: true
            text: "Teal: registered inspection regions   ·   Amber: scene defect label   ·   Predictions appear in the inspector"
            color: "#69738C"
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }
    }
    ScrollView {
        Layout.preferredWidth: 284
        Layout.fillHeight: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: parent.width
            spacing: 14
            Panel {
                Layout.fillWidth: true
                padding: 16
                bodySpacing: 10
                SectionLabel {
                    text: "ASSEMBLY STATE"
                }
                Repeater {
                    model: ["Nominal assembly", "C1 missing", "U4 misaligned", "C1 polarity reversed", "U4 solder bridge"]
                    delegate: Button {
                        id: scenarioButton
                        required property string modelData
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: 34
                        Accessible.name: modelData
                        onClicked: forge.sceneIndex = index
                        contentItem: RowLayout {
                            spacing: 10
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: scenarioButton.index === 0 ? "#167565" : "#C58940"
                            }
                            Text {
                                text: scenarioButton.modelData
                                color: forge.sceneIndex === scenarioButton.index ? "#3436DC" : "#18203B"
                                font.pixelSize: 12
                                font.weight: forge.sceneIndex === scenarioButton.index ? Font.DemiBold : Font.Normal
                                Layout.fillWidth: true
                            }
                            Text {
                                text: forge.sceneIndex === scenarioButton.index ? "●" : ""
                                color: "#3436DC"
                                font.pixelSize: 10
                            }
                        }
                        background: Rectangle {
                            radius: 7
                            color: forge.sceneIndex === scenarioButton.index ? "#F0F0FF" : scenarioButton.hovered ? "#F5F6FB" : "white"
                            border.color: scenarioButton.activeFocus ? "#7476FF" : forge.sceneIndex === scenarioButton.index ? "#D0D0FD" : "#E8ECF4"
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: forge.sceneIndex === 1 ? "Body and terminals removed. Pads and silkscreen remain." : forge.sceneIndex === 2 ? "U4: +1.2 mm X, +0.6 mm Y, rotation +7°." : forge.sceneIndex === 3 ? "C1 body rotated 180°. Board polarity mark stays fixed." : forge.sceneIndex === 4 ? "Metal volume between pads 1 and 2 of U4." : "46 mounted components. Original visual CAD reference."
                    color: "#69738C"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }
            Panel {
                Layout.fillWidth: true
                padding: 16
                bodySpacing: 10
                SectionLabel {
                    text: "VISUAL INSPECTOR"
                }
                Text {
                    text: forge.modelPath ? (forge.prediction.decision || "…") : "Model not trained"
                    color: forge.prediction.decision === "PASS" ? "#167565" : forge.prediction.decision === "DEFECT" ? "#B4484E" : "#69738C"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: forge.modelPath ? forge.labels[forge.prediction.class_index || 0] + "  ·  " + ((forge.prediction.confidence || 0) * 100).toFixed(1) + "%" : "Generate a dataset, then train a model in Model lab."
                    color: "#18203B"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: studio.showScores ? "Hide class scores" : "Show class scores"
                    visible: !!forge.modelPath
                    quiet: true
                    onClicked: studio.showScores = !studio.showScores
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: studio.showScores
                    Repeater {
                        model: forge.prediction.probabilities || []
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: forge.labels[modelData.class_index]
                                    color: "#69738C"
                                    font.pixelSize: 10
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: (modelData.probability * 100).toFixed(1) + "%"
                                    color: "#18203B"
                                    font.pixelSize: 10
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 3
                                radius: 2
                                color: "#EBEEF6"
                                Rectangle {
                                    width: parent.width * modelData.probability
                                    height: 3
                                    radius: 2
                                    color: "#5557E5"
                                }
                            }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: !!forge.modelPath
                    text: forge.prediction.note || ""
                    color: "#69738C"
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }
            ComponentCard {
                Layout.fillWidth: true
            }
            Panel {
                Layout.fillWidth: true
                padding: 16
                bodySpacing: 10
                SectionLabel {
                    text: "CAMERA & LIGHT"
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Inclination"
                        color: "#18203B"
                        font.pixelSize: 11
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Text {
                        text: Math.round(forge.tilt) + "°"
                        color: "#69738C"
                        font.pixelSize: 11
                    }
                }
                ValueSlider {
                    Layout.fillWidth: true
                    from: 0
                    to: 60
                    value: forge.tilt
                    onMoved: forge.tilt = value
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Azimuth"
                        color: "#18203B"
                        font.pixelSize: 11
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Text {
                        text: Math.round(forge.azimuth) + "°"
                        color: "#69738C"
                        font.pixelSize: 11
                    }
                }
                ValueSlider {
                    Layout.fillWidth: true
                    from: -180
                    to: 180
                    value: forge.azimuth
                    onMoved: forge.azimuth = value
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Light exposure"
                        color: "#18203B"
                        font.pixelSize: 11
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Text {
                        text: forge.exposure.toFixed(2) + "×"
                        color: "#69738C"
                        font.pixelSize: 11
                    }
                }
                ValueSlider {
                    Layout.fillWidth: true
                    from: .4
                    to: 1.8
                    value: forge.exposure
                    onMoved: forge.exposure = value
                }
                Text {
                    Layout.fillWidth: true
                    visible: forge.tilt < 18
                    text: "Polarity is ambiguous in top views. Use an inclined view for inspection."
                    color: "#9B691B"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
