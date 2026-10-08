// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls

Button {
    id: action
    property bool filled: false
    property bool quiet: false
    implicitHeight: 38
    leftPadding: 16
    rightPadding: 16
    font.pixelSize: 12
    font.weight: Font.DemiBold
    contentItem: Text {
        text: action.text
        font: action.font
        color: !action.enabled ? "#99A2B8" : action.filled ? "white" : "#18203B"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 7
        color: !action.enabled ? "#EBEDF5" : action.filled ? (action.down ? "#282AB8" : action.hovered ? "#484AE7" : "#3436DC") : action.hovered ? "#F0F2FB" : action.quiet ? "transparent" : "white"
        border.color: action.activeFocus ? "#7476FF" : action.filled ? "#3436DC" : action.quiet ? "transparent" : "#DFE4F0"
        border.width: action.activeFocus ? 2 : 1
    }
}
