// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    implicitHeight: 36
    leftPadding: 12
    rightPadding: 28
    font.pixelSize: 11
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: "#18203B"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Text {
        x: control.width - width - 11
        y: (control.height - height) / 2
        text: "▾"
        color: "#69738C"
        font.pixelSize: 14
    }
    background: Rectangle {
        radius: 7
        color: "white"
        border.color: control.activeFocus ? "#7476FF" : "#DFE4F0"
    }
    delegate: ItemDelegate {
        required property var modelData
        required property int index
        width: control.width
        text: String(modelData)
        font: control.font
        highlighted: control.highlightedIndex === index
    }
    popup: Popup {
        y: control.height + 3
        width: control.width
        padding: 4
        implicitHeight: Math.min(contentItem.implicitHeight, 240)
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle {
            radius: 8
            color: "white"
            border.color: "#DFE4F0"
        }
    }
}
