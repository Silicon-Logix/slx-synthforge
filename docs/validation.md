<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Validation

Environment: Windows x64, Qt 6.12.0, MinGW GCC 13.1.0, Python 3.13 and NumPy.
Automated checks cover geometry/annotation contracts, all 46 individual removals,
unseen edited poses, acquisition gates, edit locking, undo/history, portable
experiments, file tampering and both local training workflows. Release 0.4.1 adds
resolution gating, workspace/model restoration and compatibility with historical
report labels while still rejecting falsified numeric results.

The included assembly model was trained on 2,150 observable synthetic component
examples: 1,252 training, 358 validation and 540 test. Test accuracy is 91.48%;
coverage at score 0.80 is 78.15%, with no escaped defects or false alarms among
accepted decisions on this test. These measurements do not cover arbitrary edits
or real photographs. Scores are uncalibrated.

All 46 individual removals produce 45 DEFECT, one REVIEW and zero PASS at the
standard capture. Removing C2 and moving U4 by +3.7 mm / −1.1 mm with 23° rotation
produces two defects, 44 passes and no reviews.

The completed Assembly AI lab example evaluates 1,076 independent examples
(800 defects, 276 nominal). Baseline and candidate both have 92.286% raw accuracy,
one raw missed defect and zero raw false alarms. REVIEW decreases from 250 to
249. There are no accepted errors on that test. The original 540-example control
test remains at 91.481% accuracy and 118 reviews. This cycle does not demonstrate
a reduction in missed defects.

The earlier C1/U4 improvement run increased raw accuracy from 94% to 98% on a
100-image frozen challenge test; raw misses fell from six to two and raw false
alarms stayed at zero. Reviews fell from 100 to four, exposing two accepted
escaped defects. The report shows that trade-off; higher accuracy alone is
insufficient to promote a candidate. Its original control accuracy fell from
100% to 98.33%.

The public checkout is built using the binary SDK alone. Export checks cover
file hashes, confidential material, credentials, personal paths, binary vendor
metadata and stripped symbols. The portable build is exercised with the Qt
kit removed from PATH. Corresponding runtime sources accompany the distribution.
