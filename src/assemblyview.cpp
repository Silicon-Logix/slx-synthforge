// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief OpenGL exploration preview, independent of CPU inspection capture.
#include "assemblyview.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QQuickOpenGLUtils>
#include <QMatrix4x4>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace
{
class AssemblyRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_3_3_Core
{
public:
    explicit AssemblyRenderer(std::shared_ptr<PreviewStats> stats) : m_stats(std::move(stats)) {}
    ~AssemblyRenderer() override
    {
        if (m_initialized)
        {
            for (auto &part : m_gpu) { glDeleteBuffers(1, &part.vbo); glDeleteVertexArrays(1, &part.vao); }
            glDeleteBuffers(1, &m_wire.vbo); glDeleteVertexArrays(1, &m_wire.vao);
            if (m_query) glDeleteQueries(1, &m_query);
        }
    }
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }
    void synchronize(QQuickFramebufferObject *item) override
    {
        const auto view = static_cast<AssemblyView *>(item);
        m_parts = view->parts();
        if (!view->source()) return;
        m_camera = view->source()->previewCamera();
        m_selected = view->source()->selectedReference();
        m_edits.clear();
        for (const auto &v : view->source()->edits())
        {
            const auto edit = v.toMap();
            m_edits.insert(edit["reference"].toString(), edit);
        }
    }
    void render() override
    {
        if (!m_parts) return;
        if (!m_initialized)
        {
            if (!initializeOpenGLFunctions()) return;
            m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
                #version 330 core
                layout(location=0) in vec3 position;
                layout(location=1) in vec3 normal;
                layout(location=2) in vec3 color;
                uniform mat4 mvp;
                uniform mat4 model;
                out vec3 n;
                out vec3 rgb;
                void main() { gl_Position=mvp*vec4(position,1); n=mat3(model)*normal; rgb=color; }
            )");
            m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
                #version 330 core
                in vec3 n;
                in vec3 rgb;
                uniform vec3 light;
                uniform float selected;
                uniform float exposure;
                out vec4 outputColor;
                void main() {
                    float diffuse=max(0.0,dot(normalize(n),light));
                    vec3 c=pow(clamp(rgb*(0.50+0.50*diffuse)*exposure,0.0,1.0),vec3(1.0/2.2));
                    c=mix(c,vec3(0.27,0.89,0.83),selected*0.48);
                    outputColor=vec4(c,1);
                }
            )");
            if (!m_program.link()) { qWarning() << "Assembly preview shader:" << m_program.log(); return; }
            for (const auto &part : *m_parts)
            {
                GpuPart gpu;
                glGenVertexArrays(1, &gpu.vao); glBindVertexArray(gpu.vao);
                glGenBuffers(1, &gpu.vbo); glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
                glBufferData(GL_ARRAY_BUFFER, part.mesh.vertices.size(), part.mesh.vertices.constData(), GL_STATIC_DRAW);
                for (int attribute = 0; attribute < 3; ++attribute)
                {
                    glEnableVertexAttribArray(attribute);
                    glVertexAttribPointer(attribute, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                                          reinterpret_cast<void *>(attribute * 3 * sizeof(float)));
                }
                gpu.count = part.mesh.vertices.size() / (9 * sizeof(float));
                m_gpu.append(gpu);
            }
            m_initialized = true;
            if (qEnvironmentVariableIsSet("SLX_PREVIEW_BENCHMARK")) glGenQueries(1, &m_query);
            glGenVertexArrays(1, &m_wire.vao); glBindVertexArray(m_wire.vao);
            glGenBuffers(1, &m_wire.vbo); glBindBuffer(GL_ARRAY_BUFFER, m_wire.vbo);
            glBufferData(GL_ARRAY_BUFFER, 24 * 9 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
            for (int attribute = 0; attribute < 3; ++attribute)
            {
                glEnableVertexAttribArray(attribute);
                glVertexAttribPointer(attribute, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                                      reinterpret_cast<void *>(attribute * 3 * sizeof(float)));
            }
        }
        if (m_query) glBeginQuery(GL_TIME_ELAPSED, m_query);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE); glDisable(GL_BLEND); glDisable(GL_SCISSOR_TEST);
        glClearColor(.067f, .094f, .196f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        const float az = qDegreesToRadians(float(m_camera.azimuth)), tilt = qDegreesToRadians(float(m_camera.tilt));
        const QVector3D direction(std::cos(az) * std::sin(tilt), std::sin(az) * std::sin(tilt), std::cos(tilt));
        const QVector3D right(-std::sin(az), std::cos(az), 0);
        const QVector3D up = QVector3D::crossProduct(direction, right);
        const QVector3D center(0, 0, 3);
        QMatrix4x4 view; view.lookAt(center + direction * 200, center, up);
        const float half = m_camera.span / 2;
        const float vertical = half * framebufferObject()->height() / framebufferObject()->width();
        QMatrix4x4 projection; projection.ortho(-half, half, -vertical, vertical, 1, 500);
        m_program.bind();
        const float light = qDegreesToRadians(float(m_camera.lightAzimuth));
        m_program.setUniformValue("light", QVector3D(std::cos(light) * .5f, std::sin(light) * .5f, 1).normalized());
        m_program.setUniformValue("exposure", float(m_camera.exposure));
        for (int i = 0; i < m_parts->size(); ++i)
        {
            const auto &part = (*m_parts)[i];
            const auto edit = m_edits.value(part.reference);
            if (edit["removed"].toBool())
            {
                if (part.reference == m_selected)
                {
                    // Display the expected CAD envelope for an absent component.
                    const int edges[24] = {0,1,0,2,0,4,1,3,1,5,2,3,2,6,3,7,4,5,4,6,5,7,6,7};
                    float vertices[24 * 9];
                    for (int vertex = 0; vertex < 24; ++vertex)
                    {
                        const int corner = edges[vertex];
                        for (int axis = 0; axis < 3; ++axis)
                            vertices[vertex * 9 + axis] = corner & (1 << axis) ? part.mesh.maximum[axis] : part.mesh.minimum[axis];
                        vertices[vertex * 9 + 3] = 0; vertices[vertex * 9 + 4] = 0; vertices[vertex * 9 + 5] = 1;
                        vertices[vertex * 9 + 6] = .1f; vertices[vertex * 9 + 7] = .8f; vertices[vertex * 9 + 8] = .7f;
                    }
                    QMatrix4x4 model; model.translate(part.mesh.pivot);
                    m_program.setUniformValue("model", model);
                    m_program.setUniformValue("mvp", projection * view * model);
                    m_program.setUniformValue("selected", 1.f);
                    glBindVertexArray(m_wire.vao); glBindBuffer(GL_ARRAY_BUFFER, m_wire.vbo);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                    glDisable(GL_DEPTH_TEST);
                    glDrawArrays(GL_LINES, 0, 24);
                    glEnable(GL_DEPTH_TEST);
                }
                continue;
            }
            QMatrix4x4 model;
            model.translate(part.mesh.pivot + QVector3D(edit["dx_mm"].toFloat(), edit["dy_mm"].toFloat(), 0));
            model.rotate(edit["rotation_deg"].toFloat(), 0, 0, 1);
            m_program.setUniformValue("model", model);
            m_program.setUniformValue("mvp", projection * view * model);
            m_program.setUniformValue("selected", part.reference == m_selected ? 1.f : 0.f);
            glBindVertexArray(m_gpu[i].vao);
            glDrawArrays(GL_TRIANGLES, 0, m_gpu[i].count);
        }
        glBindVertexArray(0); glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_program.release();
        if (m_query)
        {
            glEndQuery(GL_TIME_ELAPSED);
            GLuint64 duration = 0;
            glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, &duration);
            m_stats->gpuNanoseconds += duration;
            ++m_stats->frames;
        }
        QQuickOpenGLUtils::resetOpenGLState();
    }
private:
    struct GpuPart { GLuint vao = 0, vbo = 0; int count = 0; };
    QVector<GpuPart> m_gpu;
    GpuPart m_wire;
    GLuint m_query = 0;
    std::shared_ptr<PreviewStats> m_stats;
    std::shared_ptr<const QVector<PreviewPart>> m_parts;
    QHash<QString, QVariantMap> m_edits;
    QOpenGLShaderProgram m_program;
    slx::Camera m_camera;
    QString m_selected;
    bool m_initialized = false;
};
}
AssemblyView::AssemblyView(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);
}
QQuickFramebufferObject::Renderer *AssemblyView::createRenderer() const { return new AssemblyRenderer(m_stats); }
QVariantMap AssemblyView::renderStats() const
{
    const auto frames = m_stats->frames.load();
    return {{"measured_gpu_frames", qulonglong(frames)},
            {"mean_gpu_draw_ms", m_stats->gpuNanoseconds.load() / 1e6 / std::max<quint64>(1, frames)}};
}
void AssemblyView::setSource(Controller *source)
{
    if (m_source == source) return;
    if (m_source) disconnect(m_source, nullptr, this, nullptr);
    m_source = source;
    auto parts = std::make_shared<QVector<PreviewPart>>();
    if (source)
    {
        parts->append({"PCB", source->engine()->mesh("PCB")});
        for (const auto &v : source->engine()->components())
        {
            const auto reference = v.toObject()["reference"].toString();
            parts->append({reference, source->engine()->mesh(reference)});
        }
        connect(source, &Controller::cameraChanged, this, &AssemblyView::update);
        connect(source, &Controller::editorChanged, this, &AssemblyView::update);
    }
    m_parts = parts;
    emit sourceChanged();
    update();
}
