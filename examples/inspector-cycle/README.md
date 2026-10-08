<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Completed inspection cycle

Open Assembly AI lab and select **Show example comparison**. The active model
is retained until a version is selected explicitly.

The frozen benchmark contains 1,076 examples: 800 defects and 276 nominal.
Both versions have 92.286% raw accuracy, one raw missed defect and zero raw false
alarms. REVIEW decreases from 250 to 249. Accepted decisions contain no escaped
defects or false alarms. The original control test remains at 91.481% accuracy
and 118 reviews. This run does not demonstrate a decrease in missed defects.

The development case is U4 translated X 1.7 / Y 0.4 mm and rotated 17°. It is
separate from the test and generates nearby training/validation examples only.
Baseline and candidate retain immutable weights, reports and hashes. The
experiment folder includes captured pixels and audit metadata that the editor
can reopen. Paths are relative to make the example portable.

Original Silicon LogiX evaluation data. See the repository LICENSE.
