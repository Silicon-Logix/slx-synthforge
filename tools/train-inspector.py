# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

"""Train the editable-board RGB reference classifier; never consume edit commands."""
import argparse
import hashlib
import json
from pathlib import Path
import uuid
import csv
import html
import os
import sys
import numpy as np


def emit(event, **fields):
    print(json.dumps({"event": event, **fields}), flush=True)


def load_data(directory):
    if (directory / "incomplete.json").exists():
        raise ValueError("Dataset is incomplete")
    raw = (directory / "dataset.json").read_bytes()
    manifest = json.loads(raw)
    if manifest["schema"] != "slx.inspector-data.v1" or manifest["feature_count"] != 16:
        raise ValueError("Unsupported inspector dataset")
    binary = (directory / "features.f32").read_bytes()
    if hashlib.sha256(binary).hexdigest() != manifest["features_sha256"]:
        raise ValueError("Dataset checksum mismatch")
    x = np.frombuffer(binary, dtype="<f4").reshape(-1, 16).astype(np.float64)
    rows = manifest["samples"]
    if not rows or len(rows) != len(x) or not np.isfinite(x).all():
        raise ValueError("Invalid inspector samples")
    if any(r["class_index"] not in (0, 1, 2) or r["split"] not in ("train", "validation", "test") for r in rows):
        raise ValueError("Invalid labels or split")
    groups = {name: {r["capture_group"] for r in rows if r["split"] == name}
              for name in ("train", "validation", "test")}
    if any(groups[a] & groups[b] for a, b in (("train", "validation"), ("train", "test"), ("validation", "test"))):
        raise ValueError("Capture group leakage")
    return manifest, x, rows, hashlib.sha256(raw + binary).hexdigest()


def score(model, x, rows, threshold):
    mean, scale = np.asarray(model["mean"], dtype=float), np.asarray(model["scale"], dtype=float)
    weights, b = np.asarray(model["weights"], dtype=float), np.asarray(model["bias"], dtype=float)
    if (model["schema"] != "slx.assembly-inspector.v1" or model.get("feature_count") != 16 or
            mean.shape != (16,) or scale.shape != (16,) or weights.shape != (48,) or b.shape != (3,) or
            any(not np.isfinite(a).all() or np.any(np.abs(a) > 1e6) for a in (mean, scale, weights, b)) or
            np.any(scale <= 0) or not len(rows) or len(rows) != len(x)):
        raise ValueError("Invalid baseline model")
    w = weights.reshape(16, 3)
    logits = (x - mean) / scale @ w + b
    p = np.exp(logits - logits.max(axis=1, keepdims=True))
    p /= p.sum(axis=1, keepdims=True)
    predicted, accepted = p.argmax(axis=1), p.max(axis=1) >= threshold
    actual = np.asarray([r["class_index"] for r in rows])
    matrix = np.zeros((3, 3), dtype=int)
    np.add.at(matrix, (actual, predicted), 1)
    return {"samples": len(rows), "actual_defects": int((actual != 0).sum()),
            "actual_nominal": int((actual == 0).sum()),
            "accuracy": float((actual == predicted).mean()), "coverage": float(accepted.mean()),
            "missed_defects_raw": int(((actual != 0) & (predicted == 0)).sum()),
            "false_alarms_raw": int(((actual == 0) & (predicted != 0)).sum()),
            "escaped_defects": int(((actual != 0) & (predicted == 0) & accepted).sum()),
            "false_alarms": int(((actual == 0) & (predicted != 0) & accepted).sum()),
            "review": int((~accepted).sum()), "confusion": matrix.tolist()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--epochs", type=int, default=2000)
    parser.add_argument("--augment", type=Path)
    parser.add_argument("--benchmark", type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--benchmark-hash")
    parser.add_argument("--threshold", type=float, default=.80)
    args = parser.parse_args()
    if args.output.exists() or not 50 <= args.epochs <= 10000 or not 0 <= args.threshold <= 1:
        raise ValueError("Output exists or invalid training parameters")
    manifest, x, rows, data_hash = load_data(args.dataset)
    sources = [{"role": "base", "sha256": data_hash, "seed": manifest["seed"]}]
    if args.augment:
        aug, ax, ar, ah = load_data(args.augment)
        if aug.get("purpose") != "augmentation" or any(r["split"] == "test" for r in ar):
            raise ValueError("Augmentation must contain train/validation only")
        if aug["asset_sha256"] != manifest["asset_sha256"] or aug["seed"] == manifest["seed"]:
            raise ValueError("Augmentation must match assets and use an independent seed")
        sources.append({"role": "augmentation", "sha256": ah, "seed": aug["seed"]})
        x, rows = np.concatenate((x, ax)), rows + ar
    benchmark = None
    if args.benchmark:
        bm, bx, br, bh = load_data(args.benchmark)
        if bh != args.benchmark_hash or bm.get("purpose") != "benchmark" or any(r["split"] != "test" for r in br):
            raise ValueError("Frozen benchmark has changed or is not test-only")
        if bm["asset_sha256"] != manifest["asset_sha256"] or bm["seed"] in {m["seed"] for m in sources}:
            raise ValueError("Benchmark is not independent of training")
        baseline_bytes = args.baseline.read_bytes()
        baseline = json.loads(baseline_bytes)
        if baseline["asset_sha256"] != manifest["asset_sha256"]:
            raise ValueError("Baseline assets mismatch")
        benchmark = bm, bx, br, bh, baseline, hashlib.sha256(baseline_bytes).hexdigest()
    elif args.augment:
        raise ValueError("Freeze a benchmark and baseline before improvement")
    y = np.array([r["class_index"] for r in rows])
    indices = {name: np.array([i for i, r in enumerate(rows) if r["split"] == name], dtype=int)
               for name in ("train", "validation", "test")}
    if not all(len(ii) and set(y[ii]) == {0, 1, 2} for ii in indices.values()):
        raise ValueError("All base splits must contain three classes")
    mean = x[indices["train"]].mean(axis=0)
    scale = np.maximum(.04, x[indices["train"]].std(axis=0))
    z = (x - mean) / scale
    rng = np.random.default_rng(7831)
    w = rng.normal(0, .015, (x.shape[1], 3))
    b = np.zeros(3)
    mw, vw, mb, vb = np.zeros_like(w), np.zeros_like(w), np.zeros_like(b), np.zeros_like(b)
    best, best_loss = None, float("inf")

    def probabilities(ii):
        a = z[ii] @ w + b
        a = np.exp(a - a.max(axis=1, keepdims=True))
        return a / a.sum(axis=1, keepdims=True)

    emit("started", samples=len(rows))
    for epoch in range(1, args.epochs + 1):
        ii = indices["train"]
        p = probabilities(ii)
        p[np.arange(len(ii)), y[ii]] -= 1
        gw = z[ii].T @ p / len(ii) + .0003 * w
        gb = p.mean(axis=0)
        mw, vw = .9 * mw + .1 * gw, .999 * vw + .001 * gw * gw
        mb, vb = .9 * mb + .1 * gb, .999 * vb + .001 * gb * gb
        w -= .015 * (mw / (1 - .9 ** epoch)) / (np.sqrt(vw / (1 - .999 ** epoch)) + 1e-8)
        b -= .015 * (mb / (1 - .9 ** epoch)) / (np.sqrt(vb / (1 - .999 ** epoch)) + 1e-8)
        ii = indices["validation"]
        p = probabilities(ii)
        loss = -np.log(np.maximum(1e-12, p[np.arange(len(ii)), y[ii]])).mean()
        if loss < best_loss:
            best_loss, best = loss, (w.copy(), b.copy(), epoch)
        if epoch % 100 == 0 or epoch == args.epochs:
            emit("progress", progress=epoch / args.epochs, validation_loss=float(loss))
    w, b, chosen_epoch = best
    report = {"selected_epoch": chosen_epoch, "selection": "minimum validation cross entropy",
              "split_policy": manifest["split_policy"], "feature_sha256": manifest["features_sha256"],
              "sources": sources, "counts": {name: len(ii) for name, ii in indices.items()}}
    for name, ii in indices.items():
        p = probabilities(ii)
        predicted = p.argmax(axis=1)
        matrix = np.zeros((3, 3), dtype=int)
        for actual, prediction in zip(y[ii], predicted):
            matrix[actual, prediction] += 1
        accepted = p.max(axis=1) >= args.threshold
        report[name] = {"accuracy": float((predicted == y[ii]).mean()), "confusion": matrix.tolist(),
                        "coverage": float(accepted.mean()),
                        "escaped_defects": int(((y[ii] != 0) & (predicted == 0) & accepted).sum()),
                        "false_alarms": int(((y[ii] == 0) & (predicted != 0) & accepted).sum())}
    model = {"schema": "slx.assembly-inspector.v1", "model_id": "assembly-" + uuid.uuid4().hex[:12],
             "feature_count": x.shape[1], "asset_sha256": manifest["asset_sha256"],
             "classes": manifest["classes"], "mean": mean.tolist(), "scale": scale.tolist(),
             "weights": w.ravel().tolist(), "bias": b.tolist(), "validation": report,
             "scope": "Synthetic RGB reference inspection, nominal/missing/changed pose; scores uncalibrated"}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    payload = (json.dumps(model, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    comparison = None
    if benchmark:
        bm, bx, br, bh, baseline, baseline_hash = benchmark
        comparison = {"schema": "slx.inspector-comparison.v1", "benchmark_sha256": bh,
                      "baseline_sha256": baseline_hash, "candidate_sha256": hashlib.sha256(payload).hexdigest(),
                      "baseline_id": baseline["model_id"], "candidate_id": model["model_id"],
                      "threshold": args.threshold,
                      "scope": "Synthetic component examples with observable RGB differences; fixed inspection camera. Scores uncalibrated.",
                      "baseline": score(baseline, bx, br, args.threshold),
                      "candidate": score(model, bx, br, args.threshold)}
        ti = indices["test"]
        tx, tr = x[ti], [rows[i] for i in ti]
        comparison["control_baseline"] = score(baseline, tx, tr, args.threshold)
        comparison["control_candidate"] = score(model, tx, tr, args.threshold)
        per_ref = {}
        for reference in sorted({r["reference"] for r in br}):
            ri = [i for i, r in enumerate(br) if r["reference"] == reference]
            per_ref[reference] = {"baseline": score(baseline, bx[ri], [br[i] for i in ri], args.threshold),
                                  "candidate": score(model, bx[ri], [br[i] for i in ri], args.threshold)}
        comparison["per_reference"] = per_ref
        # Recheck immutable inputs before committing the model and its report.
        if load_data(args.benchmark)[3] != bh or hashlib.sha256(args.baseline.read_bytes()).hexdigest() != baseline_hash:
            raise ValueError("Benchmark or baseline changed during training")
        cp = args.output.with_name("comparison.json")
        cp.write_text(json.dumps(comparison, indent=2) + "\n", encoding="utf-8")
        keys = ("samples", "actual_defects", "actual_nominal", "accuracy", "missed_defects_raw", "false_alarms_raw",
                "escaped_defects", "false_alarms", "review", "coverage")
        with cp.with_suffix(".csv").open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle); writer.writerow(("metric", "baseline", "candidate"))
            for key in keys: writer.writerow((key, comparison["baseline"][key], comparison["candidate"][key]))
        table = "".join(f"<tr><td>{key}</td><td>{comparison['baseline'][key]}</td><td>{comparison['candidate'][key]}</td></tr>" for key in keys)
        cp.with_suffix(".html").write_text("<!doctype html><meta charset=utf-8><title>Silicon LogiX — Model comparison</title>"
            "<style>body{font:16px Segoe UI,sans-serif;max-width:1000px;margin:48px auto;color:#18203b}td,th{padding:12px;border-bottom:1px solid #ddd}code{overflow-wrap:anywhere}</style>"
            "<h1>Silicon LogiX · SynthForge</h1><h2>Independent model comparison</h2>"
            f"<p>{html.escape(comparison['scope'])}</p><p>Threshold {args.threshold}; benchmark SHA-256 <code>{bh}</code></p>"
            f"<table><tr><th>Metric</th><th>{html.escape(baseline['model_id'])}</th><th>{html.escape(model['model_id'])}</th></tr>{table}</table>"
            "<p>Review means manual verification. A lower review count can expose more escaped defects. Original control test and per-component results are in comparison.json.</p>", encoding="utf-8")
    temporary = args.output.with_suffix(".tmp")
    temporary.write_bytes(payload)
    os.replace(temporary, args.output)
    emit("complete", model=str(args.output.resolve()), validation=report, comparison=comparison)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        emit("error", message=str(error))
        sys.exit(1)
