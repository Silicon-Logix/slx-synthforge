// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: studio
    objectName: "interactiveStudio"
    signal saveRequested
    property bool guided: false
    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: forge.editorMode && forge.canUndo
        onActivated: forge.undoEdit()
    }
    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: forge.editorMode && forge.canRedo
        onActivated: forge.redoEdit()
    }
    FolderDialog {
        id: experimentDialog
        title: "Open SynthForge experiment folder"
        onAccepted: forge.loadExperiment(selectedFolder)
    }
    property var edit: forge.componentEdit
    property var report: forge.assemblyReport
    property var rows: (report.results || []).slice().sort((a, b) => {
        const order = {
            DEFECT: 0,
            REVIEW: 1,
            PASS: 2
        };
        return order[a.decision] - order[b.decision];
    })
    function apply(removed, dx, dy, rotation) {
        forge.editComponent(forge.selectedReference, removed, dx, dy, rotation);
    }
    spacing: 12
    RowLayout {
        Layout.fillWidth: true
        Text {
            text: "ASSEMBLY WORKBENCH"
            color: "#69738C"
            font.pixelSize: 10
            font.letterSpacing: 1
            Layout.fillWidth: true
        }
        ActionButton {
            text: "Undo"
            implicitHeight: 30
            enabled: forge.canUndo
            onClicked: forge.undoEdit()
            ToolTip.text: "Ctrl+Z"
            ToolTip.visible: hovered
        }
        ActionButton {
            text: "Redo"
            implicitHeight: 30
            enabled: forge.canRedo
            onClicked: forge.redoEdit()
        }
        ActionButton {
            text: "Save experiment"
            implicitHeight: 30
            enabled: !!report.results && !forge.inspectionStale && !forge.busy
            onClicked: forge.saveExperiment()
        }
        ActionButton {
            text: "Open"
            implicitHeight: 30
            enabled: !forge.busy && !forge.inspectionActive
            onClicked: experimentDialog.open()
        }
        ActionButton {
            text: "Report"
            implicitHeight: 30
            enabled: !!forge.experimentPath
            onClicked: forge.openExperiment()
        }
        ActionButton {
            text: studio.guided ? "Close guide" : "Guided trial"
            implicitHeight: 30
            onClicked: studio.guided = !studio.guided
        }
    }
    Rectangle {
        visible: studio.guided
        Layout.fillWidth: true
        implicitHeight: guideText.implicitHeight + 24
        color: "#E8F6F3"
        radius: 10
        Text {
            id: guideText
            anchors.fill: parent
            anchors.margins: 12
            color: "#176D62"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            text: !forge.selectedReference ? "1 / Click a component in the 3D view or choose it from the list. Any mounted reference can be selected." : forge.edits.length === 0 ? "2 / Remove, move or rotate the selected component. Use Undo and Redo to explore freely." : !report.results || forge.inspectionStale ? "3 / Start inspection to capture a new image and analyze it with the AI model." : "4 / Click a result to link the image and CAD reference. REVIEW needs verification. Save the experiment, press Stop and try Undo."
        }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 64
        color: "#EBEDFC"
        radius: 10
        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12
            Repeater {
                model: ["Select and edit", "Start inspection", "Inspect the image"]
                delegate: RowLayout {
                    required property string modelData
                    required property int index
                    Layout.fillWidth: true
                    spacing: 9
                    Rectangle {
                        width: 25
                        height: 25
                        radius: 8
                        color: "#3436DC"
                        Text {
                            anchors.centerIn: parent
                            text: index + 1
                            color: "white"
                            font.pixelSize: 12
                        }
                    }
                    Text {
                        text: modelData
                        color: "#303969"
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 14
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            EditorViewport {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 170
            }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: forge.edits.length + " edits · " + forge.components.length + " components"
                    color: "#69738C"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                }
                ActionButton {
                    text: "Top view"
                    implicitHeight: 32
                    onClicked: forge.topCamera()
                }
                ActionButton {
                    text: "Reset view"
                    implicitHeight: 32
                    onClicked: forge.resetCamera()
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: report.results ? (studio.height < 600 ? 164 : 214) : 100
                color: "white"
                radius: 12
                border.color: "#DFE4F0"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 14
                    ColumnLayout {
                        visible: !report.results
                        Layout.fillWidth: true
                        Text {
                            text: "Try a free edit"
                            color: "#18203B"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: "Remove a component or move it, then start inspection. The model analyzes a new RGB image against the nominal board reference."
                            color: "#69738C"
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    ColumnLayout {
                        visible: !!report.results
                        Layout.preferredWidth: 175
                        Layout.fillHeight: true
                        Text {
                            text: forge.inspectionStale ? "PREVIOUS CAPTURE" : "CAPTURED IMAGE"
                            color: forge.inspectionStale ? "#9B691B" : "#69738C"
                            font.pixelSize: 9
                            font.weight: Font.DemiBold
                        }
                        Image {
                            id: capture
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            source: report.results ? "image://frame/capture/" + forge.inspectionVersion : ""
                            fillMode: Image.PreserveAspectFit
                            cache: false
                            Repeater {
                                model: forge.inspectionStale ? [] : studio.rows.filter(r => r.decision === "DEFECT" || r.reference === forge.selectedReference)
                                delegate: Rectangle {
                                    required property var modelData
                                    x: (capture.width - capture.paintedWidth) / 2 + modelData.bbox[0] * capture.paintedWidth / 640
                                    y: (capture.height - capture.paintedHeight) / 2 + modelData.bbox[1] * capture.paintedHeight / 480
                                    width: modelData.bbox[2] * capture.paintedWidth / 640
                                    height: modelData.bbox[3] * capture.paintedHeight / 480
                                    color: "transparent"
                                    border.color: modelData.reference === forge.selectedReference ? "#7ADDD4" : "#FFBE65"
                                    border.width: 1
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: mouse => {
                                    const x = mouse.x - (width - capture.paintedWidth) / 2;
                                    const y = mouse.y - (height - capture.paintedHeight) / 2;
                                    forge.pickCapture(x * 640 / capture.paintedWidth, y * 480 / capture.paintedHeight);
                                }
                            }
                        }
                        Text {
                            text: "Fixed view · 640 × 480"
                            color: "#69738C"
                            font.pixelSize: 9
                        }
                    }
                    ColumnLayout {
                        visible: !!report.results
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: forge.inspectionStale ? "Capture again" : report.defects + " defects · " + report.review + " to review"
                                color: forge.inspectionStale ? "#9B691B" : report.defects ? "#B4484E" : "#167565"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 4
                            model: studio.rows
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view.width
                                height: 38
                                radius: 6
                                color: forge.selectedReference === modelData.reference ? "#ECEEFF" : "#F6F8FC"
                                opacity: forge.inspectionStale ? .55 : 1
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 7
                                    Text {
                                        text: modelData.reference
                                        color: "#18203B"
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                        Layout.preferredWidth: 30
                                    }
                                    Text {
                                        text: modelData.decision === "REVIEW" ? "Review required" : modelData.class_name
                                        color: "#69738C"
                                        font.pixelSize: 10
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: modelData.decision
                                        color: modelData.decision === "DEFECT" ? "#B4484E" : modelData.decision === "REVIEW" ? "#9B691B" : "#167565"
                                        font.pixelSize: 9
                                        font.weight: Font.DemiBold
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: forge.selectReference(modelData.reference)
                                }
                            }
                        }
                        Text {
                            text: report.model_id || ""
                            color: "#8B95AA"
                            font.pixelSize: 9
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
        ColumnLayout {
            Layout.preferredWidth: studio.width < 900 ? 275 : 310
            Layout.minimumWidth: studio.width < 900 ? 275 : 310
            Layout.maximumWidth: studio.width < 900 ? 275 : 310
            Layout.fillHeight: true
            spacing: 10
            Panel {
                Layout.fillWidth: true
                padding: 14
                bodySpacing: 9
                SectionLabel {
                    text: "INSPECTION CONTROL"
                }
                ActionButton {
                    Layout.fillWidth: true
                    filled: true
                    enabled: !forge.busy
                    text: forge.inspectionScanning ? "Capturing image…" : forge.inspectionActive ? "Stop · edit the board" : "Start inspection"
                    onClicked: forge.inspectionActive ? forge.stopInspection() : forge.startInspection()
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: forge.inspectionActive ? "Results belong to the captured board. Press Stop to unlock editing." : "While inspection is stopped, you can remove, move and rotate multiple components."
                    color: "#69738C"
                    font.pixelSize: 11
                }
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                    width: parent.width
                    spacing: 10
                    Panel {
                        Layout.fillWidth: true
                        padding: 14
                        bodySpacing: 10
                        SectionLabel {
                            text: "COMPONENT EDITOR"
                        }
                        ChoiceBox {
                            Layout.fillWidth: true
                            model: forge.components.map(c => c.reference)
                            currentIndex: model.indexOf(forge.selectedReference)
                            displayText: forge.selectedReference || "Click the board or choose here"
                            onActivated: forge.selectReference(currentText)
                        }
                        Text {
                            text: forge.selectedReference ? "CAD reference: " + forge.selectedReference : "Choose one of the 46 components"
                            color: "#69738C"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            enabled: !!forge.selectedReference && !forge.inspectionActive && !forge.busy
                            spacing: 8
                            ActionButton {
                                Layout.fillWidth: true
                                text: studio.edit.removed ? "Reinsert component" : "Remove component"
                                onClicked: studio.apply(!studio.edit.removed, studio.edit.dx_mm || 0, studio.edit.dy_mm || 0, studio.edit.rotation_deg || 0)
                            }
                            ControlSlider {
                                Layout.fillWidth: true
                                title: "X offset"
                                onGestureStarted: forge.beginEdit()
                                onGestureFinished: forge.endEdit()
                                from: -10
                                to: 10
                                suffix: " mm"
                                enabled: !studio.edit.removed
                                value: studio.edit.dx_mm || 0
                                onEdited: nextValue => studio.apply(false, nextValue, studio.edit.dy_mm || 0, studio.edit.rotation_deg || 0)
                            }
                            ControlSlider {
                                Layout.fillWidth: true
                                title: "Y offset"
                                onGestureStarted: forge.beginEdit()
                                onGestureFinished: forge.endEdit()
                                from: -10
                                to: 10
                                suffix: " mm"
                                enabled: !studio.edit.removed
                                value: studio.edit.dy_mm || 0
                                onEdited: nextValue => studio.apply(false, studio.edit.dx_mm || 0, nextValue, studio.edit.rotation_deg || 0)
                            }
                            ControlSlider {
                                Layout.fillWidth: true
                                title: "Rotation"
                                onGestureStarted: forge.beginEdit()
                                onGestureFinished: forge.endEdit()
                                from: -180
                                to: 180
                                suffix: "°"
                                decimals: 0
                                enabled: !studio.edit.removed
                                value: studio.edit.rotation_deg || 0
                                onEdited: nextValue => studio.apply(false, studio.edit.dx_mm || 0, studio.edit.dy_mm || 0, nextValue)
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                ActionButton {
                                    text: "Rotate 180°"
                                    Layout.fillWidth: true
                                    enabled: !studio.edit.removed
                                    onClicked: studio.apply(false, studio.edit.dx_mm || 0, studio.edit.dy_mm || 0, studio.edit.rotation_deg === 180 ? 0 : 180)
                                }
                                ActionButton {
                                    text: "Restore"
                                    Layout.fillWidth: true
                                    onClicked: forge.restoreComponent(forge.selectedReference)
                                }
                            }
                        }
                    }
                    Panel {
                        Layout.fillWidth: true
                        padding: 14
                        bodySpacing: 8
                        SectionLabel {
                            text: "BOARD EDITS"
                        }
                        Text {
                            visible: forge.edits.length === 0
                            text: "Nominal assembly"
                            color: "#167565"
                            font.pixelSize: 11
                        }
                        Repeater {
                            model: forge.edits
                            delegate: Text {
                                required property var modelData
                                text: modelData.reference + " · " + (modelData.removed ? "removed" : "X " + modelData.dx_mm.toFixed(1) + " / Y " + modelData.dy_mm.toFixed(1) + " mm / " + modelData.rotation_deg.toFixed(0) + "°")
                                color: "#69738C"
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                        ActionButton {
                            text: "Restore entire board"
                            Layout.fillWidth: true
                            enabled: !forge.inspectionActive && !forge.busy && forge.edits.length > 0
                            onClicked: forge.restoreAssembly()
                        }
                    }
                    Panel {
                        visible: !!report.results && !!forge.selectedReference
                        Layout.fillWidth: true
                        padding: 14
                        bodySpacing: 8
                        SectionLabel {
                            text: "CAPTURE DETAIL"
                        }
                        Image {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 95
                            source: report.results && forge.selectedReference ? "image://frame/capture-detail/" + forge.inspectionVersion : ""
                            cache: false
                            fillMode: Image.PreserveAspectFit
                        }
                        Text {
                            property var result: studio.rows.find(r => r.reference === forge.selectedReference) || ({})
                            text: forge.inspectionStale ? "The board has changed: capture a new image." : result.note ? result.note + " · score " + (result.confidence * 100).toFixed(1) + "%" : ""
                            color: "#69738C"
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: "Recognition uses RGB pixels. Symmetric rotations and hidden parts need verification; scores are uncalibrated."
                        color: "#8B95AA"
                        font.pixelSize: 10
                    }
                    ActionButton {
                        text: "Confirm case for AI improvement"
                        enabled: !!report.results && !forge.inspectionStale && !!forge.inspectorCycle.benchmark && !forge.busy
                        onClicked: forge.collectInspectorCase()
                    }
                    ActionButton {
                        text: "Open dataset scenarios"
                        quiet: true
                        Layout.fillWidth: true
                        onClicked: forge.enterEditor(false)
                    }
                }
            }
        }
    }
}
