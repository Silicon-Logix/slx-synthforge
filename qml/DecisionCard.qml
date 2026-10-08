// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

Panel {
    id: decision
    property bool hasModel: !!forge.modelPath
    property bool accepted: hasModel && forge.prediction.accepted === true
    property color accent: !hasModel ? "#69738C" : !accepted ? "#A26E1E" : forge.prediction.class_index === 0 ? "#167565" : "#BB4052"
    padding: 18
    bodySpacing: 7
    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            SectionLabel {
                text: "ACQUISITION DECISION"
            }
            Text {
                Layout.fillWidth: true
                text: forge.rendering ? "Acquiring…" : !decision.hasModel ? "Load a model" : decision.accepted ? forge.prediction.decision : "REACQUIRE"
                color: decision.accent
                font.pixelSize: 27
                font.weight: Font.DemiBold
            }
        }
        ColumnLayout {
            spacing: 4
            Text {
                text: decision.hasModel ? (forge.prediction.confidence * 100).toFixed(1) + "%" : "—"
                color: "#18203B"
                font.pixelSize: 25
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignRight
            }
            Text {
                text: "MODEL SCORE"
                font.pixelSize: 8
                color: "#8791AA"
                Layout.alignment: Qt.AlignRight
            }
        }
    }
    Text {
        Layout.fillWidth: true
        text: decision.hasModel ? forge.labels[forge.prediction.class_index] + " · " + forge.prediction.note : "Use the included baseline or train a model in Model lab."
        color: "#69738C"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.fillWidth: true
        visible: decision.hasModel && !forge.rendering && forge.prediction.class_index !== forge.annotations.class_index
        text: "Prediction differs from the synthetic scene label. Collect this development case when acquisition quality allows it."
        color: "#BB4052"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }
}
