// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

Panel {
    id: metric
    property string label
    property string field
    property string detail
    property bool ready: forge.comparison.before !== undefined
    property int before: ready ? forge.comparison.before[field] : 0
    property int after: ready ? forge.comparison.after[field] : 0
    padding: 16
    bodySpacing: 8
    SectionLabel {
        text: metric.label
    }
    RowLayout {
        Layout.fillWidth: true
        Text {
            text: metric.ready ? metric.before + " → " + metric.after : "—"
            color: "#18203B"
            font.pixelSize: 27
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        Text {
            visible: metric.ready
            text: (metric.after > metric.before ? "+" : "") + (metric.after - metric.before)
            color: metric.after < metric.before ? "#167565" : metric.after > metric.before ? "#BB4052" : "#69738C"
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
    }
    Text {
        Layout.fillWidth: true
        text: metric.detail
        color: "#69738C"
        font.pixelSize: 10
        wrapMode: Text.WordWrap
    }
}
