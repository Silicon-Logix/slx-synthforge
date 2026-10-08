// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief UI orchestration, asynchronous jobs and local training process lifecycle.
#pragma once
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <slx/core.h>

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool rendering READ rendering NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString workspace READ workspace WRITE setWorkspace NOTIFY changed)
    Q_PROPERTY(QString python READ python WRITE setPython NOTIFY changed)
    Q_PROPERTY(QString datasetPath READ datasetPath NOTIFY changed)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY changed)
    Q_PROPERTY(QVariantMap dataset READ dataset NOTIFY changed)
    Q_PROPERTY(QVariantMap evaluation READ evaluation NOTIFY changed)
    Q_PROPERTY(QVariantMap annotations READ annotations NOTIFY frameChanged)
    Q_PROPERTY(QVariantMap prediction READ prediction NOTIFY frameChanged)
    Q_PROPERTY(QVariantMap assets READ assets CONSTANT)
    Q_PROPERTY(int frameVersion READ frameVersion NOTIFY frameChanged)
    Q_PROPERTY(int sceneIndex READ sceneIndex WRITE setSceneIndex NOTIFY changed)
    Q_PROPERTY(double azimuth READ azimuth WRITE setAzimuth NOTIFY cameraChanged)
    Q_PROPERTY(double tilt READ tilt WRITE setTilt NOTIFY cameraChanged)
    Q_PROPERTY(double exposure READ exposure WRITE setExposure NOTIFY cameraChanged)
    Q_PROPERTY(double span READ span WRITE setSpan NOTIFY cameraChanged)
    Q_PROPERTY(double threshold READ threshold WRITE setThreshold NOTIFY frameChanged)
    Q_PROPERTY(double lightAzimuth READ lightAzimuth WRITE setLightAzimuth NOTIFY cameraChanged)
    Q_PROPERTY(double reflection READ reflection WRITE setReflection NOTIFY cameraChanged)
    Q_PROPERTY(double blur READ blur WRITE setBlur NOTIFY cameraChanged)
    Q_PROPERTY(QVariantMap selection READ selection NOTIFY frameChanged)
    Q_PROPERTY(QVariantMap cycle READ cycle NOTIFY changed)
    Q_PROPERTY(QVariantMap comparison READ comparison NOTIFY changed)
    Q_PROPERTY(QVariantMap stress READ stress NOTIFY changed)
    Q_PROPERTY(QVariantList cases READ cases NOTIFY changed)
    Q_PROPERTY(QVariantList versions READ versions NOTIFY changed)
    Q_PROPERTY(bool canCollect READ canCollect NOTIFY changed)
    Q_PROPERTY(bool robustDataset READ robustDataset WRITE setRobustDataset NOTIFY changed)
    Q_PROPERTY(QStringList labels READ labels CONSTANT)
    Q_PROPERTY(bool editorMode READ editorMode NOTIFY editorChanged)
    Q_PROPERTY(bool inspectionActive READ inspectionActive NOTIFY editorChanged)
    Q_PROPERTY(bool inspectionScanning READ inspectionScanning NOTIFY editorChanged)
    Q_PROPERTY(bool inspectionStale READ inspectionStale NOTIFY editorChanged)
    Q_PROPERTY(bool acceleratedPreview READ acceleratedPreview NOTIFY editorChanged)
    Q_PROPERTY(QVariantList components READ components CONSTANT)
    Q_PROPERTY(QVariantList edits READ edits NOTIFY editorChanged)
    Q_PROPERTY(QString selectedReference READ selectedReference NOTIFY editorChanged)
    Q_PROPERTY(QVariantMap componentEdit READ componentEdit NOTIFY editorChanged)
    Q_PROPERTY(QVariantMap assemblyReport READ assemblyReport NOTIFY editorChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY editorChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY editorChanged)
    Q_PROPERTY(QString experimentPath READ experimentPath NOTIFY editorChanged)
    Q_PROPERTY(QVariantMap inspectorCycle READ inspectorCycle NOTIFY changed)
    Q_PROPERTY(QVariantList inspectorCases READ inspectorCases NOTIFY changed)
    Q_PROPERTY(bool hasInspectorExample READ hasInspectorExample CONSTANT)
    Q_PROPERTY(QString inspectorModelId READ inspectorModelId NOTIFY editorChanged)
    Q_PROPERTY(QString inspectorModel READ inspectorModel NOTIFY editorChanged)
    Q_PROPERTY(int inspectionVersion READ inspectionVersion NOTIFY editorChanged)
  public:
    Controller(const QString &assetRoot, const QString &projectRoot, const QString &workRoot,
               QObject *parent = nullptr);
    ~Controller() override;
    bool busy() const { return m_busy; }
    bool rendering() const { return m_rendering; }
    double progress() const { return m_progress; }
    QString status() const { return m_status; }
    QString error() const { return m_error; }
    QString workspace() const { return m_workspace; }
    QString python() const { return m_python; }
    QString datasetPath() const { return m_datasetPath; }
    QString modelPath() const { return m_modelPath; }
    QVariantMap dataset() const { return m_dataset.toVariantMap(); }
    QVariantMap evaluation() const { return m_evaluation.toVariantMap(); }
    QVariantMap annotations() const { return m_frame.annotations.toVariantMap(); }
    QVariantMap prediction() const { return m_prediction.toVariantMap(); }
    QVariantMap assets() const { return m_engine->assetSummary().toVariantMap(); }
    int frameVersion() const { return m_version; }
    int sceneIndex() const { return m_scene; }
    double azimuth() const { return m_camera.azimuth; }
    double tilt() const { return m_camera.tilt; }
    double exposure() const { return m_camera.exposure; }
    double span() const { return m_camera.span; }
    double threshold() const { return m_threshold; }
    double lightAzimuth() const { return m_camera.lightAzimuth; }
    double reflection() const { return m_camera.reflection; }
    double blur() const { return m_camera.blur; }
    QVariantMap selection() const { return m_selection.toVariantMap(); }
    QVariantMap cycle() const { return m_cycle.toVariantMap(); }
    QVariantMap comparison() const { return m_comparison.toVariantMap(); }
    QVariantMap stress() const { return m_stress.toVariantMap(); }
    QVariantList cases() const { return m_cases.toVariantList(); }
    QVariantList versions() const { return m_versions.toVariantList(); }
    bool canCollect() const;
    bool robustDataset() const { return m_robustDataset; }
    QStringList labels() const
    {
        return {"Nominal", "Missing component", "Misalignment", "Reversed polarity",
                "Solder bridge"};
    }
    QImage image(const QString &kind = {}) const;
    bool editorMode() const { return m_editorMode; }
    bool inspectionActive() const { return m_inspectionActive; }
    bool inspectionScanning() const { return m_inspectionScanning; }
    bool inspectionStale() const { return m_inspectionStale; }
    bool acceleratedPreview() const { return m_acceleratedPreview; }
    QVariantList components() const { return m_engine->components().toVariantList(); }
    QVariantList edits() const { return m_edits.toVariantList(); }
    QString selectedReference() const { return m_selectedReference; }
    QVariantMap componentEdit() const;
    QVariantMap assemblyReport() const { return m_assemblyReport.toVariantMap(); }
    int inspectionVersion() const { return m_inspectionVersion; }
    std::shared_ptr<slx::Engine> engine() const { return m_engine; }
    slx::Camera previewCamera() const { return m_camera; }
    bool canUndo() const { return !m_busy && !m_inspectionActive && !m_undo.isEmpty(); }
    bool canRedo() const { return !m_busy && !m_inspectionActive && !m_redo.isEmpty(); }
    QString experimentPath() const { return m_experimentPath; }
    QVariantMap inspectorCycle() const { return m_inspectorCycle.toVariantMap(); }
    QVariantList inspectorCases() const { return m_inspectorCases.toVariantList(); }
    bool hasInspectorExample() const;
    Q_INVOKABLE bool loadInspectorExample();
    QString inspectorModelId() const;
    QString inspectorModel() const { return m_inspectorPath; }
    Q_INVOKABLE void beginEdit();
    Q_INVOKABLE void endEdit();
    Q_INVOKABLE void undoEdit();
    Q_INVOKABLE void redoEdit();
    Q_INVOKABLE bool saveExperiment(const QString &name = {});
    Q_INVOKABLE bool loadExperiment(const QUrl &directory);
    Q_INVOKABLE void openExperiment() const;
    Q_INVOKABLE void generateInspector(int groups = 12);
    Q_INVOKABLE void freezeInspector(int groups = 6);
    Q_INVOKABLE void collectInspectorCase();
    Q_INVOKABLE void augmentInspector(int groups = 6);
    Q_INVOKABLE void trainInspector(int epochs = 2000);
    Q_INVOKABLE void activateInspector(bool candidate);
    Q_INVOKABLE void openInspectorReport() const;
    Q_INVOKABLE void enterEditor(bool enabled = true);
    Q_INVOKABLE bool editComponent(const QString &reference, bool removed, double dx, double dy, double rotation);
    Q_INVOKABLE void restoreComponent(const QString &reference);
    Q_INVOKABLE void restoreAssembly();
    Q_INVOKABLE void startInspection();
    Q_INVOKABLE void stopInspection();
    Q_INVOKABLE void beginOrbit();
    Q_INVOKABLE void endOrbit();
    Q_INVOKABLE void pickPreview(double x, double y);
    Q_INVOKABLE void pickCapture(double x, double y);
    void setAcceleratedPreview(bool enabled);
    void setWorkspace(const QString &value);
    void setPython(const QString &value);
    void setSceneIndex(int value);
    void setAzimuth(double value);
    void setTilt(double value);
    void setExposure(double value);
    void setSpan(double value);
    void setThreshold(double value);
    void setLightAzimuth(double value);
    void setReflection(double value);
    void setBlur(double value);
    void setRobustDataset(bool value) { m_robustDataset = value; emit changed(); }
    Q_INVOKABLE void resetCamera();
    Q_INVOKABLE void topCamera();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void generate(int groups, int seed);
    Q_INVOKABLE void train(int epochs);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE bool loadDataset(const QUrl &url);
    Q_INVOKABLE bool loadModel(const QUrl &url);
    Q_INVOKABLE void selectSample(int index);
    Q_INVOKABLE QUrl sampleUrl(const QString &relative) const;
    Q_INVOKABLE void openWorkspace() const;
    Q_INVOKABLE void chooseWorkspace(const QUrl &url);
    Q_INVOKABLE void openDataset() const;
    Q_INVOKABLE void saveFrame(const QUrl &url);
    Q_INVOKABLE void selectReference(const QString &reference);
    Q_INVOKABLE void pick(double x, double y);
    Q_INVOKABLE void freezeBenchmark(int groups = 20);
    Q_INVOKABLE void scanStress();
    Q_INVOKABLE void collectCase();
    Q_INVOKABLE void prepareImprovement(int groups = 45);
    Q_INVOKABLE void trainImprovement(int epochs = 100);
    Q_INVOKABLE void inspectCase(int index);
    Q_INVOKABLE void inspectProbe(int index);
    Q_INVOKABLE QUrl caseUrl(const QString &relative) const;
    Q_INVOKABLE void openComparison() const;
    Q_INVOKABLE void openCycle() const;
  signals:
    void changed();
    void frameChanged();
    void cameraChanged();
    void editorChanged();

  private:
    void applyEdits(const QJsonArray &edits, bool remember = true);
    void initializeInspectorWorkflow();
    void loadInspectorCycle();
    void saveInspectorCycle();
    void inspectorJob(const QString &purpose, int groups);
    void requestRender();
    void scheduleRender();
    void initializeEditor();
    void publish(slx::Frame frame);
    void fail(const QString &message);
    void readTrainingOutput();
    void refreshImages();
    void loadCycle();
    void saveCycle();
    void refreshVersions();
    void startTrainer(int epochs, const QStringList &extra);
    void beginCycleJob(const QString &task);
    void initializeCycle();
    void verifyBenchmark() const;
    void storeCase(const slx::Frame &frame, const QJsonObject &prediction);
    std::shared_ptr<slx::Engine> m_engine;
    std::unique_ptr<slx::Model> m_model;
    QString m_projectRoot, m_workspace, m_python = "python", m_datasetPath, m_modelPath;
    QString m_status = "Ready to explore PowerBoard", m_error, m_stderr;
    bool m_busy = false, m_rendering = false, m_pending = false;
    double m_progress = 0, m_threshold = .75;
    int m_scene = 0, m_version = 0;
    slx::Camera m_camera;
    slx::Frame m_frame;
    QJsonObject m_dataset, m_evaluation, m_prediction;
    QJsonObject m_selection, m_cycle, m_comparison, m_stress;
    QJsonArray m_cases, m_versions;
    QString m_selectedReference, m_cyclePath, m_cycleTask, m_trainedModel, m_source = "development";
    bool m_robustDataset = false;
    mutable QMutex m_imageMutex;
    QImage m_image, m_detail, m_evidence;
    QFutureWatcher<slx::Frame> m_renderWatcher;
    QFutureWatcher<QString> m_datasetWatcher;
    QFutureWatcher<QJsonObject> m_cycleWatcher;
    QTimer m_renderTimer;
    QProcess m_trainer;
    QByteArray m_stdout;
    std::atomic_bool m_cancel{false};
    struct InspectionCapture { slx::Frame frame; QJsonObject report; };
    QFutureWatcher<InspectionCapture> m_inspectionWatcher;
    QJsonArray m_edits;
    QJsonObject m_assemblyReport;
    slx::Frame m_inspectionFrame;
    QImage m_capture, m_captureDetail;
    bool m_editorMode = false, m_inspectionActive = false, m_inspectionScanning = false;
    bool m_inspectionStale = false, m_acceleratedPreview = false, m_orbiting = false;
    bool m_orbitMoved = false;
    int m_inspectionVersion = 0;
    QString m_inspectorPath, m_experimentPath;
    QVector<QJsonArray> m_undo, m_redo;
    bool m_editTransaction = false;
    QJsonArray m_editStart;
    QJsonObject m_inspectorCycle;
    QJsonArray m_inspectorCases;
    QFutureWatcher<QString> m_inspectorDataWatcher;
    QProcess m_inspectorTrainer;
    QString m_inspectorTask, m_inspectorOutput, m_inspectorStderr;
    QByteArray m_inspectorStdout;
};
