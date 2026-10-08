// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls

SpinBox {
    id: control
    implicitHeight: 36
    implicitWidth: 150
    leftPadding: 36
    rightPadding: 36
    font.pixelSize: 12
    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: "#18203B"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        selectByMouse: true
        inputMethodHints: Qt.ImhDigitsOnly
    }
    up.indicator: Rectangle {
        x: control.width - width
        height: parent.height
        width: 32
        radius: 6
        color: control.up.pressed ? "#E7E8FF" : control.up.hovered ? "#F0F1FF" : "transparent"
        Text {
            anchors.centerIn: parent
            text: "+"
            font.pixelSize: 19
            color: control.enabled ? "#3436DC" : "#99A2B8"
        }
    }
    down.indicator: Rectangle {
        x: 0
        height: parent.height
        width: 32
        radius: 6
        color: control.down.pressed ? "#E7E8FF" : control.down.hovered ? "#F0F1FF" : "transparent"
        Text {
            anchors.centerIn: parent
            text: "−"
            font.pixelSize: 19
            color: control.enabled ? "#3436DC" : "#99A2B8"
        }
    }
    background: Rectangle {
        radius: 7
        color: control.enabled ? "white" : "#F5F6FB"
        border.color: control.activeFocus ? "#7476FF" : "#DFE4F0"
    }
}
