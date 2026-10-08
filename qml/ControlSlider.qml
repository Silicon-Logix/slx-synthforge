// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: control
    property string title
    property real from: 0
    property real to: 1
    property real value: 0
    property int decimals: 1
    property string suffix
    signal edited(real nextValue)
    signal gestureStarted
    signal gestureFinished
    spacing: 2
    RowLayout {
        Layout.fillWidth: true
        Text {
            text: control.title
            color: "#18203B"
            font.pixelSize: 11
            Layout.fillWidth: true
        }
        Text {
            text: control.value.toFixed(control.decimals) + control.suffix
            color: "#69738C"
            font.pixelSize: 11
            font.family: "Consolas"
        }
    }
    ValueSlider {
        Layout.fillWidth: true
        from: control.from
        to: control.to
        value: control.value
        onMoved: control.edited(value)
        onPressedChanged: pressed ? control.gestureStarted() : control.gestureFinished()
    }
}
