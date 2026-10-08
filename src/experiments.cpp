// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Portable local experiments retain captured pixels, edit audit and model identity.
#include "controller.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QUuid>
#include <cmath>
#include <stdexcept>

namespace
{
QByteArray read(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot read experiment file");
    if (file.size() > 16 * 1024 * 1024) throw std::runtime_error("Experiment file exceeds size limit");
    return file.readAll();
}
QString hash(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}
void write(const QString &path, const QByteArray &data)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        throw std::runtime_error("Cannot commit experiment file");
}
QJsonObject json(const QByteArray &bytes)
{
    const auto doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) throw std::runtime_error("Invalid experiment JSON");
    return doc.object();
}
QJsonArray numericResults(const QJsonArray &results)
{
    QJsonArray values;
    for (const auto &value : results)
    {
        auto row = value.toObject();
        // Display text can change between releases; every inference field must match.
        row.remove("class_name");
        row.remove("note");
        values.append(row);
    }
    return values;
}
}
bool Controller::saveExperiment(const QString &name)
{
    if (m_busy || m_inspectionStale || m_assemblyReport.isEmpty())
    { fail("Capture the current board before saving the experiment."); return false; }
    try
    {
        const auto modelBytes = read(m_inspectorPath);
        if (hash(modelBytes) != m_assemblyReport["model_sha256"].toString())
            throw std::runtime_error("Active model changed since acquisition; reacquire before saving");
        const auto path = m_workspace + "/experiments/experiment-" +

            QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-") + QUuid::createUuid().toString(QUuid::Id128).left(8);

        if (!QDir().mkpath(path)) throw std::runtime_error("Cannot create experiment directory");

        write(path + "/incomplete.json", "{\"status\":\"incomplete\"}");

        const auto capture = image("capture");

        const auto nominal = m_engine->render(0, slx::Camera{}, QSize(640, 480)).image;

        if (!capture.save(path + "/capture.png") || !nominal.save(path + "/nominal.png"))

            throw std::runtime_error("Cannot save experiment images");

        write(path + "/inspection.json", QJsonDocument(m_assemblyReport).toJson());

        write(path + "/model.json", modelBytes);

        QJsonObject hashes;

        for (const auto &file : {"capture.png", "nominal.png", "inspection.json", "model.json"})

            hashes[file] = hash(read(path + "/" + file));

        QJsonObject manifest{{"schema", "slx.experiment.v1"}, {"name", name.left(120)},

            {"saved_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},

            {"asset_sha256", m_engine->assetSummary()["sha256"]}, {"edits", m_edits},

            {"selected_reference", m_selectedReference}, {"files", hashes},

            {"model_id", m_assemblyReport["model_id"]}, {"model_sha256", m_assemblyReport["model_sha256"]},

            {"camera", QJsonObject{{"azimuth", m_camera.azimuth}, {"tilt", m_camera.tilt}, {"span", m_camera.span}}}};

        write(path + "/experiment.json", QJsonDocument(manifest).toJson());

        QString rows;

        for (const auto &v : m_assemblyReport["results"].toArray())

        {

            const auto r = v.toObject();

            rows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4%</td></tr>")

                .arg(r["reference"].toString().toHtmlEscaped(), r["decision"].toString().toHtmlEscaped(),

                     r["class_name"].toString().toHtmlEscaped(), QString::number(100 * r["confidence"].toDouble(), 'f', 1));

        }

        const QString html = QString(R"HTML(<!doctype html><meta charset="utf-8"><title>Silicon LogiX · Experiment</title>

<style>body{font:16px Segoe UI,sans-serif;color:#18203b;max-width:1100px;margin:40px auto}img{width:48%;border-radius:12px}td,th{padding:9px;border-bottom:1px solid #dde2ed}pre{white-space:pre-wrap}code{overflow-wrap:anywhere}header{background:#111832;color:white;padding:24px;border-radius:12px}</style>

<header><h1>Silicon LogiX · SynthForge</h1><p>%1</p></header><h2>Inspection evidence</h2>

<p>Model %2 · SHA-256 <code>%3</code></p><p>Nominal reference / captured RGB · 640 × 480 · threshold 0.80</p>

<img src="nominal.png" alt="Nominal reference"><img src="capture.png" alt="Captured RGB">

<p>Defects: %4 · Review: %5 · Pass: %6. Scores are uncalibrated; hidden or visually symmetric parts require verification.</p>

<table><tr><th>CAD reference</th><th>Decision</th><th>Visual class</th><th>Score</th></tr>%7</table>

<h2>Applied edits — audit metadata</h2><p>Edit commands are recorded after inference and never used as recognition inputs.</p><pre>%8</pre>

<p>experiment.json stores asset/model identity and SHA-256 checksums. These checksums detect accidental changes; they are not a digital signature.</p>)HTML")

            .arg(name.isEmpty() ? "Local experiment" : name.toHtmlEscaped(), m_assemblyReport["model_id"].toString().toHtmlEscaped(),

                 m_assemblyReport["model_sha256"].toString(), QString::number(m_assemblyReport["defects"].toInt()),

                 QString::number(m_assemblyReport["review"].toInt()), QString::number(m_assemblyReport["passed"].toInt()), rows,

                 QString::fromUtf8(QJsonDocument(m_edits).toJson()).toHtmlEscaped());

        write(path + "/report.html", html.toUtf8());

        if (!QFile::remove(path + "/incomplete.json")) throw std::runtime_error("Cannot finalize experiment");

        m_experimentPath = path;

        m_status = "Experiment saved with images, edits, results and AI model version.";

        emit editorChanged(); emit changed();

        return true;

    }

    catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); return false; }

}

bool Controller::loadExperiment(const QUrl &directory)

{

    if (m_busy || m_inspectionActive || !directory.isLocalFile()) return false;

    try

    {

        const auto path = QDir::cleanPath(directory.toLocalFile());

        if (QFile::exists(path + "/incomplete.json")) throw std::runtime_error("Experiment is incomplete");

        const auto manifest = json(read(path + "/experiment.json"));

        if (manifest["schema"] != "slx.experiment.v1" || manifest["asset_sha256"] != m_engine->assetSummary()["sha256"])

            throw std::runtime_error("Experiment does not match the current assets");

        const auto files = manifest["files"].toObject();

        for (const auto &file : {"capture.png", "nominal.png", "inspection.json", "model.json"})

            if (hash(read(path + "/" + file)) != files[file].toString()) throw std::runtime_error("Experiment checksum mismatch");

        QSet<QString> known, seen;

        for (const auto &v : m_engine->components()) known.insert(v.toObject()["reference"].toString());

        const auto edits = manifest["edits"].toArray();

        for (const auto &v : edits)

        {

            const auto e = v.toObject(); const auto ref = e["reference"].toString();

            const double dx = e["dx_mm"].toDouble(100), dy = e["dy_mm"].toDouble(100), rotation = e["rotation_deg"].toDouble(1000);

            if (!known.contains(ref) || seen.contains(ref) || !std::isfinite(dx) || !std::isfinite(dy) ||

                !std::isfinite(rotation) || std::abs(dx) > 10 || std::abs(dy) > 10 || std::abs(rotation) > 180)

                throw std::runtime_error("Invalid experiment edit");

            seen.insert(ref);

        }

        const auto frame = QImage::fromData(read(path + "/capture.png"));

        if (frame.size() != QSize(640, 480)) throw std::runtime_error("Invalid acquisition dimensions");

        auto report = json(read(path + "/inspection.json"));

        if (report["edits_at_capture"].toArray() != edits || report["model_sha256"] != hash(read(path + "/model.json")))

            throw std::runtime_error("Experiment audit or model identity mismatch");

        // Validate the saved numeric model through the same binary inference API.

        const auto checked = m_engine->inspectAssembly(frame, slx::Camera{}, path + "/model.json", .80);

        if (numericResults(checked["results"].toArray()) != numericResults(report["results"].toArray()) ||
            checked["model_id"] != report["model_id"] || report["model_id"] != manifest["model_id"] ||
            report["model_sha256"] != manifest["model_sha256"] ||
            checked["schema"] != report["schema"] || checked["asset_sha256"] != report["asset_sha256"] ||
            checked["threshold"] != report["threshold"] || checked["decision"] != report["decision"] ||
            checked["defects"] != report["defects"] || checked["review"] != report["review"] ||
            checked["passed"] != report["passed"] || checked["components"] != report["components"])
            throw std::runtime_error("Saved inspection results do not match the captured pixels");
        report["results"] = checked["results"];
        report["scope"] = checked["scope"];
        report["acquisition"] = "Fixed inspection view at 640 × 480; 3D orbit is for exploration";

        enterEditor(true);

        applyEdits(edits);

        m_assemblyReport = report;
        m_inspectionFrame = {}; m_inspectionFrame.image = frame;

        m_inspectionStale = report["model_sha256"].toString() != hash(read(m_inspectorPath));

        { QMutexLocker lock(&m_imageMutex); m_capture = frame; }

        m_experimentPath = path;

        ++m_inspectionVersion;

        const auto camera = manifest["camera"].toObject();

        const double az = camera["azimuth"].toDouble(-65), tilt = camera["tilt"].toDouble(32), span = camera["span"].toDouble(133);

        if (std::isfinite(az) && std::isfinite(tilt) && std::isfinite(span))

        { setAzimuth(az); setTilt(tilt); setSpan(span); }

        selectReference(manifest["selected_reference"].toString());

        m_status = m_inspectionStale ? "Historical experiment loaded. The active model differs: capture again for a new comparison." : "Experiment loaded and results verified against the saved pixels.";

        emit editorChanged(); emit changed();

        return true;

    }

    catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); return false; }

}

void Controller::openExperiment() const

{

    if (!m_experimentPath.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(m_experimentPath + "/report.html"));

}
