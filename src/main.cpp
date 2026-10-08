// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Desktop entry point and reproducible command-line rendering probes.
#include "controller.h"
#include "assemblyview.h"
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QStandardPaths>
#include <QTimer>
#include <iostream>
#include <cmath>
#include <stdexcept>

class FrameProvider : public QQuickImageProvider
{
  public:
    explicit FrameProvider(Controller *controller)
        : QQuickImageProvider(Image), m_controller(controller)
    {
    }
    QImage requestImage(const QString &id, QSize *size, const QSize &) override
    {
        auto result = m_controller->image(id.section('/', 0, 0));
        if (size)
            *size = result.size();
        return result;
    }

  private:
    Controller *m_controller;
};
int main(int argc, char **argv)
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    for (int i = 1; i < argc; ++i)
        if (QByteArray(argv[i]) == "--orbit-benchmark") format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Silicon LogiX");
    app.setApplicationName("SLX SynthForge");
    app.setApplicationVersion(SLX_APP_VERSION);
    app.setFont(QFont("Segoe UI"));
    app.setWindowIcon(QIcon(":/brand/synthforge-icon.svg"));
    QQuickStyle::setStyle("Basic");
    const auto args = app.arguments();
    bool accelerated = false;
    if (!args.contains("--software-ui"))
    {
        QOpenGLContext probe;
        probe.setFormat(format);
        QOffscreenSurface surface;
        surface.setFormat(format);
        surface.create();
        accelerated = probe.create() && probe.makeCurrent(&surface) &&
                      (probe.format().majorVersion() > 3 ||
                       (probe.format().majorVersion() == 3 && probe.format().minorVersion() >= 3));
        probe.doneCurrent();
    }
    QQuickWindow::setGraphicsApi(accelerated ? QSGRendererInterface::OpenGL : QSGRendererInterface::Software);
    auto option = [&](const QString &key, const QString &fallback)
    {
        int index = args.indexOf(key);
        return index >= 0 && index + 1 < args.size() ? args[index + 1] : fallback;
    };
    if (args.contains("--diagnostics"))
    {
        qputenv("SLX_DIAGNOSTICS", option("--diagnostics", "synthforge.log").toUtf8());
        qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message)
        {
            QFile log(QString::fromUtf8(qgetenv("SLX_DIAGNOSTICS")));
            if (log.open(QIODevice::WriteOnly | QIODevice::Append)) log.write(message.toUtf8() + '\n');
        });
    }
    QString project = QCoreApplication::applicationDirPath();
    if (!QFile::exists(project + "/tools/train.py"))
    {
        if (QFile::exists(QDir::currentPath() + "/tools/train.py"))
            project = QDir::currentPath();
        else if (QFile::exists(project + "/../../tools/train.py"))
            project = QDir(project + "/../..").absolutePath();
    }
    const QString assetRoot = option("--assets", project + "/SLX_SynthForge_PowerBoard");
    const QString workspace =
        option("--workspace", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
                                  "/Silicon LogiX/SynthForge");
    try
    {
        if (args.contains("--generate") || args.contains("--render") || args.contains("--generate-inspector"))
        {
            slx::Engine engine(assetRoot);
            if (args.contains("--generate-inspector"))
            {
                std::atomic_bool cancel{false};
                const auto path = engine.generateInspectorData(workspace, option("--generate-inspector", "10").toInt(),
                    option("--seed", "7831").toUInt(), [](int done, int total)
                    { if (done % 100 == 0) std::cout << done << "/" << total << std::endl; }, cancel);
                std::cout << path.toStdString() << std::endl;
                return 0;
            }
            if (args.contains("--generate"))
            {
                slx::DatasetOptions o;
                o.groups = option("--generate", "60").toInt();
                o.seed = option("--seed", "2026").toUInt();
                o.profile = option("--profile", "nominal");
                o.purpose = option("--purpose", "training");
                std::atomic_bool cancel{false};
                auto path = engine.generate(
                    workspace, o,
                    [](int done, int total)
                    {
                        if (done % 25 == 0)
                            std::cout << done << "/" << total << std::endl;
                    },
                    cancel);
                std::cout << path.toStdString() << std::endl;
                return 0;
            }
            slx::Camera c;
            c.tilt = option("--tilt", "32").toDouble();
            c.azimuth = option("--azimuth", "-65").toDouble();
            c.exposure = option("--exposure", "1").toDouble();
            c.lightAzimuth = option("--light", "-35").toDouble();
            c.reflection = option("--reflection", "1").toDouble();
            c.blur = option("--blur", "0").toDouble();
            auto frame = engine.render(option("--render", "0").toInt(), c);
            const auto output = option("--output", workspace + "/render.png");
            QDir().mkpath(QFileInfo(output).absolutePath());
            if (!frame.image.save(output) || !frame.instanceMask.save(output + ".mask.png"))
                return 3;
            QFile labels(output + ".json");
            if (labels.open(QIODevice::WriteOnly))
                labels.write(QJsonDocument(frame.annotations).toJson());
            if (args.contains("--model"))
            {
                slx::Model model(option("--model", ""));
                std::cout << QJsonDocument(model.predict(frame, .75)).toJson().toStdString();
            }
            return 0;
        }
        Controller controller(assetRoot, project, workspace);
        controller.setAcceleratedPreview(accelerated);
        if (args.contains("--dataset"))
            controller.loadDataset(QUrl::fromLocalFile(option("--dataset", "")));
        else if (QFile::exists(project + "/examples/demo-dataset/dataset.json"))
            controller.loadDataset(QUrl::fromLocalFile(project + "/examples/demo-dataset"));
        if (args.contains("--model"))
            controller.loadModel(QUrl::fromLocalFile(option("--model", "")));
        if (args.contains("--scene"))
            controller.setSceneIndex(option("--scene", "0").toInt());
        if (args.contains("--tilt"))
            controller.setTilt(option("--tilt", "32").toDouble());
        if (args.contains("--blur")) controller.setBlur(option("--blur", "0").toDouble());
        if (args.contains("--reflection")) controller.setReflection(option("--reflection", "1").toDouble());
        if (args.contains("--exposure")) controller.setExposure(option("--exposure", "1").toDouble());
        if (args.contains("--reference")) controller.selectReference(option("--reference", "U4"));
        if (args.contains("--probe")) controller.inspectProbe(option("--probe", "0").toInt());
        if (args.contains("--exercise-cycle"))
        {
            if (controller.modelPath().isEmpty()) throw std::runtime_error("Load a trained base dataset for --exercise-cycle");
            QTimer steps;
            steps.setInterval(100);
            int step = 0;
            QObject::connect(&steps, &QTimer::timeout, &app, [&]
            {
                if (controller.busy()) return;
                if (!controller.error().isEmpty())
                {
                    std::cerr << controller.error().toStdString() << std::endl;
                    app.exit(2);
                    return;
                }
                if (step == 0) controller.freezeBenchmark(20);
                if (step == 1) controller.scanStress();
                if (step == 2) controller.prepareImprovement(45);
                if (step == 3) controller.trainImprovement(100);
                if (step == 4)
                {
                    auto report = QJsonObject::fromVariantMap(controller.comparison());
                    report.remove("predictions");
                    std::cout << QJsonDocument(report).toJson().toStdString();
                    app.exit(report.isEmpty() ? 3 : 0);
                }
                ++step;
            });
            steps.start();
            return app.exec();
        }
        if (!args.contains("--scene") && !args.contains("--probe") && option("--tab", "0") == "0")
            controller.enterEditor();
        if (args.contains("--edit-remove"))
            controller.editComponent(option("--edit-remove", "C1"), true, 0, 0, 0);
        if (args.contains("--edit-move"))
            controller.editComponent(option("--edit-move", "U4"), false, option("--dx", "3").toDouble(),
                                     option("--dy", "0").toDouble(), option("--rotation", "0").toDouble());
        if (args.contains("--inspect-assembly")) controller.startInspection();
        if (args.contains("--assembly-report"))
        {
            auto reportTimer = new QTimer(&app);
            reportTimer->setInterval(100);
            QObject::connect(reportTimer, &QTimer::timeout, &app, [&, reportTimer]
            {
                if (controller.inspectionScanning()) return;
                reportTimer->stop();
                const auto report = QJsonObject::fromVariantMap(controller.assemblyReport());
                QFile output(option("--assembly-report", workspace + "/inspection.json"));
                if (!output.open(QIODevice::WriteOnly) || report.isEmpty()) { app.exit(3); return; }
                output.write(QJsonDocument(report).toJson());
                if (!args.contains("--snapshot")) app.quit();
            });
            reportTimer->start();
        }
        if (args.contains("--orbit-benchmark")) qputenv("SLX_PREVIEW_BENCHMARK", "1");
        if (args.contains("--experiment")) controller.loadExperiment(QUrl::fromLocalFile(option("--experiment", "")));
        if (args.contains("--inspector-example")) controller.loadInspectorExample();
        qmlRegisterType<AssemblyView>("SiliconLogiX.SynthForge", 1, 0, "AssemblyView");
        QQmlApplicationEngine qml;
        qml.addImageProvider("frame", new FrameProvider(&controller));
        qml.rootContext()->setContextProperty("forge", &controller);
        qml.rootContext()->setContextProperty("appVersion", app.applicationVersion());
        qml.rootContext()->setContextProperty("coreIdentity", QString::fromUtf8(SiliconLogiX_SynthForge_CoreIdentity()));
        qml.rootContext()->setContextProperty("initialModelEvidence", args.contains("--evidence"));
        for (const auto &entry :
             QList<QPair<QString, QString>>{{"licenseText", ":/legal/LICENSE"},
                                            {"noticesText", ":/legal/notices"},
                                            {"qtLicenseText", ":/legal/qt-license"}})
        {
            QFile f(entry.second);
            if (!f.open(QIODevice::ReadOnly))
                return 1;
            qml.rootContext()->setContextProperty(entry.first, QString::fromUtf8(f.readAll()));
        }
        qml.load(QUrl("qrc:/qml/Main.qml"));
        if (qml.rootObjects().isEmpty())
            return 1;
        if (args.contains("--orbit-benchmark"))
        {
            auto window = qobject_cast<QQuickWindow *>(qml.rootObjects().first());
            QElapsedTimer elapsed;
            int frames = 0, steps = 0;
            qint64 drawTime = 0;
            QTimer motion;
            motion.setInterval(16);
            QObject::connect(&motion, &QTimer::timeout, &app, [&]
            {
                controller.setAzimuth(-100 + (++steps % 240) * .6);
                controller.setTilt(30 + 8 * std::sin(steps * .05));
                QElapsedTimer draw;
                draw.start();
                frames += !window->grabWindow().isNull();
                drawTime += draw.nsecsElapsed();
                if (steps >= 240)
                {
                    motion.stop();
                    controller.endOrbit();
                    std::cout << "{\"gpu\":" << (accelerated ? "true" : "false") << ",\"frames\":" << frames
                              << ",\"seconds\":" << elapsed.elapsed() / 1000. << ",\"input_steps\":" << steps
                              << ",\"mean_window_capture_ms\":" << drawTime / 1e6 / std::max(1, frames) << "}" << std::endl;
                    if (const auto preview = window->findChild<AssemblyView *>())
                        std::cout << QJsonDocument(QJsonObject::fromVariantMap(preview->renderStats())).toJson(QJsonDocument::Compact).toStdString() << std::endl;
                    app.quit();
                }
            });
            QTimer::singleShot(1500, &app, [&] { frames = 0; controller.beginOrbit(); elapsed.start(); motion.start(); });
            return app.exec();
        }
        if (args.contains("--demo-inspector-cycle"))
        {
            int stage = 0;
            QTimer demo;
            demo.setInterval(100);
            QObject::connect(&demo, &QTimer::timeout, &app, [&]
            {
                if (controller.busy()) return;
                if (!controller.error().isEmpty()) { app.exit(4); return; }
                switch (stage++)
                {
                case 0: controller.stopInspection(); controller.freezeInspector(6); break;
                case 1:
                    controller.enterEditor(true); controller.selectReference("U4");
                    controller.editComponent("U4", false, 1.7, .4, 17);
                    controller.startInspection(); break;
                case 2: controller.collectInspectorCase(); controller.stopInspection(); controller.augmentInspector(6); break;
                case 3: controller.trainInspector(2000); break;
                case 4: controller.activateInspector(true); controller.startInspection(); break;
                case 5: app.exit(controller.saveExperiment("Editable-board improvement validation") ? 0 : 4); break;
                }
            });
            demo.start();
            return app.exec();
        }
        if (args.contains("--smoke-test"))
        {
            QTimer::singleShot(300, &app, &QCoreApplication::quit);
            return app.exec();
        }
        if (args.contains("--snapshot"))
        {
            auto window = qobject_cast<QQuickWindow *>(qml.rootObjects().first());
            if (!window)
                return 4;
            window->setProperty("currentTab", option("--tab", "0").toInt());
            if (args.contains("--guided"))
                if (auto studio = window->findChild<QObject *>("interactiveStudio")) studio->setProperty("guided", true);
            if (args.contains("--compact"))
                window->resize(1120, 740);
            if (args.contains("--about"))
                QMetaObject::invokeMethod(window, "showAbout");
            const auto path = option("--snapshot", workspace + "/snapshot.png");
            auto captureTimer = new QTimer(&app);
            captureTimer->setInterval(250);
            auto attempts = std::make_shared<int>(0);
            QObject::connect(captureTimer, &QTimer::timeout, &app, [&, window, path, attempts]
            {
                if (++*attempts < 7 || controller.inspectionScanning()) return;
                if (args.contains("--assembly-report"))
                {
                    QFile report(option("--assembly-report", workspace + "/inspection.json"));
                    if (!report.open(QIODevice::WriteOnly)) { app.exit(3); return; }
                    report.write(QJsonDocument(QJsonObject::fromVariantMap(controller.assemblyReport())).toJson());
                }
                QDir().mkpath(QFileInfo(path).absolutePath());
                app.exit(window->grabWindow().save(path) ? 0 : 3);
            });
            captureTimer->start();
        }
        return app.exec();
    }
    catch (const std::exception &e)
    {
        std::cerr << "SLX SynthForge: " << e.what() << std::endl;
        return 2;
    }
}
