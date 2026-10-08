// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Inspector improvement uses development edits and an immutable independent benchmark.
#include "controller.h"
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QException>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <stdexcept>

namespace
{
QByteArray bytes(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) throw std::runtime_error(("Cannot read " + path).toStdString());
    return f.readAll();
}
QString digest(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}
QJsonObject object(const QString &path)
{
    const auto document = QJsonDocument::fromJson(bytes(path));
    if (!document.isObject()) throw std::runtime_error("Invalid inspector JSON");
    return document.object();
}
void write(const QString &path, const QJsonObject &value)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(value).toJson()) < 0 || !file.commit())
        throw std::runtime_error("Cannot commit inspector workflow");
}
class InspectorJobError : public QException
{
public:
    explicit InspectorJobError(const char *value) : message(value) {}
    const char *what() const noexcept override { return message.c_str(); }
    void raise() const override { throw *this; }
    InspectorJobError *clone() const override { return new InspectorJobError(*this); }
private:
    std::string message;
};
}
void Controller::loadInspectorCycle()
{
    m_inspectorCycle = {};
    m_inspectorCases = {};
    m_inspectorPath = m_projectRoot + "/assets/models/assembly-inspector.json";
    // Packaged training examples are optional; inference needs only the model.
    const auto examples = m_projectRoot + "/examples/inspector-dataset";
    if (QFile::exists(examples + "/dataset.json")) m_inspectorCycle["dataset"] = examples;
    const auto state = m_workspace + "/inspector-cycle.json";
    if (QFile::exists(state))
    {
        try
        {
            const auto loaded = object(state);
            if (loaded["schema"] == "slx.inspector-cycle.v1" &&
                loaded["asset_sha256"] == m_engine->assetSummary()["sha256"])
            {
                m_inspectorCycle = loaded;
                m_inspectorCases = loaded["cases"].toArray();
                const auto path = loaded["active_model"].toString();
                if (QFile::exists(path)) m_inspectorPath = path;
            }
        }
        catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
    }
}
void Controller::initializeInspectorWorkflow()
{
    loadInspectorCycle();
    connect(&m_inspectorDataWatcher, &QFutureWatcher<QString>::finished, this, [this]
    {
        m_busy = false;
        try
        {
            const auto path = m_inspectorDataWatcher.result();
            m_inspectorCycle[m_inspectorTask == "training" ? "dataset" : m_inspectorTask] = path;
            if (m_inspectorTask == "benchmark")
                m_inspectorCycle["benchmark_sha256"] = digest(bytes(path + "/dataset.json") + bytes(path + "/features.f32"));
            m_progress = 1;
            m_status = "Inspector data ready. The frozen test remains separate from training.";
            saveInspectorCycle();
        }
        catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
        catch (...) { fail("Inspector generation interrupted; partial data cannot be used."); }
        emit changed();
    });
    connect(&m_inspectorTrainer, &QProcess::readyReadStandardError, this, [this]
    { m_inspectorStderr = (m_inspectorStderr + QString::fromUtf8(m_inspectorTrainer.readAllStandardError())).right(4000); });
    connect(&m_inspectorTrainer, &QProcess::readyReadStandardOutput, this, [this]
    {
        m_inspectorStdout += m_inspectorTrainer.readAllStandardOutput();
        int newline;
        while ((newline = m_inspectorStdout.indexOf('\n')) >= 0)
        {
            const auto event = QJsonDocument::fromJson(m_inspectorStdout.left(newline)).object();
            m_inspectorStdout.remove(0, newline + 1);
            if (event["event"] == "progress") { m_progress = event["progress"].toDouble(); emit changed(); }
            if (event["event"] == "error") m_inspectorStderr = event["message"].toString();
        }
    });
    connect(&m_inspectorTrainer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error)
    {
        if (error == QProcess::FailedToStart)
        { m_busy = false; fail("Python is unavailable: set its executable in Dataset forge."); }
    });
    connect(&m_inspectorTrainer, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus status)
    {
        m_busy = false;
        if (code != 0 || status != QProcess::NormalExit || m_cancel)
        { fail(m_cancel ? "Training cancelled: the previous model is retained." : m_inspectorStderr); return; }
        try
        {
            const auto model = object(m_inspectorOutput);
            m_inspectorCycle[m_inspectorTask == "candidate" ? "candidate_model" : "baseline_model"] = m_inspectorOutput;
            m_inspectorCycle["validation"] = model["validation"];
            if (m_inspectorTask == "candidate")
                m_inspectorCycle["comparison"] = object(QFileInfo(m_inspectorOutput).absolutePath() + "/comparison.json");
            else m_inspectorPath = m_inspectorOutput;
            m_progress = 1;
            m_status = m_inspectorTask == "candidate" ? "Comparison ready. Review missed defects, false alarms and review counts before choosing a model." : "New baseline ready for the editor.";
            saveInspectorCycle();
            emit editorChanged();
            emit changed();
        }
        catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
    });
}
void Controller::saveInspectorCycle()
{
    m_inspectorCycle["schema"] = "slx.inspector-cycle.v1";
    m_inspectorCycle["asset_sha256"] = m_engine->assetSummary()["sha256"];
    m_inspectorCycle["cases"] = m_inspectorCases;
    m_inspectorCycle["active_model"] = m_inspectorPath;
    if (!QDir().mkpath(m_workspace)) throw std::runtime_error("Cannot create workspace");
    write(m_workspace + "/inspector-cycle.json", m_inspectorCycle);
}
void Controller::inspectorJob(const QString &purpose, int groups)
{
    if (m_busy || m_inspectionActive) return;
    if (purpose == "benchmark" && !m_inspectorCycle["benchmark"].toString().isEmpty())
    { fail("Test already frozen. Generate a new dataset to start another cycle."); return; }
    if (purpose == "augmentation" && (m_inspectorCases.isEmpty() || m_inspectorCycle["benchmark"].toString().isEmpty()))
    { fail("Freeze the test and collect at least one development case."); return; }
    clearError();
    slx::InspectorDataOptions options;
    options.groups = groups;
    options.purpose = purpose;
    options.seed = QRandomGenerator::global()->generate();
    options.focusEdits = m_inspectorCases;
    if (purpose == "training") { m_inspectorCycle = {}; m_inspectorCases = {}; }
    try
    {
        if (purpose == "benchmark")
        {
            const auto directory = m_workspace + "/inspector/" + QUuid::createUuid().toString(QUuid::Id128);
            if (!QDir().mkpath(directory)) throw std::runtime_error("Cannot freeze baseline");
            const auto baseline = directory + "/baseline.json";
            const auto raw = bytes(m_inspectorPath);
            QSaveFile file(baseline);
            if (!file.open(QIODevice::WriteOnly) || file.write(raw) != raw.size() || !file.commit()) throw std::runtime_error("Cannot freeze baseline");
            m_inspectorCycle["frozen_model"] = baseline;
            m_inspectorCycle["frozen_model_sha256"] = digest(raw);
            m_inspectorCycle["baseline_model"] = m_inspectorPath;
        }
    }
    catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); return; }
    m_busy = true; m_cancel = false; m_progress = 0; m_inspectorTask = purpose;
    m_status = "Generating component RGB data: " + purpose;
    emit changed();
    const auto engine = m_engine;
    const auto root = m_workspace + "/inspector";
    m_inspectorDataWatcher.setFuture(QtConcurrent::run([this, engine, root, options]
    {
        try
        {
            return engine->generateInspectorData(root, options, [this](int done, int total)
            {
                QMetaObject::invokeMethod(this, [this, done, total] { m_progress = double(done) / total; emit changed(); }, Qt::QueuedConnection);
            }, m_cancel);
        }
        catch (const std::exception &error) { throw InspectorJobError(error.what()); }
    }));
}
void Controller::generateInspector(int groups) { inspectorJob("training", groups); }
void Controller::freezeInspector(int groups) { inspectorJob("benchmark", groups); }
void Controller::augmentInspector(int groups) { inspectorJob("augmentation", groups); }
void Controller::collectInspectorCase()
{
    if (m_busy || m_inspectionStale || m_assemblyReport.isEmpty()) return;
    if (m_inspectorCycle["benchmark"].toString().isEmpty())
    { fail("Freeze the independent test before collecting development cases from the editor."); return; }
    QJsonObject row;
    for (const auto &v : m_assemblyReport["results"].toArray())
        if (v.toObject()["reference"].toString() == m_selectedReference) row = v.toObject();
    if (row.isEmpty()) { fail("Select a result in the editor."); return; }
    auto edit = QJsonObject::fromVariantMap(componentEdit());
    const auto fingerprint = digest(QJsonDocument(edit).toJson(QJsonDocument::Compact));
    for (const auto &v : m_inspectorCases)
        if (v.toObject()["case_sha256"] == fingerprint) { fail("This case is already in the queue."); return; }
    edit["case_sha256"] = fingerprint;
    edit["prediction"] = row;
    edit["model_id"] = m_assemblyReport["model_id"];
    edit["annotation_source"] = "User-confirmed synthetic edit audit, never passed to inference";
    m_inspectorCases.append(edit);
    // New cases invalidate prepared augmentation; retraining needs regenerated data.
    m_inspectorCycle.remove("augmentation");
    try { saveInspectorCycle(); m_status = "Case confirmed and collected: " + m_selectedReference; emit changed(); }
    catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
}
void Controller::trainInspector(int epochs)
{
    if (m_busy || m_inspectionActive) return;
    const auto dataset = m_inspectorCycle["dataset"].toString();
    if (dataset.isEmpty()) { fail("Generate the inspector dataset first."); return; }
    if (!m_inspectorCycle["benchmark"].toString().isEmpty() && m_inspectorCycle["augmentation"].toString().isEmpty())
    { fail("Prepare additional examples before improving the frozen baseline."); return; }
    clearError();
    const bool candidate = !m_inspectorCycle["augmentation"].toString().isEmpty();
    QStringList args{m_projectRoot + "/tools/train-inspector.py", dataset, "--epochs", QString::number(epochs)};
    if (candidate)
    {
        try
        {
            if (digest(bytes(m_inspectorCycle["frozen_model"].toString())) != m_inspectorCycle["frozen_model_sha256"].toString())
                throw std::runtime_error("Frozen baseline has changed");
        }
        catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); return; }
        args << "--augment" << m_inspectorCycle["augmentation"].toString()
             << "--benchmark" << m_inspectorCycle["benchmark"].toString()
             << "--benchmark-hash" << m_inspectorCycle["benchmark_sha256"].toString()
             << "--baseline" << m_inspectorCycle["frozen_model"].toString();
    }
    m_inspectorOutput = m_workspace + "/inspector/models/" + QUuid::createUuid().toString(QUuid::Id128) + "/model.json";
    args << "--output" << m_inspectorOutput;
    m_inspectorTask = candidate ? "candidate" : "baseline";
    m_inspectorStdout.clear(); m_inspectorStderr.clear();
    m_busy = true; m_cancel = false; m_progress = 0;
    m_status = "Training the inspector and evaluating independently…";
    m_inspectorTrainer.start(m_python, args);
    emit changed();
}
void Controller::activateInspector(bool candidate)
{
    if (m_busy || m_inspectionActive) return;
    const auto path = m_inspectorCycle[candidate ? "candidate_model" : "baseline_model"].toString();
    if (!QFile::exists(path)) { fail("Model version unavailable."); return; }
    m_inspectorPath = path;
    // A saved acquisition belongs to its original model and must be reacquired.
    m_inspectionStale = !m_assemblyReport.isEmpty();
    try { saveInspectorCycle(); emit editorChanged(); emit changed(); }
    catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
}
void Controller::openInspectorReport() const
{
    const auto model = m_inspectorCycle["candidate_model"].toString();
    if (!model.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(model).absolutePath() + "/comparison.html"));
}

bool Controller::hasInspectorExample() const
{
    return QFile::exists(m_projectRoot + "/examples/inspector-cycle/cycle.json");
}
bool Controller::loadInspectorExample()
{
    if (m_busy || m_inspectionActive) return false;
    try
    {
        const auto root = m_projectRoot + "/examples/inspector-cycle";
        auto example = object(root + "/cycle.json");
        if (example["schema"] != "slx.inspector-cycle.v1" || example["asset_sha256"] != m_engine->assetSummary()["sha256"])
            throw std::runtime_error("Example assets mismatch");
        for (const auto &key : {"dataset", "benchmark", "augmentation", "frozen_model", "baseline_model", "candidate_model"})
        {
            const auto relative = example[key].toString();
            const auto path = QDir::cleanPath(root + "/" + relative);
            if (relative.isEmpty() || QDir::isAbsolutePath(relative) ||
                !path.startsWith(QDir::cleanPath(m_projectRoot) + "/", Qt::CaseInsensitive) || !QFileInfo::exists(path))
                throw std::runtime_error("Invalid example path");
            example[key] = path;
        }
        if (digest(bytes(example["frozen_model"].toString())) != example["frozen_model_sha256"].toString() ||
            digest(bytes(example["benchmark"].toString() + "/dataset.json") + bytes(example["benchmark"].toString() + "/features.f32")) != example["benchmark_sha256"].toString())
            throw std::runtime_error("Example checksum mismatch");
        m_inspectorCycle = example;
        m_inspectorCases = example["cases"].toArray();
        saveInspectorCycle();
        m_status = "Loaded the included comparison from a completed run. The active model is retained until you select a version.";
        emit changed();
        return true;
    }
    catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); return false; }
}

QString Controller::inspectorModelId() const
{
    try { return object(m_inspectorPath)["model_id"].toString(); }
    catch (...) { return "Model unavailable"; }
}
