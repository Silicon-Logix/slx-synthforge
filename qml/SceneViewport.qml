// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick

Rectangle {
    id: viewport
    property bool showRegions: true
    property bool showInstances: false
    property bool showEvidence: false
    color: "#111832"
    radius: 12
    clip: true
    Image {
        id: board
        anchors.fill: parent
        anchors.margins: 12
        source: "image://frame/live/" + forge.frameVersion
        cache: false
        fillMode: Image.PreserveAspectFit
        Item {
            id: projection
            x: (board.width - board.paintedWidth) / 2
            y: (board.height - board.paintedHeight) / 2
            width: board.paintedWidth
            height: board.paintedHeight
            property real sx: width / (forge.annotations.size ? forge.annotations.size[0] : 960)
            property real sy: height / (forge.annotations.size ? forge.annotations.size[1] : 720)
            Image {
                anchors.fill: parent
                visible: viewport.showEvidence && !!forge.modelPath
                source: viewport.showEvidence && forge.modelPath ? "image://frame/evidence/" + forge.frameVersion : ""
                cache: false
            }
            Repeater {
                model: viewport.showInstances ? (forge.annotations.instances || []) : viewport.showRegions ? (forge.annotations.inspection_regions || []) : []
                delegate: Rectangle {
                    required property var modelData
                    x: modelData.bbox[0] * projection.sx
                    y: modelData.bbox[1] * projection.sy
                    width: modelData.bbox[2] * projection.sx
                    height: modelData.bbox[3] * projection.sy
                    color: "transparent"
                    border.color: "#7ADDD4"
                    radius: 2
                    Rectangle {
                        y: -18
                        width: reference.implicitWidth + 12
                        height: 18
                        radius: 3
                        color: "#167565"
                        Text {
                            id: reference
                            anchors.centerIn: parent
                            text: modelData.reference
                            font.pixelSize: 10
                            color: "white"
                        }
                    }
                }
            }
            Repeater {
                model: viewport.showRegions ? (forge.annotations.defects || []) : []
                delegate: Rectangle {
                    required property var modelData
                    x: modelData.bbox[0] * projection.sx
                    y: modelData.bbox[1] * projection.sy
                    width: Math.max(4, modelData.bbox[2] * projection.sx)
                    height: Math.max(4, modelData.bbox[3] * projection.sy)
                    color: "#20FFBE65"
                    border.color: "#FFBE65"
                    border.width: 2
                    radius: 2
                }
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
                if (Math.abs(mouse.x - startX) + Math.abs(mouse.y - startY) > 6)
                    orbiting = true;
                if (orbiting) {
                    forge.azimuth += (mouse.x - lastX) * .35;
                    forge.tilt += (mouse.y - lastY) * .2;
                }
                lastX = mouse.x;
                lastY = mouse.y;
            }
            onReleased: mouse => {
                forge.endOrbit();
                if (!orbiting && projection.sx > 0 && projection.sy > 0)
                    forge.pick((mouse.x - projection.x) / projection.sx, (mouse.y - projection.y) / projection.sy);
            }
            onCanceled: forge.endOrbit()
            onWheel: wheel => forge.span -= wheel.angleDelta.y / 40
            onDoubleClicked: forge.resetCamera()
        }
    }
    Row {
        x: 22
        y: 20
        spacing: 8
        Rectangle {
            width: 7
            height: 7
            radius: 4
            color: "#6CDEC3"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "POWERBOARD / LINKED CAD"
            color: "#B9C3DD"
            font.pixelSize: 10
            font.letterSpacing: 1
        }
    }
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18
        width: truth.implicitWidth + 22
        height: 27
        radius: 5
        color: "#273453"
        Text {
            id: truth
            anchors.centerIn: parent
            text: "SCENE: " + (forge.labels[forge.annotations.class_index] || "USER ASSEMBLY").toUpperCase()
            color: "#DBE4FA"
            font.pixelSize: 9
        }
    }
    Text {
        x: 22
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        text: forge.rendering ? "Acquiring…" : viewport.showEvidence ? "MODEL EVIDENCE / relative pixel contributions" : "Click to inspect · Drag to orbit · Scroll to zoom"
        color: "#91A1BC"
        font.pixelSize: 10
    }
}
