// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: about
    signal licenseRequested
    signal noticesRequested
    width: 560
    modal: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 16
        color: "white"
        border.color: "#DFE4F0"
    }
    contentItem: ColumnLayout {
        spacing: 0
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            color: "#111832"
            radius: 16
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 16
                color: "#111832"
            }
            Image {
                x: 25
                y: 14
                width: 115
                height: 67
                source: "qrc:/brand/logo.svg"
                fillMode: Image.PreserveAspectFit
            }
            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 26
                spacing: 14
                Image {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    source: "qrc:/brand/synthforge-icon.svg"
                }
                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "SLX SynthForge"
                        color: "white"
                        font.pixelSize: 22
                        font.weight: Font.Bold
                    }
                    Text {
                        text: "Create  /  learn  /  inspect"
                        color: "#B7BEE0"
                        font.pixelSize: 11
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                Text {
                    text: "v" + appVersion
                    color: "#DCE0FF"
                    font.pixelSize: 12
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 3
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0
                        color: "#2948FF"
                    }
                    GradientStop {
                        position: 1
                        color: "#D100FF"
                    }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 26
            spacing: 17
            Text {
                Layout.fillWidth: true
                text: "An industrial vision demonstrator: edit real component geometry, acquire RGB images, inspect the assembly and improve models through measurable experiments."
                color: "#18203B"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            GridLayout {
                columns: 2
                columnSpacing: 24
                rowSpacing: 12
                SectionLabel {
                    text: "AUTHOR"
                }
                Text {
                    text: "Marco Pezzullo · Silicon LogiX"
                    color: "#18203B"
                    font.pixelSize: 12
                }
                SectionLabel {
                    text: "TECHNOLOGY"
                }
                Text {
                    text: "C++17 · Qt Quick · Python / NumPy"
                    color: "#18203B"
                    font.pixelSize: 12
                }
                SectionLabel {
                    text: "CORE"
                }
                Text {
                    text: "SiliconLogiX.SynthForge.Core · ABI 4"
                    color: "#18203B"
                    font.pixelSize: 12
                }
                SectionLabel {
                    text: "COPYRIGHT"
                }
                Text {
                    text: "© 2026 Marco Pezzullo (Silicon LogiX)"
                    color: "#18203B"
                    font.pixelSize: 12
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#DFE4F0"
            }
            Text {
                Layout.fillWidth: true
                text: "Silicon LogiX Evaluation License 1.0. Study and evaluation are permitted; commercial use and distribution of modifications require written authorization."
                color: "#69738C"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                text: "PowerBoard is a visual CAD specimen. Synthetic metrics do not certify a circuit or a production inspection system."
                color: "#69738C"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                ActionButton {
                    text: "License"
                    onClicked: {
                        about.close();
                        about.licenseRequested();
                    }
                }
                ActionButton {
                    text: "Third-party notices"
                    onClicked: {
                        about.close();
                        about.noticesRequested();
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                ActionButton {
                    text: "Website ↗"
                    onClicked: Qt.openUrlExternally("https://www.siliconlogix.it/")
                }
            }
            ActionButton {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                filled: true
                onClicked: about.close()
            }
        }
    }
}
