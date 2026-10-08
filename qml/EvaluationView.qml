// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: results
    signal inspectRequested(int sampleIndex)
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    property var report: forge.evaluation
    property var filtered: {
        var values = report.predictions || [];
        var output = [];
        for (var i = 0; i < values.length; i++)
            if (filter.currentIndex === 0 || (filter.currentIndex === 1 && !values[i].correct) || (filter.currentIndex === 2 && values[i].confidence < forge.threshold))
                output.push(values[i]);
        return output;
    }
    ColumnLayout {
        width: results.availableWidth
        spacing: 18
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Metric {
                Layout.fillWidth: true
                label: "TEST ACCURACY"
                value: results.report.test ? (results.report.test.accuracy * 100).toFixed(1) + "%" : "—"
                detail: "Measured on held-out synthetic images"
                valueColor: "#3436DC"
            }
            Metric {
                Layout.fillWidth: true
                label: "MACRO F1"
                value: results.report.test ? results.report.test.macro_f1.toFixed(3) : "—"
                detail: "Equal weight to each assembly state"
            }
            Metric {
                Layout.fillWidth: true
                label: "TEST SAMPLES"
                value: results.report.predictions ? String(results.report.predictions.length) : "—"
                detail: "Acquisition groups unseen in training"
            }
        }
        Panel {
            Layout.fillWidth: true
            visible: !results.report.test
            Text {
                text: "Train a model to see measured results"
                color: "#18203B"
                font.pixelSize: 19
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: "Evaluation includes a confusion matrix, per-class precision and recall, and an image gallery for errors and uncertain predictions."
                color: "#69738C"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 18
            visible: !!results.report.test
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "CONFUSION MATRIX"
                }
                Text {
                    text: "Rows: true state  ·  Columns: prediction"
                    color: "#69738C"
                    font.pixelSize: 11
                }
                GridLayout {
                    columns: 6
                    columnSpacing: 5
                    rowSpacing: 5
                    Layout.fillWidth: true
                    Text {
                        text: ""
                    }
                    Repeater {
                        model: ["OK", "MISS", "ALIGN", "POL", "BRIDGE"]
                        Text {
                            required property string modelData
                            text: modelData
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            color: "#69738C"
                            font.pixelSize: 9
                            font.weight: Font.DemiBold
                        }
                    }
                    Repeater {
                        model: 30
                        delegate: Rectangle {
                            required property int index
                            property int row: Math.floor(index / 6)
                            property int col: index % 6 - 1
                            property int amount: col < 0 ? 0 : (results.report.test ? results.report.test.confusion_matrix[row][col] : 0)
                            Layout.fillWidth: true
                            Layout.preferredWidth: col < 0 ? 48 : -1
                            Layout.minimumWidth: col < 0 ? 48 : 0
                            Layout.preferredHeight: 40
                            color: col < 0 ? "transparent" : amount > 0 ? (row === col ? "#E7E8FF" : "#FBEAED") : "#F5F6FB"
                            radius: 5
                            Text {
                                anchors.centerIn: parent
                                text: parent.col < 0 ? ["OK", "MISS", "ALIGN", "POL", "BRIDGE"][parent.row] : String(parent.amount)
                                color: parent.col < 0 ? "#69738C" : parent.amount > 0 ? (parent.row === parent.col ? "#3436DC" : "#B4484E") : "#A3ACBF"
                                font.pixelSize: parent.col < 0 ? 9 : 14
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: "OK: nominal · MISS: missing · ALIGN: misaligned · POL: reversed · BRIDGE: solder bridge"
                    color: "#69738C"
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "PER-CLASS PERFORMANCE"
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "State"
                        Layout.fillWidth: true
                        color: "#69738C"
                        font.pixelSize: 10
                    }
                    Text {
                        text: "Precision"
                        Layout.preferredWidth: 60
                        color: "#69738C"
                        font.pixelSize: 10
                    }
                    Text {
                        text: "Recall"
                        Layout.preferredWidth: 52
                        color: "#69738C"
                        font.pixelSize: 10
                    }
                    Text {
                        text: "F1"
                        Layout.preferredWidth: 43
                        color: "#69738C"
                        font.pixelSize: 10
                    }
                }
                Repeater {
                    model: results.report.test ? results.report.test.per_class : []
                    delegate: ColumnLayout {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: 11
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: forge.labels[index]
                                Layout.fillWidth: true
                                color: "#18203B"
                                font.pixelSize: 11
                            }
                            Text {
                                text: (modelData.precision * 100).toFixed(0) + "%"
                                Layout.preferredWidth: 60
                                color: "#18203B"
                                font.pixelSize: 11
                            }
                            Text {
                                text: (modelData.recall * 100).toFixed(0) + "%"
                                Layout.preferredWidth: 52
                                color: "#18203B"
                                font.pixelSize: 11
                            }
                            Text {
                                text: modelData.f1.toFixed(2)
                                Layout.preferredWidth: 43
                                color: "#3436DC"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#EDF0F6"
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: "Precision: how often a prediction is correct. Recall: how many examples of that state are found."
                    color: "#69738C"
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "Open report folder"
                    onClicked: forge.openDataset()
                }
            }
        }
        Panel {
            Layout.fillWidth: true
            visible: !!results.report.test
            RowLayout {
                Layout.fillWidth: true
                SectionLabel {
                    text: "TEST IMAGE REVIEW"
                }
                Item {
                    Layout.fillWidth: true
                }
                ChoiceBox {
                    id: filter
                    model: ["All test images", "Errors only", "Below review threshold"]
                    implicitWidth: 205
                }
            }
            Text {
                Layout.fillWidth: true
                text: results.filtered.length + " images  ·  Select an image to compare its scene label and C++ prediction in Scene studio."
                color: "#69738C"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            Text {
                visible: results.filtered.length === 0
                text: "No test images match this filter."
                color: "#167565"
                font.pixelSize: 13
                Layout.topMargin: 12
                Layout.bottomMargin: 12
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 12
                rowSpacing: 12
                Repeater {
                    model: results.visible ? results.filtered : []
                    delegate: Rectangle {
                        id: tile
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        radius: 8
                        color: "#F7F8FC"
                        border.color: modelData.correct ? "#DFE4F0" : "#E5B7BC"
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 6
                            Image {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                source: forge.sampleUrl(tile.modelData.image)
                                sourceSize: Qt.size(320, 240)
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                            }
                            Text {
                                text: "True: " + forge.labels[tile.modelData.true_index]
                                color: "#69738C"
                                font.pixelSize: 10
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "AI: " + forge.labels[tile.modelData.predicted_index] + "  ·  " + (tile.modelData.confidence * 100).toFixed(1) + "%"
                                color: tile.modelData.correct ? "#167565" : "#B4484E"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: results.inspectRequested(tile.modelData.sample_index)
                        }
                    }
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: !!results.report.test
            text: "Results describe synthetic PowerBoard images, known inspection regions and single defects. Real inspection performance requires real images and a separate validation campaign."
            color: "#69738C"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
}
