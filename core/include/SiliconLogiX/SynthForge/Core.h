// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Versioned SDK boundary for scene rendering, annotation and inference.
#pragma once
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QRect>
#include <QVector3D>
#include <QStringList>
#include <atomic>
#include <functional>
#include <memory>

#if defined(SLX_CORE_BUILD)
#define SLX_API Q_DECL_EXPORT
#else
#define SLX_API Q_DECL_IMPORT
#endif

namespace SiliconLogiX::SynthForge
{
constexpr int CoreAbiVersion = 4;
constexpr int InspectorFeatureCount = 16;
constexpr int FeatureSide = 24;
constexpr int FeatureCount = FeatureSide * FeatureSide * 3 * 2;
/// @brief Orthographic CAD camera: angles in degrees, span in millimetres, blur in pixels.
struct Camera
{
    double azimuth = -65;
    double tilt = 32;
    double span = 133;
    double lightAzimuth = -35;
    double exposure = 1.0;
    double reflection = 1.0;
    double blur = 0.0;
    quint32 noiseSeed = 0;
};
struct Frame
{
    QImage image;
    QImage instanceMask;
    QJsonObject annotations;
    QVector<float> features;
};
struct MeshData
{
    QByteArray vertices; // xyz, normal xyz, linear RGB; nine float32 values.
    QVector3D minimum, maximum, pivot;
    int instance = 0;
};
/// Training and benchmark groups use independent seeds; augmentation has no test rows.
struct InspectorDataOptions
{
    int groups = 12;
    quint32 seed = 2026;
    QString purpose = "training";
    QJsonArray focusEdits;
};
struct DatasetOptions
{
    int groups = 60;
    quint32 seed = 2026;
    QSize resolution = {960, 720};
    QString profile = "nominal";
    QString purpose = "training";
    QJsonArray focusRecipes;
};
/// @brief Immutable asset bank, safe for concurrent renders after construction.
class SLX_API Engine
{
  public:
    explicit Engine(const QString &assetDirectory);
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    QStringList scenarios() const;
    QJsonObject assetSummary() const;
    Frame render(int scenario, const Camera &camera, QSize size = {960, 720}) const;
    /// @brief Render free edits on nominal CAD. Each edit names a mounted reference,
    /// removed, dx_mm/dy_mm (±10 mm), rotation_deg (±180°) about its CAD pivot.
    Frame renderAssembly(const QJsonArray &edits, const Camera &camera,
                         QSize size = {960, 720}) const;
    QJsonArray components() const;
    MeshData mesh(const QString &reference, int scenario = 0) const;
    QString pickAssembly(const QJsonArray &edits, const Camera &camera, QSize size,
                         QPointF pixel) const;
    /// @brief Infer from captured RGB and the registered camera; edit commands,
    /// edited masks and annotations are not accepted as recognition inputs.
    /// The supplied model is validated against the asset bank; scores are uncalibrated.
    QJsonObject inspectAssembly(const QImage &acquisition, const Camera &camera,
                                const QString &modelPath, double threshold = .80) const;
    QString generateInspectorData(const QString &directory, int groups, quint32 seed,
                                  const std::function<void(int, int)> &progress,
                                  const std::atomic_bool &cancel) const;
    /// @brief Generate synthetic inspector examples in a new immutable run directory.
    /// training separates capture groups; benchmark is test-only; augmentation uses
    /// confirmed development focusEdits and never adds test rows. Callback runs on
    /// the calling thread. Cancellation leaves the partial run marked incomplete.
    QString generateInspectorData(const QString &directory, const InspectorDataOptions &options,
                                  const std::function<void(int, int)> &progress,
                                  const std::atomic_bool &cancel) const;
    QString referenceAt(const Frame &frame, QPoint pixel) const;
    QJsonObject inspect(const Frame &frame, const QString &reference) const;
    QImage highlight(const Frame &frame, const QString &reference) const;
    QString generate(const QString &parentDirectory, const DatasetOptions &options,
                     const std::function<void(int, int)> &progress,
                     const std::atomic_bool &cancel) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
/// @brief Loads only the supported visual softmax model format; no executable payload.
class SLX_API Model
{
  public:
    explicit Model(const QString &path);
    ~Model();
    QJsonObject predict(const Frame &frame, double threshold) const;
    QImage evidence(const Frame &frame, int classIndex) const;
    QString datasetId() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
SLX_API QStringList classNames();
SLX_API QString coreVersion();
} // namespace SiliconLogiX::SynthForge

/// Stable vendor identity for deployment auditing; this is not a licensing gate.
extern "C" SLX_API const char *SiliconLogiX_SynthForge_CoreIdentity();
