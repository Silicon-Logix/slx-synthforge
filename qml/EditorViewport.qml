// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import SiliconLogiX.SynthForge 1.0

Rectangle {
    id: viewport
    color: "#111832"
    radius: 12
    clip: true
    Item {
        id: projection
        anchors.centerIn: parent
        width: Math.min(parent.width - 24, (parent.height - 24) * 4 / 3)
        height: width * 3 / 4
        Loader {
            anchors.fill: parent
            sourceComponent: forge.acceleratedPreview ? hardware : software
        }
        Component {
            id: hardware
            AssemblyView {
                source: forge
            }
        }
        Component {
            id: software
            Image {
                source: "image://frame/live/" + forge.frameVersion
                cache: false
                fillMode: Image.Stretch
            }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            property real lastX
            property real lastY
            property real startX
            property real startY
            property bool orbiting: false
            onPressed: mouse => {
                lastX = startX = mouse.x;
                lastY = startY = mouse.y;
                orbiting = false;
                forge.beginOrbit();
            }
            onPositionChanged: mouse => {
                if (!pressed)
                    return;
                if (Math.abs(mouse.x - startX) + Math.abs(mouse.y - startY) > 5)
                    orbiting = true;
                if (orbiting) {
                    forge.azimuth += (mouse.x - lastX) * .4;
                    forge.tilt += (mouse.y - lastY) * .22;
                }
                lastX = mouse.x;
                lastY = mouse.y;
            }
            onReleased: mouse => {
                forge.endOrbit();
                if (!orbiting)
                    forge.pickPreview(mouse.x * 960 / width, mouse.y * 720 / height);
            }
            onCanceled: forge.endOrbit()
            onWheel: wheel => forge.span -= wheel.angleDelta.y / 40
            onDoubleClicked: forge.resetCamera()
        }
    }
    Text {
        x: 20
        y: 18
        text: "POWERBOARD / " + (forge.acceleratedPreview ? "GPU PREVIEW" : "SOFTWARE PREVIEW")
        color: "#B9C3DD"
        font.pixelSize: 10
        font.letterSpacing: 1
    }
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 14
        width: stateLabel.implicitWidth + 24
        height: 30
        radius: 6
        color: forge.inspectionActive ? "#17473F" : "#293457"
        Text {
            id: stateLabel
            anchors.centerIn: parent
            text: forge.inspectionActive ? "INSPECTING / EDITS LOCKED" : "STOPPED / EDITABLE BOARD"
            color: forge.inspectionActive ? "#82E2CD" : "#E0E6FA"
            font.pixelSize: 9
            font.weight: Font.DemiBold
        }
    }
    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
        text: forge.selectedReference ? "SELECTED: " + forge.selectedReference + "  ·  Drag to orbit" : "Click a component · Drag to orbit · Scroll to zoom"
        color: "#A6B7D6"
        font.pixelSize: 10
    }
}
