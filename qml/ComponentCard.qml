// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

Panel {
    id: card
    padding: 16
    bodySpacing: 9
    SectionLabel {
        text: "LINKED COMPONENT"
    }
    RowLayout {
        Layout.fillWidth: true
        Text {
            text: forge.selection.reference || "Select on the board"
            color: "#18203B"
            font.pixelSize: forge.selection.reference ? 23 : 15
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        Text {
            text: forge.selection.visible ? "VISIBLE" : forge.selection.reference ? "EXPECTED POSITION" : ""
            color: "#9B691B"
            font.pixelSize: 8
        }
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: forge.selection.reference ? 130 : 0
        visible: !!forge.selection.reference
        color: "#111832"
        radius: 7
        clip: true
        Image {
            anchors.fill: parent
            anchors.margins: 6
            source: forge.selection.reference ? "image://frame/detail/" + forge.frameVersion : ""
            cache: false
            fillMode: Image.PreserveAspectFit
        }
    }
    Text {
        Layout.fillWidth: true
        text: forge.selection.cad ? forge.selection.cad.value + " · " + forge.selection.cad.class_name.replace(/_/g, " ") : "Click a defect, inspection region or component to link the image to its CAD reference."
        color: "#69738C"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.fillWidth: true
        visible: !!forge.selection.reference
        text: forge.selection.inspected_by_model ? "Frame result: " + (forge.prediction.decision || "Model not loaded") + (forge.prediction.class_index !== undefined ? " · " + forge.labels[forge.prediction.class_index] : "") : "CAD reference only. The model inspects C1 / U4."
        color: "#3436DC"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.fillWidth: true
        text: forge.prediction.model_id || "Synthetic image detail · Original CAD coordinates"
        color: "#8791AA"
        font.pixelSize: 9
        elide: Text.ElideMiddle
    }
    ActionButton {
        visible: !!forge.selection.reference
        text: "Clear selection"
        quiet: true
        onClicked: forge.selectReference("")
    }
}
