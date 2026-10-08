// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

// Branded workspace navigation, local artifact dialogs and job status.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1440
    height: 940
    minimumWidth: 1120
    minimumHeight: 740
    visible: true
    title: "SLX SynthForge · Silicon LogiX"
    color: "#F3F5FB"
    property int currentTab: 0
    property var titles: ["Scene studio", "Dataset forge", "Model lab", "Evaluation", "Robustness lab", "Improvement loop", "Assembly AI lab"]
    property var subtitles: ["Edit the board, capture an image and inspect the anomalies.", "Turn controlled 3D variations into traceable training data.", "Train the C1/U4 dataset model. The board editor uses a separate reference inspector.", "Understand predictions, uncertainty and inspection errors.", "Find the acquisition limits. Turn optical stress into inspection evidence.", "Collect useful failures and compare model versions on a frozen challenge.", "Improve the editable-board inspector on an independent test."]
    function showAbout() {
        about.open();
    }
    onClosing: close => {
        if (forge.busy) {
            close.accepted = false;
            closeDialog.open();
        }
    }
    Theme {
        id: theme
    }
    header: Rectangle {
        height: 82
        color: "white"
        Rectangle {
            width: parent.width
            height: 4
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
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: theme.lineColor
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 26
            anchors.rightMargin: 26
            anchors.topMargin: 4
            spacing: 15
            Rectangle {
                Layout.preferredWidth: 96
                Layout.preferredHeight: 62
                color: theme.navy
                radius: 7
                Image {
                    anchors.fill: parent
                    anchors.margins: 3
                    source: "qrc:/brand/logo.svg"
                    fillMode: Image.PreserveAspectFit
                }
            }
            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 36
                color: theme.lineColor
            }
            Image {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                source: "qrc:/brand/synthforge-icon.svg"
            }
            ColumnLayout {
                spacing: 2
                Text {
                    text: "SynthForge"
                    color: theme.ink
                    font.pixelSize: 20
                    font.weight: Font.Bold
                }
                Text {
                    text: "SYNTHETIC VISION / INDUSTRIAL AI"
                    color: theme.muted
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.1
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Rectangle {
                Layout.preferredWidth: 124
                Layout.preferredHeight: 27
                color: "#EDF6F3"
                radius: 14
                Text {
                    anchors.centerIn: parent
                    text: "●  LOCAL WORKSPACE"
                    color: theme.success
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                }
            }
            ActionButton {
                text: "Data folder"
                quiet: true
                onClicked: forge.openWorkspace()
            }
            ActionButton {
                text: "About"
                onClicked: about.open()
            }
        }
    }
    footer: Rectangle {
        height: forge.busy ? 62 : 38
        color: "white"
        Rectangle {
            width: parent.width
            height: 1
            color: theme.lineColor
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 18
            Rectangle {
                width: 7
                height: 7
                radius: 4
                color: forge.error ? theme.failure : forge.busy ? theme.accent : theme.success
            }
            Text {
                text: forge.status
                color: theme.muted
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            ProgressBar {
                visible: forge.busy
                Layout.preferredWidth: 180
                value: forge.progress
            }
            Text {
                visible: forge.busy
                text: Math.round(forge.progress * 100) + "%"
                color: theme.accent
                font.pixelSize: 11
            }
            ActionButton {
                visible: forge.busy
                text: "Cancel"
                onClicked: forge.cancel()
            }
            Text {
                visible: !forge.busy
                text: "SLX POWERBOARD  ·  SYNTHETIC DEMO  ·  v" + appVersion
                color: theme.muted
                font.pixelSize: 9
                font.letterSpacing: .6
            }
        }
    }
    RowLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            Layout.preferredWidth: 188
            Layout.fillHeight: true
            color: "#F9FAFD"
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: theme.lineColor
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 9
                SectionLabel {
                    text: "WORKSPACE"
                    Layout.topMargin: 10
                    Layout.bottomMargin: 15
                }
                Repeater {
                    model: window.titles
                    delegate: Button {
                        id: nav
                        enabled: !forge.inspectionScanning
                        required property string modelData
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: 50
                        onClicked: {
                            forge.enterEditor(index === 0);
                            window.currentTab = index;
                        }
                        contentItem: RowLayout {
                            spacing: 11
                            Rectangle {
                                width: 23
                                height: 23
                                radius: 6
                                color: window.currentTab === nav.index ? "#3436DC" : "#E9ECF5"
                                Text {
                                    anchors.centerIn: parent
                                    text: String(nav.index + 1).padStart(2, "0")
                                    color: window.currentTab === nav.index ? "white" : "#69738C"
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                }
                            }
                            Text {
                                text: nav.modelData
                                color: window.currentTab === nav.index ? theme.accent : theme.ink
                                font.pixelSize: 12
                                font.weight: window.currentTab === nav.index ? Font.DemiBold : Font.Normal
                                Layout.fillWidth: true
                            }
                        }
                        background: Rectangle {
                            radius: 8
                            color: window.currentTab === nav.index ? "#ECEEFF" : nav.hovered ? "#F0F2F9" : "transparent"
                            border.color: nav.activeFocus ? "#7476FF" : "transparent"
                        }
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.lineColor
                }
                SectionLabel {
                    text: "REFERENCE ASSET"
                    Layout.topMargin: 9
                }
                Text {
                    text: "SLX PowerBoard"
                    color: theme.ink
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "46 components\nEditable assembly\nOriginal CAD geometry"
                    color: theme.muted
                    font.pixelSize: 10
                    lineHeight: 1.5
                }
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    text: "From virtual assembly\nto visual inspection."
                    color: "#8791AA"
                    font.pixelSize: 10
                    lineHeight: 1.4
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            spacing: 18
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 6
                    Text {
                        text: window.titles[window.currentTab]
                        color: theme.ink
                        font.pixelSize: 27
                        font.weight: Font.DemiBold
                        font.letterSpacing: -.5
                    }
                    Text {
                        Layout.fillWidth: true
                        text: window.subtitles[window.currentTab]
                        color: theme.muted
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                Text {
                    text: ["01 / SCENE", "02 / DATA", "03 / LEARN", "04 / EVIDENCE", "05 / STRESS", "06 / IMPROVE", "07 / ASSEMBLY AI"][window.currentTab]
                    color: "#929CB3"
                    font.pixelSize: 10
                    font.letterSpacing: 1
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: errorText.implicitHeight + 26
                visible: !!forge.error
                color: "#FCEFF0"
                radius: 8
                border.color: "#E9C7CB"
                Text {
                    id: errorText
                    anchors.left: parent.left
                    anchors.right: clear.left
                    anchors.margins: 13
                    anchors.verticalCenter: parent.verticalCenter
                    text: forge.error
                    color: theme.failure
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
                ActionButton {
                    id: clear
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Dismiss"
                    quiet: true
                    onClicked: forge.clearError()
                }
            }
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: window.currentTab
                StudioView {
                    onSaveRequested: saveDialog.open()
                }
                DatasetView {
                    onImportRequested: datasetDialog.open()
                    onWorkspaceRequested: workspaceDialog.open()
                    onTrainRequested: window.currentTab = 2
                }
                ModelView {
                    onImportRequested: modelDialog.open()
                    onEvaluationRequested: window.currentTab = 3
                }
                EvaluationView {
                    onInspectRequested: sampleIndex => {
                        forge.selectSample(sampleIndex);
                        window.currentTab = 0;
                    }
                }
                RobustnessView {
                    onCycleRequested: window.currentTab = 5
                }
                ImprovementView {
                    onProbeRequested: probeIndex => {
                        forge.inspectProbe(probeIndex);
                        window.currentTab = 4;
                    }
                    onInspectRequested: caseIndex => {
                        forge.inspectCase(caseIndex);
                        window.currentTab = 4;
                    }
                }
                InspectorLab {
                    onEditorRequested: {
                        forge.enterEditor(true);
                        window.currentTab = 0;
                    }
                }
            }
        }
    }
    FolderDialog {
        id: datasetDialog
        title: "Open SynthForge dataset folder"
        onAccepted: forge.loadDataset(selectedFolder)
    }
    FolderDialog {
        id: workspaceDialog
        title: "Choose dataset output folder"
        onAccepted: forge.chooseWorkspace(selectedFolder)
    }

    FileDialog {
        id: modelDialog
        title: "Open SynthForge visual model"
        nameFilters: ["SynthForge model (*.slxmodel.json)"]
        onAccepted: forge.loadModel(selectedFile)
    }
    FileDialog {
        id: saveDialog
        title: "Save inspection image"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        nameFilters: ["PNG image (*.png)"]
        onAccepted: forge.saveFrame(selectedFile)
    }
    AboutDialog {
        id: about
        anchors.centerIn: parent
        onLicenseRequested: {
            legal.title = "Evaluation license";
            legal.document = licenseText;
            legal.open();
        }
        onNoticesRequested: {
            legal.title = "Third-party components";
            legal.document = noticesText + "\n\n" + qtLicenseText;
            legal.open();
        }
    }
    Dialog {
        id: legal
        property string document
        anchors.centerIn: parent
        width: Math.min(780, window.width - 100)
        height: Math.min(690, window.height - 120)
        modal: true
        standardButtons: Dialog.Close
        contentItem: ScrollView {
            clip: true
            TextArea {
                text: legal.document
                readOnly: true
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: theme.ink
                selectByMouse: true
            }
        }
    }
    Dialog {
        id: closeDialog
        anchors.centerIn: parent
        title: "A job is running"
        modal: true
        standardButtons: Dialog.Ok
        Label {
            text: "Cancel the job before closing SynthForge."
        }
    }
}
