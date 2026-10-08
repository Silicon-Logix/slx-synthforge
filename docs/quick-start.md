<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Quick start

## Editable-board inspection

1. Open **Scene studio** and select a component in the 3D view or reference list.
2. Click **Remove**, or change its X/Y offsets and rotation. Multiple edits are supported.
3. Click **Start inspection**. The app acquires a repeatable 640 × 480 RGB image,
   locks editing and runs the included model on its pixels.
4. Select a result to connect its captured detail, decision, score and model ID
   with the corresponding CAD reference. **PASS**, **DEFECT** and **REVIEW** are distinct outcomes.
5. Click **Stop** to change the board again. **Restore entire board**, Undo/Redo and
   Ctrl+Z/Ctrl+Y let you repeat the demonstration.
6. **Save experiment** writes the captured images, model, edits and report into
   a new experiment folder. **Report** displays its HTML report; **Open**
   restores an experiment after verifying its files and inference results.

Orbit and zoom affect the preview. Recognition uses its fixed inspection
camera, so rotating the preview does not silently change inspection conditions.
**Guided trial** follows your actions without inserting a defect automatically.

## Training the editable-board inspector

Install Python and `python -m pip install -r tools/requirements.txt`, then select
that executable in **Dataset forge**. In **Assembly AI lab**, the included inspector
data is available for training. **Show example comparison** loads a completed run
and retains the active model until you choose a version.

To run a new improvement cycle: freeze an independent test, inspect an edited
board, select a result and confirm it as a development case. Stop inspection,
generate additional examples in Assembly AI lab and train a candidate. Compare
missed defects, false alarms, review count, coverage and the original control
test before choosing the baseline or candidate. Improvement is not guaranteed.
**Generate inspector dataset** starts a new cycle while preserving earlier files.

## Five-scenario laboratory

The additional laboratories use a separate C1/U4 model for nominal, missing C1,
misaligned U4, reversed C1 and a U4 solder bridge. Generate or import a dataset
in Dataset forge, train in Model lab and inspect held-out results in Evaluation.
Robustness lab varies illumination, reflections, blur and camera tilt using
real rendered images. Unsupported or uncertain captures request review/reacquisition.
Improvement loop collects development failures, augments training/validation and
compares two versions on the same frozen challenge test.

## Running and troubleshooting

The portable application needs no Qt installation and no Python for inspection.
Choose a writable workspace to retain local outputs. The public source build
requires the exact kit described in the README. A different Qt/compiler ABI
requires a matching approved core binary.

Use `--software-ui` if OpenGL 3.3 is unavailable. Dragging uses a lower-resolution
software preview and finishes with a full render. Use `--diagnostics path.log`
when collecting a reproducible startup issue. Reports and checksums detect
accidental changes; they are not digital signatures.
