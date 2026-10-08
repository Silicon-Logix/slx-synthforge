// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Traceable development cases, frozen benchmarks and immutable model versions.
#include "controller.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
QJsonObject read(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot read workflow artifact");
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) throw std::runtime_error("Invalid workflow artifact");
    return doc.object();
}
void write(const QString &path, const QJsonObject &value)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    const auto bytes = QJsonDocument(value).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("Cannot commit workflow artifact");
}
QString digest(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot hash workflow artifact");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) throw std::runtime_error("Cannot hash workflow artifact");
    return QString::fromLatin1(hash.result().toHex());
}
QString contained(const QString &root, const QString &relative)
{
    const auto path = QDir::cleanPath(QDir(root).absoluteFilePath(relative));
    if (!path.startsWith(QDir(root).absolutePath() + "/", Qt::CaseInsensitive))
        throw std::runtime_error("Workflow path escapes its dataset directory");
    return path;
}
slx::Camera cameraFrom(const QJsonObject &c)
{
    slx::Camera camera;
    camera.azimuth = c["azimuth_deg"].toDouble(-65);
    camera.tilt = c["tilt_deg"].toDouble(32);
    camera.span = c["span_mm"].toDouble(133);
    camera.lightAzimuth = c["light_azimuth_deg"].toDouble(-35);
    camera.exposure = c["exposure"].toDouble(1);
    camera.reflection = c["reflection_gain"].toDouble(1);
    camera.blur = c["blur_sigma_px"].toDouble();
    camera.noiseSeed = quint32(c["noise_seed"].toDouble());
    return camera;
}
bool targetable(const slx::Frame &f)
{
    const auto c = cameraFrom(f.annotations["camera"].toObject());
    return c.azimuth >= -80 && c.azimuth <= -50 && c.span >= 127 && c.span <= 140 &&
           c.tilt >= 18 && c.tilt <= 52 && c.exposure >= .6 && c.exposure <= 1.55 &&
           c.blur <= 2 && c.reflection <= 4 && c.lightAzimuth >= -110 && c.lightAzimuth <= 90 &&
           f.annotations["quality"].toObject()["roi_clipped_fraction"].toDouble() <= .45;
}
QString category(int truth, const QJsonObject &p)
{
    const int predicted = p["class_index"].toInt();
    if (truth > 0 && predicted == 0) return "Missed defect";
    if (truth == 0 && predicted > 0) return "False alarm";
    if (truth != predicted) return "Wrong defect type";
    return "Reacquisition";
}
} // namespace

void Controller::setLightAzimuth(double value)
{
    if (!std::isfinite(value)) return;
    m_camera.lightAzimuth = std::clamp(value, -180., 180.);
    scheduleRender();
    emit cameraChanged();
}
void Controller::setReflection(double value)
{
    if (!std::isfinite(value)) return;
    m_camera.reflection = std::clamp(value, 0., 8.);
    scheduleRender();
    emit cameraChanged();
}
void Controller::setBlur(double value)
{
    if (!std::isfinite(value)) return;
    m_camera.blur = std::clamp(value, 0., 6.);
    scheduleRender();
    emit cameraChanged();
}
void Controller::refreshImages()
{
    m_selection = m_selectedReference.isEmpty() ? QJsonObject() : m_engine->inspect(m_frame, m_selectedReference);
    m_selection.insert("model_id", m_prediction["model_id"]);
    m_selection.insert("decision", m_prediction["decision"]);
    m_selection.insert("frame_source", m_source);
    const auto box = m_selection["bbox"].toArray();
    QImage detail;
    if (box.size() == 4)
        detail = m_frame.image.copy(QRect(box[0].toInt(), box[1].toInt(), box[2].toInt(), box[3].toInt()).adjusted(-12, -12, 12, 12).intersected(m_frame.image.rect()));
    auto visual = m_selectedReference.isEmpty() ? m_frame.image : m_engine->highlight(m_frame, m_selectedReference);
    const auto evidence = m_model && !m_editorMode && !m_orbiting ? m_model->evidence(m_frame, m_prediction["class_index"].toInt()) : QImage();
    QMutexLocker lock(&m_imageMutex);
    m_image = visual;
    m_detail = detail;
    m_evidence = evidence;
}
void Controller::selectReference(const QString &reference)
{
    if (!reference.isEmpty() && !m_engine->inspect(m_frame, reference).contains("cad")) return;
    m_selectedReference = reference;
    refreshImages();
    ++m_version;
    if (m_editorMode)
    {
        QImage detail;
        for (const auto &v : m_assemblyReport["results"].toArray())
        {
            const auto row = v.toObject();
            if (row["reference"].toString() != reference) continue;
            const auto box = row["bbox"].toArray();
            detail = m_inspectionFrame.image.copy(QRect(box[0].toInt(), box[1].toInt(), box[2].toInt(), box[3].toInt()));
        }
        { QMutexLocker lock(&m_imageMutex); m_captureDetail = detail; }
        ++m_inspectionVersion;
    }
    emit editorChanged();
    emit frameChanged();
}
void Controller::pick(double x, double y)
{
    if (m_rendering || m_renderTimer.isActive()) return;
    selectReference(m_engine->referenceAt(m_frame, QPoint(int(x), int(y))));
}
void Controller::loadCycle()
{
    m_cyclePath = m_datasetPath + "/improvement";
    m_cycle = {};
    m_cases = {};
    m_stress = {};
    if (QFile::exists(m_cyclePath + "/cycle.json"))
    {
        m_cycle = read(m_cyclePath + "/cycle.json");
        if (m_cycle["schema"].toString() != "slx.cycle.v1" || m_cycle["dataset_id"] != m_dataset["id"])
            throw std::runtime_error("Improvement cycle belongs to a different dataset");
        if (m_cycle.contains("benchmark")) verifyBenchmark();
        if (QFile::exists(m_cyclePath + "/cases.json")) m_cases = read(m_cyclePath + "/cases.json")["cases"].toArray();
        if (QFile::exists(m_cyclePath + "/stress.json")) m_stress = read(m_cyclePath + "/stress.json");
    }
}
void Controller::saveCycle()
{
    m_cycle.insert("schema", "slx.cycle.v1");
    m_cycle.insert("dataset_id", m_dataset["id"]);
    write(m_cyclePath + "/cycle.json", m_cycle);
    write(m_cyclePath + "/cases.json", {{"schema", "slx.cases.v1"}, {"cases", m_cases}});
}
void Controller::verifyBenchmark() const
{
    const auto root = contained(m_cyclePath, m_cycle["benchmark"].toString());
    if (digest(root + "/dataset.json") != m_cycle["benchmark_manifest_sha256"].toString() ||
        digest(root + "/features.f32") != m_cycle["benchmark_features_sha256"].toString())
        throw std::runtime_error("Frozen benchmark changed; restore the original files before comparison");
}
void Controller::initializeCycle()
{
    connect(&m_cycleWatcher, &QFutureWatcher<QJsonObject>::finished, this, [this]
    {
        m_busy = false;
        const auto result = m_cycleWatcher.result();
        try
        {
            if (result.contains("error")) throw std::runtime_error(result["error"].toString().toStdString());
            if (m_cycleTask == "benchmark")
            {
                const auto path = result["path"].toString();
                const auto document = read(path + "/dataset.json");
                m_cycle.insert("benchmark", QDir(m_cyclePath).relativeFilePath(path));
                m_cycle.insert("benchmark_id", document["id"]);
                m_cycle.insert("benchmark_seed", document["seed"]);
                m_cycle.insert("benchmark_manifest_sha256", digest(path + "/dataset.json"));
                m_cycle.insert("benchmark_features_sha256", document["features_sha256"]);
                m_cycle.insert("benchmark_samples", document["samples"].toArray().size());
                m_cycle.insert("threshold", result["threshold"]);
                m_cycle.insert("frozen_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                saveCycle();
                m_status = "Independent benchmark frozen. Scan development cases next.";
            }
            else if (m_cycleTask == "scan")
            {
                m_stress = result;
                if (QFile::exists(m_cyclePath + "/cases.json")) m_cases = read(m_cyclePath + "/cases.json")["cases"].toArray();
                write(m_cyclePath + "/stress.json", m_stress);
                saveCycle();
                m_status = "Stress scan complete. Eligible development cases collected.";
            }
            else if (m_cycleTask == "augmentation")
            {
                m_cycle.insert("augmentation", QDir(m_cyclePath).relativeFilePath(result["path"].toString()));
                m_cycle.insert("prepared_case_count", result["case_count"]);
                saveCycle();
                m_status = "Targeted examples ready. Train and compare a new version.";
            }
            m_progress = 1;
        }
        catch (const std::exception &e) { fail(QString::fromUtf8(e.what())); }
        emit changed();
    });
}
void Controller::beginCycleJob(const QString &task)
{
    clearError();
    m_busy = true;
    m_cancel = false;
    m_progress = 0;
    m_cycleTask = task;
    m_status = task == "benchmark" ? "Freezing independent challenge images" : task == "scan" ? "Probing optical robustness" : "Generating targeted training examples";
    emit changed();
}
void Controller::freezeBenchmark(int groups)
{
    if (m_busy || m_datasetPath.isEmpty() || !m_model || m_cycle.contains("benchmark")) return;
    if (groups < 15 || groups > 100) { fail("Benchmark requires 15..100 acquisition groups"); return; }
    beginCycleJob("benchmark");
    slx::DatasetOptions options;
    options.groups = groups;
    do { options.seed = QRandomGenerator::global()->generate(); } while (options.seed == quint32(m_dataset["seed"].toDouble()));
    options.resolution = {640, 480};
    options.profile = "stress";
    options.purpose = "benchmark";
    const auto engine = m_engine;
    const auto root = m_cyclePath + "/benchmark";
    const auto threshold = m_threshold;
    m_cycleWatcher.setFuture(QtConcurrent::run([this, engine, root, options, threshold]
    {
        try
        {
            const auto path = engine->generate(root, options, [this](int done, int total)
            { QMetaObject::invokeMethod(this, [this, done, total] { m_progress = double(done) / total; emit changed(); }, Qt::QueuedConnection); }, m_cancel);
            return QJsonObject{{"path", path}, {"threshold", threshold}};
        }
        catch (const std::exception &e) { return QJsonObject{{"error", QString::fromUtf8(e.what())}}; }
    }));
}
void Controller::storeCase(const slx::Frame &frame, const QJsonObject &prediction)
{
    const int truth = frame.annotations["class_index"].toInt();
    const auto key = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject{{"camera", frame.annotations["camera"]}, {"class", truth}}).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex().left(16));
    for (const auto &v : m_cases) if (v.toObject()["id"].toString() == key) return;
    QDir().mkpath(m_cyclePath + "/cases");
    const QString image = "cases/" + key + ".png";
    if (!frame.image.save(m_cyclePath + "/" + image)) throw std::runtime_error("Cannot save development case");
    write(m_cyclePath + "/cases/" + key + ".json", frame.annotations);
    m_cases.append(QJsonObject{{"id", key}, {"image", image}, {"camera", frame.annotations["camera"]},
                              {"true_index", truth}, {"prediction", prediction}, {"category", category(truth, prediction)},
                              {"source", "development"}, {"created_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                              {"model_sha256", digest(m_modelPath)}});
}
bool Controller::canCollect() const
{
    return m_model && !m_busy && m_cycle.contains("benchmark") && m_source == "development" &&
           !m_rendering && !m_renderTimer.isActive() && targetable(m_frame) &&
           (!m_prediction["accepted"].toBool() || m_prediction["class_index"] != m_frame.annotations["class_index"]);
}
void Controller::collectCase()
{
    if (m_busy || !canCollect()) return;
    try
    {
        if (!m_cycle.contains("benchmark")) throw std::runtime_error("Freeze a benchmark before collecting development cases");
        verifyBenchmark();
        if (!targetable(m_frame)) throw std::runtime_error("Reacquire this image first; the recipe is outside the learnable stress range");
        if (m_prediction["accepted"].toBool() && m_prediction["class_index"] == m_frame.annotations["class_index"])
            throw std::runtime_error("This acquisition has no inspection failure to collect");
        storeCase(m_frame, m_prediction);
        saveCycle();
        m_status = "Development case collected with recipe and model provenance";
        emit changed();
    }
    catch (const std::exception &e) { fail(QString::fromUtf8(e.what())); }
}
void Controller::scanStress()
{
    if (m_busy || !m_model || !m_cycle.contains("benchmark")) return;
    try { verifyBenchmark(); } catch (const std::exception &e) { fail(QString::fromUtf8(e.what())); return; }
    beginCycleJob("scan");
    const auto engine = m_engine;
    const auto modelPath = m_modelPath;
    const auto root = m_cyclePath;
    const auto existing = m_cases;
    const auto threshold = m_cycle["threshold"].toDouble(.75);
    m_cycleWatcher.setFuture(QtConcurrent::run([this, engine, modelPath, root, existing, threshold]
    {
        try
        {
            slx::Model model(modelPath);
            QJsonArray rows, cases = existing;
            const QString modelHash = digest(modelPath);
            const QVector<QString> names = {"Reference", "Low light", "Bright light", "Metal reflections", "Defocus", "Shallow angle", "Steep angle", "Combined stress", "Top view", "Severe defocus", "Wide framing", "Overexposure"};
            int correct = 0, accepted = 0, missed = 0, falseAlarms = 0, missedBridges = 0;
            QDir().mkpath(root + "/cases");
            for (int recipe = 0; recipe < names.size(); ++recipe)
            {
                slx::Camera camera;
                // Development recipes use a dedicated stream, independent of
                // the random acquisition groups in the frozen challenge.
                camera.noiseSeed = 0xd3e000u + quint32(recipe);
                if (recipe == 1) camera.exposure = .6;
                if (recipe == 2) camera.exposure = 1.55;
                if (recipe == 3) { camera.reflection = 4; camera.lightAzimuth = 65; }
                if (recipe == 4) camera.blur = 1.7;
                if (recipe == 5) camera.tilt = 18;
                if (recipe == 6) camera.tilt = 52;
                if (recipe == 7) { camera.reflection = 3; camera.exposure = 1.45; camera.blur = 1.5; camera.lightAzimuth = 85; camera.tilt = 42; }
                if (recipe == 8) camera.tilt = 0;
                if (recipe == 9) camera.blur = 5;
                if (recipe == 10) camera.span = 155;
                if (recipe == 11) camera.exposure = 2.2;
                for (int truth = 0; truth < 5; ++truth)
                {
                    if (m_cancel.load()) throw std::runtime_error("Stress scan cancelled; the previous case queue is preserved");
                    const auto frame = engine->render(truth, camera, {640, 480});
                    const auto p = model.predict(frame, threshold);
                    const int predicted = p["class_index"].toInt();
                    correct += predicted == truth;
                    accepted += p["accepted"].toBool();
                    missed += truth > 0 && predicted == 0;
                    falseAlarms += truth == 0 && predicted > 0;
                    missedBridges += truth == 4 && predicted != 4;
                    const bool eligible = targetable(frame) && (truth != predicted || !p["accepted"].toBool());
                    rows.append(QJsonObject{{"recipe", names[recipe]}, {"true_index", truth}, {"camera", frame.annotations["camera"]}, {"prediction", p}, {"collectible", eligible}});
                    if (eligible)
                    {
                        const auto key = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject{{"camera", frame.annotations["camera"]}, {"class", truth}}).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex().left(16));
                        bool duplicate = false;
                        for (const auto &v : cases) duplicate |= v.toObject()["id"].toString() == key;
                        if (!duplicate)
                        {
                            const QString image = "cases/" + key + ".png";
                            if (!frame.image.save(root + "/" + image)) throw std::runtime_error("Cannot save stress case");
                            write(root + "/cases/" + key + ".json", frame.annotations);
                            cases.append(QJsonObject{{"id", key}, {"image", image}, {"camera", frame.annotations["camera"]}, {"true_index", truth},
                                                     {"prediction", p}, {"category", category(truth, p)}, {"source", "development"}, {"model_sha256", modelHash},
                                                     {"created_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}});
                        }
                    }
                    const int done = recipe * 5 + truth + 1;
                    QMetaObject::invokeMethod(this, [this, done] { m_progress = done / 60.; emit changed(); }, Qt::QueuedConnection);
                }
            }
            write(root + "/cases.json", {{"schema", "slx.cases.v1"}, {"cases", cases}});
            return QJsonObject{{"schema", "slx.stress.v1"}, {"model_sha256", modelHash}, {"model_id", rows[0].toObject()["prediction"].toObject()["model_id"]},
                               {"threshold", threshold}, {"samples", rows.size()}, {"correct", correct}, {"accepted", accepted}, {"reacquire", rows.size() - accepted},
                               {"missed_defects", missed}, {"false_alarms", falseAlarms}, {"missed_bridges", missedBridges}, {"rows", rows}, {"cases_added", cases.size() - existing.size()}};
        }
        catch (const std::exception &e) { return QJsonObject{{"error", QString::fromUtf8(e.what())}}; }
    }));
}
void Controller::prepareImprovement(int groups)
{
    if (m_busy || !m_model || m_cases.isEmpty() || !m_cycle.contains("benchmark")) return;
    try { verifyBenchmark(); } catch (const std::exception &e) { fail(QString::fromUtf8(e.what())); return; }
    if (groups < 15 || groups > 500) { fail("Augmentation requires 15..500 groups"); return; }
    slx::DatasetOptions options;
    options.groups = groups;
    options.resolution = {640, 480};
    options.profile = "targeted";
    options.purpose = "augmentation";
    do { options.seed = QRandomGenerator::global()->generate(); }
    while (options.seed == quint32(m_dataset["seed"].toDouble()) || options.seed == quint32(m_cycle["benchmark_seed"].toDouble()));
    for (const auto &v : m_cases)
    {
        const auto recipe = v.toObject()["camera"];
        if (!options.focusRecipes.contains(recipe)) options.focusRecipes.append(recipe);
    }
    beginCycleJob("augmentation");
    const auto engine = m_engine;
    const auto root = m_cyclePath + "/augmentation";
    const int caseCount = m_cases.size();
    m_cycleWatcher.setFuture(QtConcurrent::run([this, engine, root, options, caseCount]
    {
        try
        {
            const auto path = engine->generate(root, options, [this](int done, int total)
            { QMetaObject::invokeMethod(this, [this, done, total] { m_progress = double(done) / total; emit changed(); }, Qt::QueuedConnection); }, m_cancel);
            return QJsonObject{{"path", path}, {"case_count", caseCount}};
        }
        catch (const std::exception &e) { return QJsonObject{{"error", QString::fromUtf8(e.what())}}; }
    }));
}
void Controller::trainImprovement(int epochs)
{
    if (m_busy || !m_model || !m_cycle.contains("augmentation")) return;
    try
    {
        verifyBenchmark();
        startTrainer(epochs, {"--augment", contained(m_cyclePath, m_cycle["augmentation"].toString()), "--benchmark", contained(m_cyclePath, m_cycle["benchmark"].toString()),
                              "--parent-model", m_modelPath, "--benchmark-sha256", m_cycle["benchmark_manifest_sha256"].toString(), "--threshold", QString::number(m_cycle["threshold"].toDouble(.75))});
    }
    catch (const std::exception &e) { fail(QString::fromUtf8(e.what())); }
}
void Controller::inspectCase(int index)
{
    if (m_busy || m_rendering || index < 0 || index >= m_cases.size()) return;
    const auto item = m_cases[index].toObject();
    m_camera = cameraFrom(item["camera"].toObject());
    m_scene = item["true_index"].toInt();
    m_source = "development";
    m_selectedReference = m_scene == 1 || m_scene == 3 ? "C1" : "U4";
    m_renderTimer.start();
    emit changed();
}
QUrl Controller::caseUrl(const QString &relative) const
{
    try { return QUrl::fromLocalFile(contained(m_cyclePath, relative)); } catch (...) { return {}; }
}
void Controller::inspectProbe(int index)
{
    const auto rows = m_stress["rows"].toArray();
    if (m_busy || m_rendering || index < 0 || index >= rows.size()) return;
    const auto item = rows[index].toObject();
    m_camera = cameraFrom(item["camera"].toObject());
    m_scene = item["true_index"].toInt();
    m_source = "development";
    m_selectedReference = m_scene == 1 || m_scene == 3 ? "C1" : "U4";
    m_renderTimer.start();
    emit changed();
}
void Controller::refreshVersions()
{
    m_versions = {};
    const auto baseline = m_datasetPath + "/models/baseline.slxmodel.json";
    if (QFile::exists(baseline)) m_versions.append(QJsonObject{{"id", "legacy-baseline"}, {"path", baseline}, {"label", "Original baseline"}});
    const QDir dir(m_datasetPath + "/models/versions");
    for (const auto &name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
    {
        const auto root = dir.filePath(name);
        if (!QFile::exists(root + "/completed.json") || QFile::exists(root + "/incomplete.json")) continue;
        m_versions.append(QJsonObject{{"id", name}, {"path", root + "/model.slxmodel.json"}, {"label", name}});
    }
}
void Controller::openComparison() const
{
    if (!m_comparison.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_modelPath).absolutePath() + "/comparison.html"));
}
void Controller::openCycle() const
{
    if (!m_cyclePath.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(m_cyclePath));
}
