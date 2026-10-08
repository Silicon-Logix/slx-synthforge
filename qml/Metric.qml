// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

Panel {
    id: metric
    property string label
    property string value
    property string detail
    property color valueColor: "#18203B"
    padding: 18
    SectionLabel {
        text: metric.label
    }
    Text {
        text: metric.value
        color: metric.valueColor
        font.pixelSize: 29
        font.weight: Font.DemiBold
    }
    Text {
        text: metric.detail
        color: "#69738C"
        font.pixelSize: 11
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
}
