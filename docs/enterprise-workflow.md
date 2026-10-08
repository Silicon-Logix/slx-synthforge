<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Inspection and improvement workflow

## Acquisition robustness

Robustness lab renders actual variations in lighting, reflection, blur and
camera tilt. Each image is inspected, and the interface distinguishes accepted
defect/nominal decisions from review/reacquisition. The grid shows where the
model remains usable and where acquisition conditions exceed its profile.
This also explains why randomized synthetic data helps diversify training.

## Development cases and a frozen test

A confirmed development case records an observed result and its synthetic edit
annotation. The annotation is for preparing new training examples and auditing;
it is not passed to recognition. Duplicate cases are rejected. New cases
invalidate earlier prepared augmentation so that the next candidate uses fresh data.

Assembly AI lab freezes an independent benchmark and the baseline bytes before
collecting cases. Additional examples contain training/validation only. Both
versions are evaluated on the identical benchmark hash and threshold, with a
separate original control test. Previous models are retained and a candidate
is activated explicitly.

## Reading outcomes

| Metric | Meaning |
| --- | --- |
| Raw missed defects | Actual defects whose highest-scoring class is nominal, including reviewed decisions |
| Raw false alarms | Nominal examples whose highest-scoring class is defective |
| Escaped defects | Accepted PASS decisions on actual defects |
| Accepted false alarms | Accepted DEFECT decisions on nominal examples |
| REVIEW | Cases held for verification rather than accepted |
| Coverage | Fraction of examples with an accepted decision |

Lower reviews can expose more accepted mistakes. Compare denominators, coverage,
misses and false alarms together. Reports include per-reference outcomes and the
original control test. HTML, CSV and JSON support presentation and further analysis.
Scores are uncalibrated and synthetic results do not establish factory performance.

## Traceable experiments

A portable experiment links a captured image, nominal reference, CAD edits,
numeric model, version ID and verified results. Hashes record file integrity.
The selected defect links back to the 3D component. No customer equipment is
needed to show acquisition, inspection, review and controlled model improvement.
