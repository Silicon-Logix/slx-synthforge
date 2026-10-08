// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls

Slider {
    id: control
    implicitHeight: 24
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.availableWidth
        height: 4
        radius: 2
        color: "#E5E9F4"
        Rectangle {
            width: control.visualPosition * parent.width
            height: 4
            radius: 2
            color: "#5B5DEA"
        }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: 16
        implicitHeight: 16
        radius: 8
        color: control.pressed ? "#E9E9FF" : "white"
        border.color: control.activeFocus ? "#3436DC" : "#8486E8"
        border.width: 2
    }
}
