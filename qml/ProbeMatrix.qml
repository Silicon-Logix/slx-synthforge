// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: matrix
    signal probeRequested(int index)
    spacing: 9
    Text {
        text: "ACQUISITION COVERAGE"
        color: "#18203B"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 6
        Item {
            Layout.preferredWidth: 113
        }
        Repeater {
            model: ["Nominal", "Missing", "Offset", "Polarity", "Bridge"]
            delegate: Text {
                required property string modelData
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: modelData
                color: "#69738C"
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    Repeater {
        model: forge.stress.rows ? 12 : 0
        delegate: RowLayout {
            id: recipeRow
            required property int index
            Layout.fillWidth: true
            spacing: 6
            Text {
                Layout.preferredWidth: 113
                text: forge.stress.rows[recipeRow.index * 5].recipe
                color: "#69738C"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
            Repeater {
                model: 5
                delegate: Button {
                    id: cell
                    required property int index
                    property int probeIndex: recipeRow.index * 5 + index
                    property var probe: forge.stress.rows[probeIndex]
                    property bool correct: probe.prediction.class_index === probe.true_index
                    property bool accepted: probe.prediction.accepted
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    implicitHeight: 25
                    enabled: !forge.busy
                    onClicked: matrix.probeRequested(probeIndex)
                    Accessible.name: probe.recipe + ", " + forge.labels[probe.true_index] + ", " + (accepted ? correct ? "recognized" : "accepted error" : "reacquire")
                    contentItem: Text {
                        text: cell.accepted ? cell.correct ? "OK" : "ERROR" : "RETRY"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 8
                        font.weight: Font.DemiBold
                        color: cell.accepted ? cell.correct ? "#167565" : "#BB4052" : "#A26E1E"
                    }
                    background: Rectangle {
                        radius: 4
                        color: cell.accepted ? cell.correct ? "#E7F4EF" : "#FBE8EB" : "#FCF1DE"
                        border.color: cell.hovered || cell.activeFocus ? "#7476FF" : "transparent"
                    }
                    ToolTip.visible: hovered
                    ToolTip.delay: 350
                    ToolTip.text: forge.labels[probe.prediction.class_index] + " · " + (probe.prediction.confidence * 100).toFixed(1) + "%\n" + probe.prediction.note
                }
            }
        }
    }
    Text {
        Layout.fillWidth: true
        text: "Green: accepted and correct · Amber: reacquire · Red: accepted error. Click a cell to reproduce its recipe with the current model."
        color: "#8791AA"
        font.pixelSize: 9
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.fillWidth: true
        text: "Scan version: " + (forge.stress.model_id || "—")
        color: "#8791AA"
        font.pixelSize: 9
        elide: Text.ElideMiddle
    }
}
