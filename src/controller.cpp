// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Keeps rendering/generation off the UI thread and exposes real run artifacts.
#include "controller.h"
#include <QCryptographicHash>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
// Preserve actionable errors across QtConcurrent's exception boundary.
class JobError : public QException
{
  public:
    explicit JobError(const char *message) : description(message) {}
    const char *what() const noexcept override { return description.c_str(); }
    void raise() const override { throw *this; }
    JobError *clone() const override { return new JobError(*this); }

  private:
    std::string description;
};
template <typename Task> auto runJob(Task task) -> decltype(task())
{
    try
    {
        return task();
    }
    catch (const std::exception &error)
    {
        throw JobError(error.what());
    }
}
QJsonObject readJson(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(("Cannot open " + path).toStdString());
    QJsonParseError error;
    auto d = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !d.isObject())
        throw std::runtime_error("Invalid JSON document");
    return d.object();
}
QString local(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}
} // namespace
Controller::Controller(const QString &assets, const QString &project, const QString &workspace,
                       QObject *parent)
    : QObject(parent), m_engine(std::make_shared<slx::Engine>(assets)), m_projectRoot(project),
      m_workspace(workspace)
{
    QSettings settings;
    m_python = settings.value("python", "python").toString();
    m_renderTimer.setSingleShot(true);
    m_renderTimer.setInterval(45);
    connect(&m_renderTimer, &QTimer::timeout, this, &Controller::requestRender);
    connect(&m_renderWatcher, &QFutureWatcher<slx::Frame>::finished, this,
            [this]
            {
                m_rendering = false;
                try
                {
                    publish(m_renderWatcher.result());
                }
                catch (const std::exception &e)
                {
                    fail(QString::fromUtf8(e.what()));
                }
                catch (...)
                {
                    fail("Scene rendering failed");
                }
                emit changed();
                if (m_pending)
                {
                    m_pending = false;
                    requestRender();
                }
            });
    connect(&m_datasetWatcher, &QFutureWatcher<QString>::finished, this,
            [this]
            {
                m_busy = false;
                try
                {
                    const auto path = m_datasetWatcher.result();
                    loadDataset(QUrl::fromLocalFile(path));
                    m_status = "Dataset ready. Train the visual baseline.";
                    m_progress = 1;
                }
                catch (const std::exception &e)
                {
                    fail(QString::fromUtf8(e.what()));
                }
                catch (...)
                {
                    fail(m_cancel ? "Generation cancelled. Partial dataset is marked incomplete."
                                  : "Dataset generation failed");
                }
                emit changed();
            });
    connect(&m_trainer, &QProcess::readyReadStandardOutput, this, &Controller::readTrainingOutput);
    connect(&m_trainer, &QProcess::readyReadStandardError, this,
            [this]
            {
                m_stderr += QString::fromUtf8(m_trainer.readAllStandardError());
                m_stderr = m_stderr.right(4000);
            });
    connect(
        &m_trainer, &QProcess::errorOccurred, this,
        [this](QProcess::ProcessError e)
        {
            if (e == QProcess::FailedToStart)
            {
                m_busy = false;
                fail("Cannot start Python. Set its executable path in Dataset / Python runtime.");
            }
        });
    connect(&m_trainer, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus exit)
            {
                readTrainingOutput();
                m_busy = false;
                if (code == 0 && exit == QProcess::NormalExit && !m_cancel)
                {
                    if (m_trainedModel.isEmpty() || !loadModel(QUrl::fromLocalFile(m_trainedModel)))
                    {
                        if (m_error.isEmpty()) fail("Trainer did not publish a completed model");
                        return;
                    }
                    refreshVersions();
                    m_status = "Model version saved. Held-out evaluation is ready.";
                    m_progress = 1;
                }
                else if (m_cancel)
                    m_status = "Training cancelled";
                else
                    fail(m_error.isEmpty() ? "Training failed. " + m_stderr : m_error);
                emit changed();
            });
    initializeEditor();
    initializeCycle();
    m_frame = m_engine->render(0, m_camera);
    publish(m_frame);
}
Controller::~Controller()
{
    m_cancel = true;
    m_inspectionWatcher.waitForFinished();
    m_inspectorDataWatcher.waitForFinished();
    if (m_inspectorTrainer.state() != QProcess::NotRunning)
    {
        m_inspectorTrainer.kill();
        m_inspectorTrainer.waitForFinished(3000);
    }
    m_renderWatcher.waitForFinished();
    m_datasetWatcher.waitForFinished();
    m_cycleWatcher.waitForFinished();
    if (m_trainer.state() != QProcess::NotRunning)
    {
        m_trainer.kill();
        m_trainer.waitForFinished(3000);
    }
}
QImage Controller::image(const QString &kind) const
{
    QMutexLocker lock(&m_imageMutex);
    if (kind == "capture") return m_capture;
    if (kind == "capture-detail") return m_captureDetail;
    return kind == "detail" ? m_detail : kind == "evidence" ? m_evidence : m_image;
}
void Controller::fail(const QString &s)
{
    m_error = s;
    m_status = "Action needs attention";
    emit changed();
}
void Controller::clearError()
{
    m_error.clear();
    emit changed();
}
void Controller::setWorkspace(const QString &s)
{
    if (!m_busy && !m_inspectionActive && !s.isEmpty())
    {
        m_workspace = QDir::cleanPath(s);
        loadInspectorCycle();
        if (!m_assemblyReport.isEmpty())
        {
            QFile model(m_inspectorPath);
            const auto modelBytes = model.open(QIODevice::ReadOnly) ? model.readAll() : QByteArray{};
            m_inspectionStale = m_assemblyReport["model_sha256"].toString() !=
                QString::fromLatin1(QCryptographicHash::hash(modelBytes, QCryptographicHash::Sha256).toHex());
        }
        emit editorChanged();
        emit changed();
    }
}
void Controller::setPython(const QString &s)
{
    if (!m_busy && !s.isEmpty())
    {
        m_python = s;
        QSettings().setValue("python", s);
        emit changed();
    }
}
void Controller::setSceneIndex(int v)
{
    if (v >= 0 && v < 5 && v != m_scene)
    {
        enterEditor(false);
        m_scene = v;
        m_renderTimer.start();
        emit changed();
    }
}
void Controller::setAzimuth(double v)
{
    if (std::isfinite(v))
    {
        m_camera.azimuth = std::clamp(v, -180., 180.);
        scheduleRender();
        emit cameraChanged();
    }
}
void Controller::setTilt(double v)
{
    if (std::isfinite(v))
    {
        m_camera.tilt = std::clamp(v, 0., 60.);
        scheduleRender();
        emit cameraChanged();
    }
}
void Controller::setExposure(double v)
{
    if (std::isfinite(v))
    {
        m_camera.exposure = std::clamp(v, .4, 1.8);
        scheduleRender();
        emit cameraChanged();
    }
}
void Controller::setSpan(double v)
{
    if (std::isfinite(v))
    {
        m_camera.span = std::clamp(v, 100., 185.);
        scheduleRender();
        emit cameraChanged();
    }
}
void Controller::setThreshold(double v)
{
    if (std::isfinite(v))
    {
        m_threshold = std::clamp(v, .1, .99);
        if (m_model)
        {
            m_prediction = m_model->predict(m_frame, m_threshold);
            refreshImages();
        }
        emit frameChanged();
        emit changed();
    }
}
void Controller::resetCamera()
{
    m_source = "development";
    m_camera = slx::Camera();
    scheduleRender();
    emit cameraChanged();
}
void Controller::topCamera()
{
    m_camera.azimuth = -90;
    m_camera.tilt = 0;
    m_camera.span = 133;
    scheduleRender();
    emit cameraChanged();
}
void Controller::requestRender()
{
    if (m_rendering)
    {
        m_pending = true;
        return;
    }
    m_rendering = true;
    emit changed();
    const auto engine = m_engine;
    const auto camera = m_camera;
    const int scene = m_scene;
    const auto edits = m_edits;
    const bool editor = m_editorMode;
    const QSize size = m_orbiting ? QSize(320, 240) : QSize(960, 720);
    m_renderWatcher.setFuture(QtConcurrent::run(
        [engine, camera, scene, edits, editor, size] { return runJob([&]
        { return editor ? engine->renderAssembly(edits, camera, size) : engine->render(scene, camera, size); }); }));
}
void Controller::publish(slx::Frame f)
{
    m_frame = std::move(f);
    m_prediction = m_model && !m_editorMode && !m_orbiting ? m_model->predict(m_frame, m_threshold) : QJsonObject();
    refreshImages();
    ++m_version;
    emit frameChanged();
    emit changed();
}
void Controller::generate(int groups, int seed)
{
    if (m_busy)
        return;
    clearError();
    m_busy = true;
    m_cancel = false;
    m_progress = 0;
    m_status = "Rendering images and scene-derived labels";
    emit changed();
    const auto engine = m_engine;
    const auto root = m_workspace;
    slx::DatasetOptions options;
    options.groups = groups;
    options.seed = quint32(seed);
    options.profile = m_robustDataset ? "stress" : "nominal";
    m_datasetWatcher.setFuture(QtConcurrent::run(
        [this, engine, root, options]
        {
            return runJob(
                [&]
                {
                    return engine->generate(
                        root, options,
                        [this](int done, int total)
                        {
                            QMetaObject::invokeMethod(
                                this,
                                [this, done, total]
                                {
                                    m_progress = double(done) / total;
                                    m_status =
                                        QString("Generating dataset  %1 / %2").arg(done).arg(total);
                                    emit changed();
                                },
                                Qt::QueuedConnection);
                        },
                        m_cancel);
                });
        }));
}
void Controller::train(int epochs)
{
    startTrainer(epochs, {});
}
void Controller::startTrainer(int epochs, const QStringList &extra)
{
    if (m_busy || m_datasetPath.isEmpty())
        return;
    const auto script = m_projectRoot + "/tools/train.py";
    if (!QFile::exists(script))
    {
        fail("Trainer script not found: " + script);
        return;
    }
    clearError();
    m_stderr.clear();
    m_stdout.clear();
    m_trainedModel.clear();
    m_cancel = false;
    m_progress = 0;
    m_busy = true;
    m_status = "Starting local Python trainer";
    emit changed();
    m_trainer.setProgram(m_python);
    m_trainer.setArguments(QStringList{"-u", script, "--dataset", m_datasetPath, "--epochs", QString::number(epochs)} + extra);
    m_trainer.start();
}
void Controller::readTrainingOutput()
{
    m_stdout += m_trainer.readAllStandardOutput();
    int newline;
    while ((newline = int(m_stdout.indexOf('\n'))) >= 0)
    {
        const auto line = m_stdout.left(newline);
        m_stdout.remove(0, newline + 1);
        const auto o = QJsonDocument::fromJson(line).object();
        const auto event = o["event"].toString();
        if (event == "progress")
        {
            m_progress = o["epoch"].toDouble() / o["epochs"].toDouble();
            m_status = QString("Epoch %1 / %2   ·   Validation accuracy %3%")
                           .arg(o["epoch"].toInt())
                           .arg(o["epochs"].toInt())
                           .arg(o["validation_accuracy"].toDouble() * 100, 0, 'f', 1);
            emit changed();
        }
        else if (event == "error")
            fail(o["message"].toString());
        else if (event == "complete")
            m_trainedModel = o["model"].toString();
    }
}
void Controller::cancel()
{
    m_cancel = true;
    if (m_trainer.state() != QProcess::NotRunning)
    {
        m_trainer.terminate();
        QTimer::singleShot(2000, this,
                           [this]
                           {
                               if (m_trainer.state() != QProcess::NotRunning && m_cancel)
                                   m_trainer.kill();
                           });
    }
    if (m_inspectorTrainer.state() != QProcess::NotRunning) m_inspectorTrainer.kill();
    m_status = "Cancelling current job";
    emit changed();
}
bool Controller::loadDataset(const QUrl &url)
{
    if (m_busy)
        return false;
    try
    {
        QString root = local(url);
        if (QFileInfo(root).isFile())
            root = QFileInfo(root).absolutePath();
        auto dataset = readJson(root + "/dataset.json");
        if (dataset["schema"].toString() != "slx.dataset.v1" ||
            dataset["feature_count"].toInt() != slx::FeatureCount ||
            QFile::exists(root + "/incomplete.json") ||
            dataset["classes"].toArray() != QJsonArray::fromStringList(slx::classNames()))
            throw std::runtime_error("Unsupported or incomplete dataset");
        if (dataset["asset_sha256"].toString() != m_engine->assetSummary()["sha256"].toString())
            throw std::runtime_error("Dataset was generated from different PowerBoard assets");
        const auto samples = dataset["samples"].toArray();
        QFile f(root + "/features.f32");
        if (!f.open(QIODevice::ReadOnly) ||
            f.size() != qint64(samples.size()) * slx::FeatureCount * 4)
            throw std::runtime_error("Dataset feature file is missing or truncated");
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(&f);
        if (QString::fromLatin1(hash.result().toHex()) != dataset["features_sha256"].toString())
            throw std::runtime_error("Dataset feature checksum mismatch");
        QJsonObject counts;
        QHash<int, QString> groups;
        for (const auto &v : samples)
        {
            auto s = v.toObject();
            const auto split = s["split"].toString();
            const int g = s["capture_group"].toInt(-1);
            if (g < 0 || (groups.contains(g) && groups[g] != split) ||
                !QStringList{"train", "validation", "test"}.contains(split))
                throw std::runtime_error("Invalid dataset split groups");
            groups[g] = split;
            counts[split] = counts[split].toInt() + 1;
        }
        dataset.insert("split_counts", counts);
        m_dataset = dataset;
        m_datasetPath = QFileInfo(root).absoluteFilePath();
        m_evaluation = {};
        m_model.reset();
        m_modelPath.clear();
        m_prediction = {};
        m_comparison = {};
        loadCycle();
        refreshVersions();
        m_status = "Dataset loaded";
        emit changed();
        emit frameChanged();
        if (QFile::exists(root + "/models/current.json"))
        {
            const auto current = readJson(root + "/models/current.json")["model"].toString();
            const auto path = QDir::cleanPath(root + "/models/" + current);
            if (!path.startsWith(QDir::cleanPath(root + "/models/") + "/", Qt::CaseInsensitive))
                throw std::runtime_error("Invalid model registry path");
            loadModel(QUrl::fromLocalFile(path));
        }
        else if (QFile::exists(root + "/models/baseline.slxmodel.json"))
            loadModel(QUrl::fromLocalFile(root + "/models/baseline.slxmodel.json"));
        return true;
    }
    catch (const std::exception &e)
    {
        fail(QString::fromUtf8(e.what()));
        return false;
    }
}
bool Controller::loadModel(const QUrl &url)
{
    if (m_busy)
        return false;
    try
    {
        const auto path = local(url);
        if (m_datasetPath.isEmpty())
        {
            QDir ancestor(QFileInfo(path).absolutePath());
            for (int i = 0; i < 5 && !QFile::exists(ancestor.filePath("dataset.json")); ++i)
                if (!ancestor.cdUp()) break;
            const auto possibleRoot = ancestor.absolutePath();
            if (QFile::exists(possibleRoot + "/dataset.json") &&
                !loadDataset(QUrl::fromLocalFile(possibleRoot)))
                return false;
        }
        const auto document = readJson(path);
        if (QFile::exists(QFileInfo(path).absolutePath() + "/incomplete.json"))
            throw std::runtime_error("Model version is incomplete");
        if (document["asset_sha256"].toString() != m_engine->assetSummary()["sha256"].toString())
            throw std::runtime_error("Model was trained on different PowerBoard assets");
        auto model = std::make_unique<slx::Model>(path);
        if (!m_datasetPath.isEmpty() && model->datasetId() != m_dataset["id"].toString())
            throw std::runtime_error(
                "Model belongs to a different dataset; load its dataset first");
        QJsonObject report;
        const auto reportPath = QFileInfo(path).absolutePath() + "/evaluation.json";
        if (QFile::exists(reportPath) && !m_datasetPath.isEmpty())
        {
            report = readJson(reportPath);
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                throw std::runtime_error("Cannot hash model");
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(&f);
            if (report["schema"].toString() != "slx.evaluation.v1" ||
                report["dataset_id"].toString() != model->datasetId() ||
                report["model_sha256"].toString() != QString::fromLatin1(hash.result().toHex()))
                throw std::runtime_error("Evaluation report does not match model");
        }
        QJsonObject comparison;
        const auto comparisonPath = QFileInfo(path).absolutePath() + "/comparison.json";
        if (QFile::exists(comparisonPath))
        {
            comparison = readJson(comparisonPath);
            if (comparison["after_sha256"] != report["model_sha256"])
                throw std::runtime_error("Comparison does not match model");
        }
        m_model = std::move(model);
        m_modelPath = path;
        m_evaluation = report;
        m_comparison = comparison;
        m_prediction = m_model->predict(m_frame, m_threshold);
        refreshImages();
        ++m_version;
        refreshVersions();
        m_status = "Visual model loaded";
        emit changed();
        emit frameChanged();
        return true;
    }
    catch (const std::exception &e)
    {
        fail(QString::fromUtf8(e.what()));
        return false;
    }
}
QUrl Controller::sampleUrl(const QString &relative) const
{
    const QString path = QDir(m_datasetPath).absoluteFilePath(relative);
    const QString prefix = QDir(m_datasetPath).absolutePath() + "/";
    if (!QDir::cleanPath(path).startsWith(prefix, Qt::CaseInsensitive))
        return {};
    return QUrl::fromLocalFile(path);
}
void Controller::selectSample(int index)
{
    m_editorMode = false;
    m_inspectionActive = false;
    emit editorChanged();
    if (m_rendering || m_busy)
        return;
    try
    {
        const auto samples = m_dataset["samples"].toArray();
        if (index < 0 || index >= samples.size())
            return;
        const auto s = samples[index].toObject();
        slx::Frame f;
        f.image = QImage(sampleUrl(s["image"].toString()).toLocalFile());
        if (f.image.isNull())
            throw std::runtime_error("Sample image is missing");
        f.annotations = readJson(sampleUrl(s["annotation"].toString()).toLocalFile());
        f.instanceMask = QImage(sampleUrl(s["mask"].toString()).toLocalFile());
        QFile binary(m_datasetPath + "/features.f32");
        if (!binary.open(QIODevice::ReadOnly) ||
            !binary.seek(qint64(index) * slx::FeatureCount * 4))
            throw std::runtime_error("Cannot read sample features");
        QDataStream stream(&binary);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        f.features.resize(slx::FeatureCount);
        for (float &v : f.features)
            stream >> v;
        if (stream.status() != QDataStream::Ok)
            throw std::runtime_error("Truncated sample features");
        m_scene = s["class_index"].toInt();
        const auto c = f.annotations["camera"].toObject();
        m_camera.azimuth = c["azimuth_deg"].toDouble();
        m_camera.tilt = c["tilt_deg"].toDouble();
        m_camera.span = c["span_mm"].toDouble();
        m_camera.lightAzimuth = c["light_azimuth_deg"].toDouble();
        m_camera.exposure = c["exposure"].toDouble();
        m_camera.reflection = c["reflection_gain"].toDouble(1);
        m_camera.blur = c["blur_sigma_px"].toDouble();
        m_camera.noiseSeed = quint32(c["noise_seed"].toDouble());
        m_source = s["split"].toString() == "test" ? "held-out" : "dataset";
        m_renderTimer.stop();
        publish(std::move(f));
        emit cameraChanged();
        m_status = QString("Test sample %1 loaded into inspection view").arg(index);
        emit changed();
    }
    catch (const std::exception &e)
    {
        fail(QString::fromUtf8(e.what()));
    }
}
void Controller::openWorkspace() const
{
    QDir().mkpath(m_workspace);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_workspace));
}
void Controller::chooseWorkspace(const QUrl &url)
{
    if (url.isLocalFile())
        setWorkspace(url.toLocalFile());
}
void Controller::openDataset() const
{
    if (!m_datasetPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_datasetPath));
}
void Controller::saveFrame(const QUrl &url)
{
    if (!m_frame.image.save(local(url)))
        fail("Cannot save image");
    else
    {
        m_status = "Inspection image saved";
        emit changed();
    }
}
