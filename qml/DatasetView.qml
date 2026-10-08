// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: datasetView
    signal importRequested
    signal workspaceRequested
    signal trainRequested
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
        width: datasetView.availableWidth
        spacing: 18
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Metric {
                Layout.fillWidth: true
                label: "IMAGES"
                value: forge.dataset.samples ? String(forge.dataset.samples.length) : "—"
                detail: "Five balanced assembly states"
            }
            Metric {
                Layout.fillWidth: true
                label: "TRAIN / VALIDATION / TEST"
                value: forge.dataset.split_counts ? forge.dataset.split_counts.train + " / " + forge.dataset.split_counts.validation + " / " + forge.dataset.split_counts.test : "60 / 20 / 20%"
                detail: "Separated by acquisition group"
            }
            Metric {
                Layout.fillWidth: true
                label: "ANNOTATIONS"
                value: "RGB + IDs"
                detail: "Instance masks · JSON · YOLO boxes"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 18
            Layout.alignment: Qt.AlignTop
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "GENERATION RECIPE"
                }
                Text {
                    text: "Build your synthetic inspection set"
                    color: "#18203B"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: "Each acquisition renders all five PowerBoard states with the same camera and lighting. A fixed seed makes the dataset repeatable."
                    color: "#69738C"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 20
                    rowSpacing: 12
                    Text {
                        text: "Acquisition groups"
                        color: "#18203B"
                        font.pixelSize: 12
                    }
                    ValueSpinBox {
                        id: groups
                        from: 15
                        to: 500
                        value: 60
                        editable: true
                        enabled: !forge.busy
                        Layout.fillWidth: true
                    }
                    Text {
                        text: "Random seed"
                        color: "#18203B"
                        font.pixelSize: 12
                    }
                    ValueSpinBox {
                        id: seed
                        from: 1
                        to: 99999999
                        value: 2026
                        editable: true
                        enabled: !forge.busy
                        Layout.fillWidth: true
                    }
                }
                ChoiceBox {
                    Layout.fillWidth: true
                    model: ["Standard acquisition profile", "Robustness profile · light, reflection, focus"]
                    currentIndex: forge.robustDataset ? 1 : 0
                    enabled: !forge.busy
                    onActivated: forge.robustDataset = currentIndex === 1
                }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 62
                    radius: 8
                    color: "#F3F4FF"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 13
                        spacing: 5
                        Text {
                            text: groups.value * 5 + " images at 960 × 720"
                            color: "#3436DC"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: forge.robustDataset ? "Camera 18–52° · Reflection 0–4× · Defocus σ 0–2 px" : "Camera 24–44° · Azimuth −80…−50° · Exposure 0.75–1.30×"
                            color: "#69738C"
                            font.pixelSize: 10
                        }
                    }
                }
                Text {
                    text: "OUTPUT FOLDER"
                    color: "#69738C"
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: forge.workspace
                    color: "#18203B"
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                }
                RowLayout {
                    Layout.fillWidth: true
                    ActionButton {
                        text: "Choose folder"
                        enabled: !forge.busy
                        onClicked: datasetView.workspaceRequested()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ActionButton {
                        text: "Generate dataset"
                        filled: true
                        enabled: !forge.busy
                        onClicked: forge.generate(groups.value, seed.value)
                    }
                }
            }
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "ACQUISITION & GROUND TRUTH"
                }
                Repeater {
                    model: [
                        {
                            title: "Camera variation",
                            body: "Orthographic views with varied inclination, azimuth and framing. Inclined views expose the C1 polarity band."
                        },
                        {
                            title: "Lighting variation",
                            body: "Directional illumination, exposure and deterministic sensor noise change independently of the defect class."
                        },
                        {
                            title: "Geometry-derived labels",
                            body: "Depth-tested instance IDs produce visible boxes. A missing component uses its expected inspection region."
                        },
                        {
                            title: "Capture groups stay together",
                            body: "All variants of one camera/light configuration belong wholly to train, validation or test."
                        }
                    ]
                    delegate: ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            text: modelData.title
                            color: "#18203B"
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.body
                            color: "#69738C"
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#EDF0F6"
                            Layout.topMargin: 6
                        }
                    }
                }
            }
        }
        Panel {
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                SectionLabel {
                    text: "ACTIVE DATASET"
                }
                Item {
                    Layout.fillWidth: true
                }
                ActionButton {
                    text: "Import dataset"
                    enabled: !forge.busy
                    onClicked: datasetView.importRequested()
                }
            }
            Text {
                Layout.fillWidth: true
                text: forge.dataset.id || "No dataset loaded yet"
                color: "#18203B"
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: forge.datasetPath || "Generate a set above or reopen an existing dataset folder."
                color: "#69738C"
                font.pixelSize: 11
                elide: Text.ElideMiddle
            }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Every run creates a new folder and preserves previous datasets."
                    color: "#69738C"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "Open folder"
                    enabled: !!forge.datasetPath
                    onClicked: forge.openDataset()
                }
                ActionButton {
                    text: "Continue to Model lab"
                    filled: true
                    enabled: !!forge.datasetPath && !forge.busy
                    onClicked: datasetView.trainRequested()
                }
            }
        }
        Panel {
            Layout.fillWidth: true
            SectionLabel {
                text: "PYTHON RUNTIME"
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: runtime
                    Layout.fillWidth: true
                    text: forge.python
                    enabled: !forge.busy
                    placeholderText: "python or full path to python.exe"
                    onEditingFinished: forge.python = text
                }
                ActionButton {
                    text: "Apply"
                    enabled: !forge.busy
                    onClicked: forge.python = runtime.text
                }
            }
            Text {
                Layout.fillWidth: true
                text: "Training requires NumPy. Rendering and C++ inference run without Python. Setup: python -m pip install -r tools/requirements.txt"
                color: "#69738C"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }
    }
}
