<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Editable assembly

All 46 mounted references can be removed, translated within ±10 mm or rotated
within ±180°. Rotation uses the component's CAD pivot. Several components can be
changed in one scene. Restore component/board and a bounded 100-step Undo/Redo
history support repeatable trials; a slider gesture is one history action.

Inspection follows **Stop → Edit → Start inspection → Review results**. Editing
is locked while inspection is running. Camera drag affects the 3D preview;
recognition acquires an independent 640 × 480 image at its fixed camera.

The included three-class model distinguishes nominal appearance, missing
components and changed position/appearance. It analyzes captured RGB rather than
reading edit commands. Results link to the original CAD reference, captured crop,
model ID and score. Changing the scene or model marks earlier results stale.

A rotation that produces the same visible shape cannot be reliably detected.
Occlusion, limited visibility, low scores and unsupported captures require REVIEW.
The five-scenario C1/U4 model handles its separate polarity and bridge examples.

Save experiment stores nominal/captured images, edits, the exact model and an
HTML report. Opening an experiment verifies all required checksums and recomputes
numeric results before applying edits. Historical label text is refreshed in
English. Checksums detect accidental corruption; they are not signatures.
