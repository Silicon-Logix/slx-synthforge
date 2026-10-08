# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

"""Compare immutable models on exactly the same held-out acquisition groups."""
import csv
import hashlib
import html
import json
from pathlib import Path
import numpy as np


def predict(model, x, annotations, threshold):
    weights = np.asarray(model["weights"], dtype=np.float32).reshape(x.shape[1], 5)
    normalized = (x - np.asarray(model["mean"], dtype=np.float32)) / np.asarray(model["scale"], dtype=np.float32)
    logits = normalized @ weights + np.asarray(model["bias"], dtype=np.float32)
    p = np.exp(logits - logits.max(axis=1, keepdims=True))
    p /= p.sum(axis=1, keepdims=True)
    domain = {"reflection_gain": [1, 1], "blur_sigma_px": [0, 0], "light_azimuth_deg": [-85, 65], **model.get("acquisition_domain", {})}
    accepted = []
    for probability, annotation in zip(p, annotations):
        camera = annotation["camera"]
        in_domain = all(bounds[0] - 1e-6 <= camera.get(key, 1 if key == "reflection_gain" else 0) <= bounds[1] + 1e-6 for key, bounds in domain.items())
        usable = camera.get("blur_sigma_px", 0) <= 2.5 and annotation.get("quality", {}).get("roi_clipped_fraction", 0) <= .45
        accepted.append(probability.max() >= threshold and annotation["polarity_observable"] and in_domain and usable)
    return p, np.asarray(accepted, dtype=bool)


def inspection_metrics(y, p, accepted):
    predicted = p.argmax(axis=1)
    defect = y != 0
    return {
        "samples": len(y), "defect_samples": int(defect.sum()), "nominal_samples": int((~defect).sum()),
        "bridge_samples": int((y == 4).sum()), "accuracy": float((predicted == y).mean()),
        "missed_defects": int((defect & (predicted == 0)).sum()),
        "false_alarms": int(((~defect) & (predicted != 0)).sum()),
        "missed_bridges": int(((y == 4) & (predicted != 4)).sum()),
        "wrong_defect_type": int((defect & (predicted != 0) & (predicted != y)).sum()),
        "escaped_defects": int((defect & (predicted == 0) & accepted).sum()),
        "accepted_false_alarms": int(((~defect) & (predicted != 0) & accepted).sum()),
        "reacquire": int((~accepted).sum()), "coverage": float(accepted.mean()),
        "accepted_accuracy": float((predicted[accepted] == y[accepted]).mean()) if accepted.any() else None,
    }


def compare_models(before_path, after_path, root, frozen, threshold, reference=None):
    d, samples, x, y, _ = frozen
    annotations = []
    for sample in samples:
        path = (root / sample["annotation"]).resolve()
        if not path.is_relative_to(root.resolve()):
            raise ValueError("Annotation path escapes the benchmark")
        annotations.append(json.loads(path.read_text(encoding="utf-8")))
    models = [json.loads(path.read_text(encoding="utf-8")) for path in (before_path, after_path)]
    scores = [predict(m, x, annotations, threshold) for m in models]
    metrics = [inspection_metrics(y, p, a) for p, a in scores]
    rows = []
    for i, sample in enumerate(samples):
        row = {"sample_index": i, "image": sample["image"], "true_index": int(y[i])}
        for name, (p, accepted) in zip(("before", "after"), scores):
            row[name] = {"predicted_index": int(p[i].argmax()), "confidence": float(p[i].max()), "accepted": bool(accepted[i]), "probabilities": p[i].tolist()}
        rows.append(row)
    out = {
        "schema": "slx.comparison.v1", "benchmark_id": d["id"], "benchmark_path": str(root.resolve()),
        "benchmark_features_sha256": d["features_sha256"],
        "benchmark_manifest_sha256": hashlib.sha256((root / "dataset.json").read_bytes()).hexdigest(),
        "asset_sha256": d["asset_sha256"], "threshold": threshold,
        "before_id": models[0].get("model_id", "legacy-baseline"), "after_id": models[1]["model_id"],
        "before_sha256": hashlib.sha256(before_path.read_bytes()).hexdigest(),
        "after_sha256": hashlib.sha256(after_path.read_bytes()).hexdigest(),
        "before": metrics[0], "after": metrics[1],
        "delta": {key: metrics[1][key] - metrics[0][key] for key in ("missed_defects", "false_alarms", "missed_bridges", "escaped_defects", "reacquire")},
        "predictions": rows,
        "protocol": "Independent frozen stress acquisition groups. Train-only normalization; validation-only epoch selection. Both models use the same threshold. Test samples cannot enter the case queue or augmentation.",
        "definitions": "Missed defects: defect classified as nominal. Missed bridges: bridge assigned any other class. Raw errors include deferred images. Escaped defects and accepted false alarms count accepted decisions only; reacquisition is reported separately.",
        "scope": "Synthetic PowerBoard C1/U4, five fixed single-defect assembly states. Confidence is uncalibrated. These results do not measure performance on real boards.",
    }
    if reference:
        reference_root, rd, rs, rx, ry, splits = reference
        indices = np.flatnonzero(splits == "test")
        annotations = []
        for i in indices:
            path = (reference_root / rs[i]["annotation"]).resolve()
            if not path.is_relative_to(reference_root.resolve()):
                raise ValueError("Annotation path escapes the reference test")
            annotations.append(json.loads(path.read_text(encoding="utf-8")))
        reference_scores = [predict(m, rx[indices], annotations, threshold) for m in models]
        out["reference_test"] = {"dataset_id": rd["id"], "features_sha256": rd["features_sha256"],
                                 "before": inspection_metrics(ry[indices], *reference_scores[0]),
                                 "after": inspection_metrics(ry[indices], *reference_scores[1])}
    csv_path = after_path.parent / "comparison.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["image", "true_class", "before_class", "after_class", "before_confidence", "after_confidence", "before_accepted", "after_accepted"])
        for row in rows:
            writer.writerow([row["image"], row["true_index"], row["before"]["predicted_index"], row["after"]["predicted_index"], row["before"]["confidence"], row["after"]["confidence"], row["before"]["accepted"], row["after"]["accepted"]])
    return out


def write_html(path, report):
    fields = [("Raw missed defects", "missed_defects"), ("Raw false alarms", "false_alarms"),
              ("Bridges not recognized", "missed_bridges"), ("Escaped defects", "escaped_defects"),
              ("Accepted false alarms", "accepted_false_alarms"), ("Reacquisition requests", "reacquire")]
    rows = "".join(f"<tr><td>{label}</td><td>{report['before'][key]}</td><td>{report['after'][key]}</td><td>{report['after'][key]-report['before'][key]:+d}</td></tr>" for label, key in fields)
    escape = html.escape
    reference = report.get("reference_test")
    control = f"<p><strong>Original acquisition test:</strong> {reference['after']['samples']} images. Raw accuracy {reference['before']['accuracy']:.1%} → {reference['after']['accuracy']:.1%}. Missed defects {reference['before']['missed_defects']} → {reference['after']['missed_defects']}. This control exposes regressions in the original acquisition domain.</p>" if reference else ""
    document = f"""<!doctype html><html lang="en"><meta charset="utf-8"><title>SynthForge inspection comparison</title>
<style>body{{font:15px 'Segoe UI',sans-serif;background:#f3f5fb;color:#18203b;margin:0}}main{{max-width:1000px;margin:40px auto;background:white;padding:40px;border-radius:16px}}header{{border-top:5px solid #5557e5;padding-top:20px}}small{{color:#69738c}}h1{{font-size:32px}}table{{width:100%;border-collapse:collapse;margin:30px 0}}td,th{{text-align:left;padding:13px;border-bottom:1px solid #dfe4f0}}code{{font-size:12px;overflow-wrap:anywhere}}@media print{{body{{background:white}}main{{margin:0;padding:20px}}}}</style>
<main><header><small>SILICON LOGIX / INDUSTRIAL VISION</small><h1>SynthForge · Model comparison</h1>
<p>{escape(report['before_id'])} → {escape(report['after_id'])}</p></header>
<p>Frozen benchmark: <strong>{escape(report['benchmark_id'])}</strong><br>
{report['after']['samples']} images · {report['after']['defect_samples']} defective · {report['after']['nominal_samples']} nominal · confidence threshold {report['threshold']:.2f}</p>
<table><tr><th>Inspection measure</th><th>Previous</th><th>Candidate</th><th>Change</th></tr>{rows}</table>
<p>Accepted coverage: {report['before']['coverage']:.1%} → {report['after']['coverage']:.1%}. Raw class accuracy: {report['before']['accuracy']:.1%} → {report['after']['accuracy']:.1%}.</p>
{control}<p>{escape(report['definitions'])}</p><p>{escape(report['protocol'])}</p><p>{escape(report['scope'])}</p>
<small>Benchmark feature SHA-256<br><code>{report['benchmark_features_sha256']}</code><br>Previous model SHA-256<br><code>{report['before_sha256']}</code><br>Candidate model SHA-256<br><code>{report['after_sha256']}</code></small></main></html>"""
    path.write_text(document, encoding="utf-8")
