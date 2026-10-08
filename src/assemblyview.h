// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.
/// @file
/// @brief GPU preview of the editable board; synchronized immutable scene state.
#pragma once
#include "controller.h"
#include <QQuickFramebufferObject>

struct PreviewPart
{
    QString reference;
    slx::MeshData mesh;
};
struct PreviewStats
{
    std::atomic<quint64> frames{0}, gpuNanoseconds{0};
};
class AssemblyView : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(Controller *source READ source WRITE setSource NOTIFY sourceChanged)
public:
    explicit AssemblyView(QQuickItem *parent = nullptr);
    Renderer *createRenderer() const override;
    Controller *source() const { return m_source; }
    void setSource(Controller *source);
    std::shared_ptr<const QVector<PreviewPart>> parts() const { return m_parts; }
    QVariantMap renderStats() const;
signals:
    void sourceChanged();
private:
    Controller *m_source = nullptr;
    std::shared_ptr<const QVector<PreviewPart>> m_parts;
    std::shared_ptr<PreviewStats> m_stats = std::make_shared<PreviewStats>();
};
