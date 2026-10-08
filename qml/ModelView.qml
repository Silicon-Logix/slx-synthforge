// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: lab
    signal importRequested
    signal evaluationRequested
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
        width: lab.availableWidth
        spacing: 18
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Metric {
                Layout.fillWidth: true
                label: "MODEL"
                value: "ROI softmax"
                detail: "Trained from C1 / U4 RGB crops"
            }
            Metric {
                Layout.fillWidth: true
                label: "FEATURES"
                value: "3,456"
                detail: "Two fixed 24 × 24 RGB regions"
            }
            Metric {
                Layout.fillWidth: true
                label: "EXECUTION"
                value: "Local CPU"
                detail: "Python training · C++ inference"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 18
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "TRAINING RUN"
                }
                Text {
                    text: "Learn from synthetic images"
                    color: "#18203B"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: "A compact visual classifier is a reproducible first baseline. It sees RGB pixels in registered C1/U4 regions and predicts one of the five prepared assembly states."
                    color: "#69738C"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: forge.dataset.id || "Load a dataset in Dataset forge first."
                    color: "#3436DC"
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Training epochs"
                        color: "#18203B"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                    ValueSpinBox {
                        id: epochs
                        from: 20
                        to: 400
                        value: 100
                        editable: true
                        enabled: !forge.busy
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Review threshold"
                        color: "#18203B"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                    Text {
                        text: Math.round(forge.threshold * 100) + "%"
                        color: "#69738C"
                        font.pixelSize: 12
                    }
                }
                ValueSlider {
                    Layout.fillWidth: true
                    from: .4
                    to: .99
                    value: forge.threshold
                    onMoved: forge.threshold = value
                }
                Text {
                    Layout.fillWidth: true
                    text: "Scores are not calibrated. Low scores, an insufficient view or acquisition parameters outside training go to REVIEW."
                    color: "#69738C"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    ActionButton {
                        text: "Import model"
                        enabled: !forge.busy
                        onClicked: lab.importRequested()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ActionButton {
                        text: "Train & evaluate"
                        filled: true
                        enabled: !!forge.datasetPath && !forge.busy
                        onClicked: forge.train(epochs.value)
                    }
                }
            }
            Panel {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                SectionLabel {
                    text: "TRAINING EVIDENCE"
                }
                Text {
                    text: forge.evaluation.history ? "Loss by epoch" : "Waiting for a training run"
                    color: "#18203B"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }
                Canvas {
                    id: lossChart
                    Layout.fillWidth: true
                    Layout.preferredHeight: 230
                    property var history: forge.evaluation.history || []
                    onHistoryChanged: requestPaint()
                    onWidthChanged: requestPaint()
                    onPaint: {
                        var c = getContext("2d");
                        c.reset();
                        var left = 32, top = 14, w = width - 46, h = height - 42;
                        c.strokeStyle = "#E6EAF3";
                        c.lineWidth = 1;
                        for (var k = 0; k < 5; k++) {
                            var gy = top + h * k / 4;
                            c.beginPath();
                            c.moveTo(left, gy);
                            c.lineTo(left + w, gy);
                            c.stroke();
                        }
                        if (!history.length) {
                            c.fillStyle = "#99A2B8";
                            c.font = "12px Segoe UI";
                            c.fillText("Actual training curves appear here", left, top + h / 2);
                            return;
                        }
                        var maxLoss = .1;
                        for (var i = 0; i < history.length; i++)
                            maxLoss = Math.max(maxLoss, history[i].train_loss, history[i].validation_loss);
                        for (var series = 0; series < 2; series++) {
                            c.strokeStyle = series === 0 ? "#3436DC" : "#B02FE2";
                            c.lineWidth = 2;
                            c.beginPath();
                            for (i = 0; i < history.length; i++) {
                                var loss = series === 0 ? history[i].train_loss : history[i].validation_loss;
                                var px = left + w * i / Math.max(1, history.length - 1), py = top + h * (1 - loss / maxLoss);
                                if (i === 0)
                                    c.moveTo(px, py);
                                else
                                    c.lineTo(px, py);
                            }
                            c.stroke();
                        }
                        c.fillStyle = "#69738C";
                        c.font = "10px Segoe UI";
                        c.fillText(maxLoss.toFixed(2), 0, top + 8);
                        c.fillText("0", 15, top + h);
                        c.fillText("1", left, top + h + 22);
                        c.fillText(String(history.length), left + w - 20, top + h + 22);
                    }
                }
                RowLayout {
                    spacing: 18
                    Text {
                        text: "● Training"
                        color: "#3436DC"
                        font.pixelSize: 11
                    }
                    Text {
                        text: "● Validation"
                        color: "#B02FE2"
                        font.pixelSize: 11
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: forge.evaluation.selected_epoch ? "Selected epoch " + forge.evaluation.selected_epoch + " by validation loss. Test data used only after selection." : "Normalization is fitted on training data. Validation selects the best epoch; test stays held out."
                    color: "#69738C"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }
        }
        Panel {
            Layout.fillWidth: true
            SectionLabel {
                text: "ACTIVE MODEL"
            }
            Text {
                Layout.fillWidth: true
                text: forge.modelPath ? forge.modelPath.split(/[\\/]/).pop() + "  ?  " + (forge.dataset.id || "") : "No visual model loaded"
                color: "#18203B"
                font.pixelSize: 12
                elide: Text.ElideMiddle
            }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: "The model and evaluation are saved with the dataset. Inference uses the private C++ core."
                    color: "#69738C"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    text: "View evaluation"
                    filled: true
                    enabled: !!forge.evaluation.test
                    onClicked: lab.evaluationRequested()
                }
            }
        }
        Text {
            Layout.fillWidth: true
            text: "Demo scope: known board, registered regions and one defect per image. Synthetic test results do not measure performance on real production boards."
            color: "#69738C"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
}
