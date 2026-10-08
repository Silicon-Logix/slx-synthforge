#!/usr/bin/env python3
# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

"""Train a real, small visual softmax baseline on registered C1/U4 RGB crops.

Only train samples fit normalization and weights; validation chooses the epoch.
The held-out test split is evaluated once after selection. No filename, scene,
camera or label enters the feature matrix. JSONL progress is consumed by Qt.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import sys
import time
import uuid
from datetime import datetime, timezone
from reporting import compare_models, write_html

# Keep CPU training responsive on workstations with many logical cores.
os.environ.setdefault("OPENBLAS_NUM_THREADS", "2")
os.environ.setdefault("OMP_NUM_THREADS", "2")
try:
    import numpy as np
except ImportError:
    print(json.dumps({"event": "error", "message": "NumPy missing. Run: python -m pip install -r tools/requirements.txt"}), flush=True)
    sys.exit(2)

CLASSES = ["nominal", "missing_component", "misalignment", "reversed_polarity", "solder_bridge"]
FEATURES = 24 * 24 * 3 * 2


def emit(event, **fields):
    print(json.dumps({"event": event, **fields}), flush=True)


def save(path, value):
    temp = path.with_suffix(path.suffix + ".tmp")
    temp.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    temp.replace(path)


def softmax(x):
    x = x - x.max(axis=1, keepdims=True)
    p = np.exp(x)
    return p / p.sum(axis=1, keepdims=True)


def load_dataset(root, purpose="training"):
    d = json.loads((root / "dataset.json").read_text(encoding="utf-8"))
    if d.get("schema") != "slx.dataset.v1" or d.get("feature_count") != FEATURES or d.get("classes") != CLASSES:
        raise ValueError("Unsupported dataset schema or feature layout")
    if (root / "incomplete.json").exists():
        raise ValueError("Dataset generation is incomplete")
    binary = (root / "features.f32").read_bytes()
    if hashlib.sha256(binary).hexdigest() != d["features_sha256"]:
        raise ValueError("Feature checksum mismatch")
    samples = d["samples"]
    if len(binary) != len(samples) * FEATURES * 4:
        raise ValueError("Feature file size mismatch")
    x = np.frombuffer(binary, dtype="<f4").reshape(len(samples), FEATURES).copy()
    if not np.isfinite(x).all() or (x < 0).any() or (x > 1).any():
        raise ValueError("Invalid RGB features")
    y = np.array([s["class_index"] for s in samples], dtype=np.int64)
    if (y < 0).any() or (y >= len(CLASSES)).any():
        raise ValueError("Invalid class index")
    groups = {}
    split = np.array([s["split"] for s in samples])
    for index, s in enumerate(samples):
        if s["index"] != index or s["split"] not in {"train", "validation", "test"}:
            raise ValueError("Invalid sample order or split")
        prev = groups.setdefault(s["capture_group"], s["split"])
        if prev != s["split"]:
            raise ValueError("Capture-group leakage between splits")
    required = ["test"] if purpose == "benchmark" else ["train", "validation"] if purpose == "augmentation" else ["train", "validation", "test"]
    if purpose == "training" and d.get("purpose", "training") != "training":
        raise ValueError("A benchmark or augmentation cannot be the base training dataset")
    if purpose != "training" and d.get("purpose") != purpose:
        raise ValueError(f"Expected a {purpose} dataset")
    if purpose == "benchmark" and set(split) != {"test"}:
        raise ValueError("The frozen benchmark must contain test samples only")
    if purpose == "augmentation" and "test" in set(split):
        raise ValueError("Augmentation must never contain test samples")
    for name in required:
        if set(y[split == name]) != set(range(len(CLASSES))):
            raise ValueError(f"Every class must appear in {name}")
    return d, samples, x, y, split


def metrics(y, p):
    predicted = p.argmax(axis=1)
    matrix = np.zeros((5, 5), dtype=np.int64)
    np.add.at(matrix, (y, predicted), 1)
    per_class = []
    for i, name in enumerate(CLASSES):
        tp = int(matrix[i, i])
        support = int(matrix[i].sum())
        precision = tp / max(1, int(matrix[:, i].sum()))
        recall = tp / max(1, support)
        f1 = 2 * precision * recall / max(1e-12, precision + recall)
        per_class.append({"class_name": name, "precision": precision, "recall": recall, "f1": f1, "support": support})
    return {"accuracy": float((y == predicted).mean()), "macro_f1": float(np.mean([c["f1"] for c in per_class])), "confusion_matrix": matrix.tolist(), "per_class": per_class}


def train(root, epochs, seed, augment=None, benchmark=None, parent_model=None, threshold=.75, benchmark_sha256=None):
    start = time.monotonic()
    d, samples, x, y, splits = load_dataset(root)
    train_idx = np.flatnonzero(splits == "train")
    val_idx = np.flatnonzero(splits == "validation")
    test_idx = np.flatnonzero(splits == "test")
    training_x, validation_x = x[train_idx], x[val_idx]
    yt, yv = y[train_idx], y[val_idx]
    sources = [{"dataset_id": d["id"], "features_sha256": d["features_sha256"], "seed": d["seed"]}]
    domain = d.get("acquisition_domain", {"azimuth_deg": [-80, -50], "tilt_deg": [24, 44], "span_mm": [127, 140], "exposure": [.75, 1.3], "light_azimuth_deg": [-85, 65], "reflection_gain": [1, 1], "blur_sigma_px": [0, 0]})
    if augment:
        extra, _, ax, ay, asp = load_dataset(augment, "augmentation")
        if extra["asset_sha256"] != d["asset_sha256"] or extra["seed"] == d["seed"]:
            raise ValueError("Augmentation must use matching assets and an independent seed")
        training_x = np.concatenate([training_x, ax[asp == "train"]])
        validation_x = np.concatenate([validation_x, ax[asp == "validation"]])
        yt = np.concatenate([yt, ay[asp == "train"]])
        yv = np.concatenate([yv, ay[asp == "validation"]])
        sources.append({"dataset_id": extra["id"], "features_sha256": extra["features_sha256"], "seed": extra["seed"]})
        domain = {key: [min(value[0], extra["acquisition_domain"][key][0]), max(value[1], extra["acquisition_domain"][key][1])] for key, value in domain.items()}
    frozen = None
    if benchmark:
        benchmark_hash = hashlib.sha256((benchmark / "dataset.json").read_bytes()).hexdigest()
        if benchmark_sha256 and benchmark_hash != benchmark_sha256:
            raise ValueError("Frozen benchmark manifest checksum mismatch")
        frozen = load_dataset(benchmark, "benchmark")
        if frozen[0]["asset_sha256"] != d["asset_sha256"] or frozen[0]["seed"] in {s["seed"] for s in sources}:
            raise ValueError("Benchmark assets or independent seed check failed")
        if not parent_model:
            raise ValueError("Comparison requires the previous model")
    parent = json.loads(parent_model.read_text(encoding="utf-8")) if parent_model else None
    if parent and (parent.get("schema") != "slx.roi-softmax.v1" or parent.get("dataset_id") != d["id"] or parent.get("asset_sha256") != d["asset_sha256"]):
        raise ValueError("The previous model does not match the base dataset")
    mean = training_x.mean(axis=0)
    scale = np.maximum(training_x.std(axis=0), .08)
    normalized = (x - mean) / scale
    xt, xv = (training_x - mean) / scale, (validation_x - mean) / scale
    weights = np.zeros((FEATURES, 5), dtype=np.float32)
    bias = np.zeros(5, dtype=np.float32)
    mw, vw = np.zeros_like(weights), np.zeros_like(weights)
    mb, vb = np.zeros_like(bias), np.zeros_like(bias)
    rng = np.random.default_rng(seed)
    best_loss = float("inf")
    best = None
    history = []
    step = 0
    for epoch in range(1, epochs + 1):
        order = rng.permutation(len(xt))
        for offset in range(0, len(order), 32):
            indices = order[offset:offset + 32]
            xb, yb = xt[indices], yt[indices]
            grad = softmax(xb @ weights + bias)
            grad[np.arange(len(yb)), yb] -= 1
            grad /= len(yb)
            gw = xb.T @ grad + .0005 * weights
            gb = grad.sum(axis=0)
            step += 1
            mw = .9 * mw + .1 * gw
            vw = .999 * vw + .001 * gw * gw
            mb = .9 * mb + .1 * gb
            vb = .999 * vb + .001 * gb * gb
            lr = .0025 * (1 - .7 * epoch / epochs)
            weights -= lr * (mw / (1 - .9 ** step)) / (np.sqrt(vw / (1 - .999 ** step)) + 1e-8)
            bias -= lr * (mb / (1 - .9 ** step)) / (np.sqrt(vb / (1 - .999 ** step)) + 1e-8)
        pt, pv = softmax(xt @ weights + bias), softmax(xv @ weights + bias)
        train_loss = float(-np.log(np.maximum(pt[np.arange(len(yt)), yt], 1e-12)).mean())
        val_loss = float(-np.log(np.maximum(pv[np.arange(len(yv)), yv], 1e-12)).mean())
        entry = {"epoch": epoch, "train_loss": train_loss, "validation_loss": val_loss, "validation_accuracy": float((pv.argmax(axis=1) == yv).mean())}
        history.append(entry)
        if val_loss < best_loss:
            best_loss = val_loss
            best = (weights.copy(), bias.copy(), epoch)
        if epoch == 1 or epoch % 5 == 0 or epoch == epochs:
            emit("progress", **entry, epochs=epochs)
    weights, bias, selected_epoch = best
    test_prob = softmax(normalized[test_idx] @ weights + bias)
    result = metrics(y[test_idx], test_prob)
    model_id = "model-" + datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:8]
    output = root / "models" / "versions" / model_id
    output.mkdir(parents=True, exist_ok=False)
    save(output / "incomplete.json", {"status": "incomplete", "model_id": model_id})
    model_path = output / "model.slxmodel.json"
    model = {"schema": "slx.roi-softmax.v1", "model_id": model_id, "parent_model_id": parent.get("model_id", "legacy-baseline") if parent else "", "created_utc": datetime.now(timezone.utc).isoformat(), "training_sources": sources, "feature_count": FEATURES, "classes": CLASSES, "dataset_id": d["id"], "asset_sha256": d["asset_sha256"], "acquisition_domain": domain, "mean": mean.tolist(), "scale": scale.tolist(), "weights": weights.ravel().tolist(), "bias": bias.tolist(), "selected_epoch": selected_epoch, "seed": seed, "architecture": "Registered RGB crops -> train-only standardization -> 5-class linear softmax"}
    save(model_path, model)
    predictions = []
    for i, probability in zip(test_idx, test_prob):
        predicted = int(probability.argmax())
        predictions.append({"sample_index": int(i), "image": samples[i]["image"], "true_index": int(y[i]), "predicted_index": predicted, "confidence": float(probability[predicted]), "correct": bool(predicted == y[i]), "probabilities": probability.tolist()})
    report = {"schema": "slx.evaluation.v1", "dataset_id": d["id"], "model_sha256": hashlib.sha256(model_path.read_bytes()).hexdigest(), "classes": CLASSES, "selected_epoch": selected_epoch, "epochs": epochs, "training_seconds": time.monotonic() - start, "split_counts": {name: int((splits == name).sum()) for name in ["train", "validation", "test"]}, "test": result, "predictions": predictions, "history": history, "scope": d["scope"], "split_policy": d["split_policy"]}
    report_path = output / "evaluation.json"
    report["model_id"] = model_id
    report["training_sources"] = sources
    report["split_counts"] = {"train": len(yt), "validation": len(yv), "test": len(test_idx)}
    save(report_path, report)
    if frozen:
        if hashlib.sha256((benchmark / "dataset.json").read_bytes()).hexdigest() != benchmark_hash or hashlib.sha256((benchmark / "features.f32").read_bytes()).hexdigest() != frozen[0]["features_sha256"]:
            raise ValueError("Frozen benchmark changed during training")
        comparison = compare_models(parent_model, model_path, benchmark, frozen, threshold, (root, d, samples, x, y, splits))
        save(output / "comparison.json", comparison)
        write_html(output / "comparison.html", comparison)
    with (output / "predictions.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["image", "true_class", "predicted_class", "confidence", "correct"])
        for p in predictions:
            writer.writerow([p["image"], CLASSES[p["true_index"]], CLASSES[p["predicted_index"]], p["confidence"], p["correct"]])
    save(output / "completed.json", {"model_id": model_id, "model_sha256": report["model_sha256"]})
    (output / "incomplete.json").unlink()
    save(root / "models" / "current.json", {"model": str(model_path.relative_to(root / "models")).replace("\\", "/"), "model_id": model_id})
    emit("complete", model=str(model_path), report=str(report_path), **result, selected_epoch=selected_epoch, seconds=report["training_seconds"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", required=True, type=Path)
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--seed", type=int, default=2026)
    parser.add_argument("--augment", type=Path)
    parser.add_argument("--benchmark", type=Path)
    parser.add_argument("--parent-model", type=Path)
    parser.add_argument("--threshold", type=float, default=.75)
    parser.add_argument("--benchmark-sha256")
    options = parser.parse_args()
    if not 5 <= options.epochs <= 1000:
        parser.error("epochs must be between 5 and 1000")
    try:
        if not 0 <= options.threshold <= 1:
            raise ValueError("Threshold must be between 0 and 1")
        train(options.dataset.resolve(), options.epochs, options.seed, options.augment, options.benchmark, options.parent_model, options.threshold, options.benchmark_sha256)
    except (OSError, ValueError, KeyError) as error:
        emit("error", message=str(error))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
