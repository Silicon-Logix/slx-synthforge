// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: studio
    signal saveRequested
    ActionButton {
        visible: !forge.editorMode
        text: "Try the editable board"
        filled: true
        onClicked: forge.enterEditor()
    }
    Loader {
        id: content
        Layout.fillWidth: true
        Layout.fillHeight: true
        sourceComponent: forge.editorMode ? editor : legacy
    }
    Component {
        id: editor
        InteractiveStudio {
            onSaveRequested: studio.saveRequested()
        }
    }
    Component {
        id: legacy
        LegacyStudioView {
            onSaveRequested: studio.saveRequested()
        }
    }
}
