// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Stop/edit/acquire workflow; inspection consumes only captured RGB.
#include "controller.h"
#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QException>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>

namespace
{
class InspectionError : public QException
{
public:
    explicit InspectionError(const char *message) : m_message(message) {}
    const char *what() const noexcept override { return m_message.c_str(); }
    void raise() const override { throw *this; }
    InspectionError *clone() const override { return new InspectionError(*this); }
private:
    std::string m_message;
};
}
void Controller::initializeEditor()
{
    m_inspectorPath = m_projectRoot + "/assets/models/assembly-inspector.json";
    initializeInspectorWorkflow();
    connect(&m_inspectionWatcher, &QFutureWatcher<InspectionCapture>::finished, this, [this]
    {
        m_inspectionScanning = false;
        m_busy = false;
        try
        {
            const auto capture = m_inspectionWatcher.result();
            m_inspectionFrame = capture.frame;
            m_assemblyReport = capture.report;
            m_inspectionStale = false;
            { QMutexLocker lock(&m_imageMutex); m_capture = capture.frame.image; }
            ++m_inspectionVersion;
            m_status = QString("Inspection complete: %1 defects, %2 to review. Press Stop to edit the board.")
                .arg(m_assemblyReport["defects"].toInt()).arg(m_assemblyReport["review"].toInt());
            selectReference(m_selectedReference);
        }
        catch (const std::exception &error)
        {
            m_inspectionActive = false;
            fail(QString::fromUtf8(error.what()));
        }
        catch (...)
        {
            m_inspectionActive = false;
            fail("Capture or recognition failed");
        }
        emit editorChanged();
        emit changed();
    });
}
QVariantMap Controller::componentEdit() const
{
    for (const auto &v : m_edits)
        if (v.toObject()["reference"].toString() == m_selectedReference) return v.toObject().toVariantMap();
    return QJsonObject{{"reference", m_selectedReference}, {"removed", false},
                       {"dx_mm", 0}, {"dy_mm", 0}, {"rotation_deg", 0}}.toVariantMap();
}
void Controller::enterEditor(bool enabled)
{
    if (m_inspectionScanning) return;
    if (m_editorMode == enabled) return;
    m_editorMode = enabled;
    m_inspectionActive = false;
    m_source = "development";
    if (enabled) m_status = "Inspection stopped: select a component, edit it and start recognition.";
    m_renderTimer.stop();
    requestRender();
    emit editorChanged();
    emit cameraChanged();
    emit changed();
}
bool Controller::editComponent(const QString &reference, bool removed, double dx, double dy, double rotation)
{
    if (!m_editorMode || m_inspectionActive || m_busy || !std::isfinite(dx) || !std::isfinite(dy) ||
        !std::isfinite(rotation) || std::abs(dx) > 10 || std::abs(dy) > 10 || std::abs(rotation) > 180) return false;
    bool known = false;
    for (const auto &v : m_engine->components()) known |= v.toObject()["reference"].toString() == reference;
    if (!known) return false;
    QJsonArray edits;
    for (const auto &v : m_edits) if (v.toObject()["reference"].toString() != reference) edits.append(v);
    if (removed || dx != 0 || dy != 0 || rotation != 0)
        edits.append(QJsonObject{{"reference", reference}, {"removed", removed},
                                  {"dx_mm", dx}, {"dy_mm", dy}, {"rotation_deg", rotation}});
    if (edits == m_edits) return true;
    applyEdits(edits);
    m_status = "Board edited. Start inspection to capture and recognize anomalies.";
    if (!m_acceleratedPreview) scheduleRender();
    emit editorChanged();
    emit changed();
    return true;
}
void Controller::restoreComponent(const QString &reference)
{
    editComponent(reference, false, 0, 0, 0);
}
void Controller::restoreAssembly()
{
    if (m_inspectionActive || m_busy || !m_editorMode) return;
    applyEdits({});
}
void Controller::startInspection()
{
    if (m_busy || m_inspectionActive || !m_editorMode) return;
    if (!QFile::exists(m_inspectorPath)) { fail("Inspector model missing: " + m_inspectorPath); return; }
    clearError();
    m_busy = m_inspectionActive = m_inspectionScanning = true;
    m_status = "Capturing RGB and comparing with the nominal reference…";
    emit changed();
    emit editorChanged();
    const auto engine = m_engine;
    const auto edits = m_edits;
    const auto model = m_inspectorPath;
    // A repeatable inspection view is independent of the exploration camera.
    const slx::Camera acquisitionCamera;
    m_inspectionWatcher.setFuture(QtConcurrent::run([engine, edits, model, acquisitionCamera]
    {
        try
        {
            auto frame = engine->renderAssembly(edits, acquisitionCamera, QSize(640, 480));
            auto report = engine->inspectAssembly(frame.image, acquisitionCamera, model, .80);
            report.insert("acquired_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            report.insert("acquisition", "Fixed inspection view at 640 × 480; 3D orbit is for exploration");
            // Edit commands are audit metadata, appended after visual inference.
            report.insert("edits_at_capture", edits);
            return InspectionCapture{std::move(frame), std::move(report)};
        }
        catch (const std::exception &error) { throw InspectionError(error.what()); }
    }));
}
void Controller::stopInspection()
{
    if (m_inspectionScanning) return;
    m_inspectionActive = false;
    m_status = "Inspection stopped. Select a component and edit the board.";
    emit editorChanged();
    emit changed();
}
void Controller::setAcceleratedPreview(bool enabled)
{
    m_acceleratedPreview = enabled;
    emit editorChanged();
}
void Controller::scheduleRender()
{
    if (m_orbiting) m_orbitMoved = true;
    if (m_editorMode && m_acceleratedPreview) return;
    if (!m_renderTimer.isActive()) m_renderTimer.start(m_orbiting ? 16 : 35);
}
void Controller::beginOrbit() { m_orbiting = true; m_orbitMoved = false; }
void Controller::endOrbit()
{
    m_orbiting = false;
    if (!m_orbitMoved) return;
    if (!m_editorMode || !m_acceleratedPreview)
    {
        m_renderTimer.stop();
        requestRender();
    }
}
void Controller::pickPreview(double x, double y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) return;
    selectReference(m_engine->pickAssembly(m_edits, m_camera, QSize(960, 720), QPointF(x, y)));
}
void Controller::pickCapture(double x, double y)
{
    if (m_inspectionStale) return;
    for (const auto &v : m_assemblyReport["results"].toArray())
    {
        const auto row = v.toObject(); const auto b = row["bbox"].toArray();
        if (QRectF(b[0].toDouble(), b[1].toDouble(), b[2].toDouble(), b[3].toDouble()).contains(x, y))
        { selectReference(row["reference"].toString()); return; }
    }
}

void Controller::applyEdits(const QJsonArray &edits, bool remember)
{
    if (edits == m_edits) return;
    if (remember && !m_editTransaction)
    {
        m_undo.append(m_edits);
        if (m_undo.size() > 100) m_undo.removeFirst();
        m_redo.clear();
    }
    m_edits = edits;
    m_inspectionStale = !m_assemblyReport.isEmpty() &&
        edits != m_assemblyReport["edits_at_capture"].toArray();
    if (!m_assemblyReport.isEmpty())
    {
        QFile model(m_inspectorPath);
        if (!model.open(QIODevice::ReadOnly) || QString::fromLatin1(QCryptographicHash::hash(model.readAll(), QCryptographicHash::Sha256).toHex()) != m_assemblyReport["model_sha256"].toString())
            m_inspectionStale = true;
    }
    if (!m_acceleratedPreview) scheduleRender();
    emit editorChanged();
}
void Controller::beginEdit()
{
    if (m_busy || m_inspectionActive || m_editTransaction) return;
    m_editStart = m_edits;
    m_editTransaction = true;
}
void Controller::endEdit()
{
    if (!m_editTransaction) return;
    m_editTransaction = false;
    if (m_editStart != m_edits)
    {
        m_undo.append(m_editStart);
        if (m_undo.size() > 100) m_undo.removeFirst();
        m_redo.clear();
    }
    emit editorChanged();
}
void Controller::undoEdit()
{
    if (!m_editorMode || !canUndo() || m_editTransaction) return;
    m_redo.append(m_edits);
    applyEdits(m_undo.takeLast(), false);
}
void Controller::redoEdit()
{
    if (!m_editorMode || !canRedo() || m_editTransaction) return;
    m_undo.append(m_edits);
    applyEdits(m_redo.takeLast(), false);
}
