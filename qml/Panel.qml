// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: panel
    default property alias body: content.data
    property int padding: 22
    property int bodySpacing: 14
    color: "white"
    radius: 12
    border.color: "#DFE4F0"
    implicitHeight: content.implicitHeight + padding * 2
    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: panel.padding
        spacing: panel.bodySpacing
    }
}
