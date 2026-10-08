// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Exercises cancellation, local Python training, import and sample inspection.
#include "controller.h"
#include <QDir>
#include <QCryptographicHash>
#include <QDataStream>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>
class WorkflowTests : public QObject
{
    Q_OBJECT
  private slots:
    void includedInspectorExample()
    {
        QTemporaryDir temp;
        Controller c(QStringLiteral(SLX_ASSET_DIR), QStringLiteral(SLX_PROJECT_DIR), temp.path());
        QVERIFY(c.hasInspectorExample());
        const auto active = c.inspectorModel();
        QVERIFY(c.loadInspectorExample());
        QCOMPARE(c.inspectorModel(), active);
        QCOMPARE(c.inspectorCycle()["comparison"].toMap()["baseline"].toMap()["samples"].toInt(), 1076);
        QCOMPARE(c.inspectorCases().size(), 1);
        c.activateInspector(true); QVERIFY(c.inspectorModel() != active);
        c.enterEditor(); c.setAcceleratedPreview(true); c.startInspection();
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QCOMPARE(c.assemblyReport()["components"].toInt(), 46);
        const auto candidate = c.inspectorModel();
        c.stopInspection();
        QTemporaryDir other;
        c.setWorkspace(other.path());
        QCOMPARE(c.inspectorModel(), active);
        QVERIFY(c.inspectorCases().isEmpty());
        QVERIFY(!c.inspectorCycle()["dataset"].toString().isEmpty());
        QVERIFY(c.inspectionStale());
        c.setWorkspace(temp.path());
        QCOMPARE(c.inspectorModel(), candidate);
        QCOMPARE(c.inspectorCases().size(), 1);
        QVERIFY(!c.inspectionStale());
    }
    void experimentAndHistory()
    {
        QTemporaryDir temp;
        Controller c(QStringLiteral(SLX_ASSET_DIR), QStringLiteral(SLX_PROJECT_DIR), temp.path());
        c.enterEditor(); c.setAcceleratedPreview(true);
        c.beginEdit();
        QVERIFY(c.editComponent("U4", false, 1, 0, 0));
        QVERIFY(c.editComponent("U4", false, 3.7, -1.1, 23));
        c.endEdit();
        c.undoEdit(); QCOMPARE(c.edits().size(), 0);
        c.redoEdit(); QCOMPARE(c.edits().size(), 1);
        c.startInspection();
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QVERIFY(c.saveExperiment("Portable audit"));
        const auto path = c.experimentPath();
        QVERIFY(QFile::exists(path + "/report.html"));
        const auto original = c.assemblyReport();
        c.stopInspection(); c.restoreAssembly();
        QVERIFY(c.inspectionStale());
        c.undoEdit(); QVERIFY(!c.inspectionStale());
        c.restoreAssembly();
        QVERIFY(c.loadExperiment(QUrl::fromLocalFile(path)));
        QCOMPARE(c.edits().size(), 1); QCOMPARE(c.assemblyReport(), original);
        QVERIFY(!c.inspectionStale());
        Controller reopened(QStringLiteral(SLX_ASSET_DIR), QStringLiteral(SLX_PROJECT_DIR), temp.path());
        reopened.setAcceleratedPreview(true);
        QVERIFY(reopened.loadExperiment(QUrl::fromLocalFile(path)));
        reopened.selectReference("U4");
        QVERIFY(!reopened.image("capture-detail").isNull());
        auto rewriteReport = [&](const QJsonObject &report)
        {
            const auto bytes = QJsonDocument(report).toJson();
            QFile inspection(path + "/inspection.json");
            QVERIFY(inspection.open(QIODevice::WriteOnly));
            QCOMPARE(inspection.write(bytes), bytes.size()); inspection.close();
            QFile file(path + "/experiment.json"); QVERIFY(file.open(QIODevice::ReadOnly));
            auto manifest = QJsonDocument::fromJson(file.readAll()).object(); file.close();
            auto files = manifest["files"].toObject();
            files["inspection.json"] = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            manifest["files"] = files;
            QVERIFY(file.open(QIODevice::WriteOnly));
            const auto encoded = QJsonDocument(manifest).toJson();
            QCOMPARE(file.write(encoded), encoded.size());
        };
        auto historical = QJsonObject::fromVariantMap(original);
        auto rows = historical["results"].toArray();
        for (int i = 0; i < rows.size(); ++i)
        {
            auto row = rows[i].toObject();
            row["class_name"] = "Legacy display label";
            row["note"] = "Legacy explanatory text";
            rows[i] = row;
        }
        historical["results"] = rows;
        rewriteReport(historical);
        QVERIFY(c.loadExperiment(QUrl::fromLocalFile(path)));
        QCOMPARE(c.assemblyReport(), original);
        // Recomputing checksums cannot make falsified inference scores valid.
        auto tampered = rows[0].toObject(); tampered["confidence"] = .123;
        rows[0] = tampered; historical["results"] = rows;
        rewriteReport(historical);
        QVERIFY(!c.loadExperiment(QUrl::fromLocalFile(path)));
        QCOMPARE(c.edits().size(), 1);
        historical = QJsonObject::fromVariantMap(original);
        historical["threshold"] = .1;
        rewriteReport(historical);
        QVERIFY(!c.loadExperiment(QUrl::fromLocalFile(path)));
        QCOMPARE(c.edits().size(), 1);
        rewriteReport(QJsonObject::fromVariantMap(original));
        // Corrupted pixels are rejected before the live scene is changed.
        QFile damaged(path + "/capture.png"); QVERIFY(damaged.open(QIODevice::Append));
        damaged.write("tampered"); damaged.close();
        QVERIFY(!c.loadExperiment(QUrl::fromLocalFile(path)));
        QCOMPARE(c.edits().size(), 1);
    }
    void inspectorImprovement()
    {
        QTemporaryDir temp;
        Controller c(QStringLiteral(SLX_ASSET_DIR), QStringLiteral(SLX_PROJECT_DIR), temp.path());
        c.setPython(QStringLiteral(SLX_PYTHON_EXECUTABLE));
        QVERIFY(!c.inspectorCycle()["dataset"].toString().isEmpty());
        c.freezeInspector(6);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 180000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        const auto frozen = c.inspectorCycle()["benchmark_sha256"].toString();
        QVERIFY(!frozen.isEmpty());
        c.enterEditor(); c.setAcceleratedPreview(true);
        c.selectReference("U4"); QVERIFY(c.editComponent("U4", false, 1.7, .4, 17));
        c.startInspection(); QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        c.collectInspectorCase(); QCOMPARE(c.inspectorCases().size(), 1);
        c.collectInspectorCase(); QCOMPARE(c.inspectorCases().size(), 1); c.clearError();
        c.stopInspection(); c.augmentInspector(6);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 180000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QFile aug(c.inspectorCycle()["augmentation"].toString() + "/dataset.json");
        QVERIFY(aug.open(QIODevice::ReadOnly));
        for (const auto &v : QJsonDocument::fromJson(aug.readAll()).object()["samples"].toArray())
            QVERIFY(v.toObject()["split"] != "test");
        const auto oldModel = c.inspectorModel();
        c.trainInspector(400); QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        const auto comparison = c.inspectorCycle()["comparison"].toMap();
        QCOMPARE(comparison["benchmark_sha256"].toString(), frozen);
        QVERIFY(comparison["baseline"].toMap()["samples"].toInt() > 800);
        QCOMPARE(c.inspectorModel(), oldModel); // No automatic promotion.
        c.activateInspector(true); QVERIFY(c.inspectorModel() != oldModel);
        c.activateInspector(false); QCOMPARE(c.inspectorModel(), oldModel);
        // A modified frozen baseline blocks a new training run.
        QFile baseline(c.inspectorCycle()["frozen_model"].toString());
        QVERIFY(baseline.open(QIODevice::Append)); baseline.write(" "); baseline.close();
        c.trainInspector(100); QVERIFY(!c.busy()); QVERIFY(!c.error().isEmpty());
    }
    void interactiveInspection()
    {
        QTemporaryDir temp;
        Controller c(QStringLiteral(SLX_ASSET_DIR), QStringLiteral(SLX_PROJECT_DIR), temp.path());
        c.enterEditor();
        c.setAcceleratedPreview(true);
        QVERIFY(c.editComponent("C2", true, 0, 0, 0));
        QVERIFY(c.editComponent("U4", false, 3.7, -1.1, 23));
        QCOMPARE(c.edits().size(), 2);
        QVERIFY(c.assemblyReport().isEmpty());
        c.startInspection();
        QVERIFY(c.inspectionActive());
        QVERIFY(!c.editComponent("C1", true, 0, 0, 0));
        c.restoreAssembly();
        QCOMPARE(c.edits().size(), 2);
        QTRY_VERIFY_WITH_TIMEOUT(!c.inspectionScanning(), 30000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QCOMPARE(c.assemblyReport()["components"].toInt(), 46);
        QVERIFY(!c.image("capture").isNull());
        const auto results = c.assemblyReport()["results"].toList();
        for (const auto &v : results)
        {
            const auto result = v.toMap();
            if (result["reference"] == "C2" || result["reference"] == "U4")
            {
                qInfo() << result;
                QVERIFY(result["decision"] != "PASS");
                QVERIFY(result["class_index"].toInt() != 0);
            }
        }
        c.selectReference("C2");
        QVERIFY(!c.image("capture-detail").isNull());
        const auto capturedImage = c.image("capture");
        c.stopInspection();
        c.restoreComponent("C2");
        QVERIFY(c.inspectionStale());
        QCOMPARE(c.image("capture"), capturedImage);
        c.restoreAssembly();
        QVERIFY(c.edits().isEmpty());
        c.startInspection();
        QTRY_VERIFY_WITH_TIMEOUT(!c.inspectionScanning(), 30000);
        QCOMPARE(c.assemblyReport()["defects"].toInt(), 0);
        QVERIFY(!c.inspectionStale());
    }
    void completeAndCancelledRuns()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        Controller c(QStringLiteral(SLX_ASSET_DIR), QStringLiteral(SLX_PROJECT_DIR), temp.path());
        c.setPython(QStringLiteral(SLX_PYTHON_EXECUTABLE));
        c.generate(15, 811);
        c.cancel();
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 20000);
        QVERIFY(c.datasetPath().isEmpty());
        const auto partial = QDir(temp.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QCOMPARE(partial.size(), 1);
        QVERIFY(QFile::exists(temp.path() + "/" + partial[0] + "/incomplete.json"));
        QVERIFY(!QFile::exists(temp.path() + "/" + partial[0] + "/dataset.json"));
        c.generate(1, 2026);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 5000);
        QVERIFY(c.error().contains("15..2000"));
        c.clearError();
        c.generate(15, 2026);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 90000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QCOMPARE(c.dataset()["samples"].toList().size(), 75);
        QVERIFY(QFile::exists(c.datasetPath() + "/data.yaml"));
        c.train(20);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QVERIFY(!c.modelPath().isEmpty());
        QVERIFY(c.evaluation()["test"].toMap()["accuracy"].toDouble() > .5);
        const auto predictions = c.evaluation()["predictions"].toList();
        QVERIFY(!predictions.isEmpty());
        for (const auto &entry : predictions)
        {
            const auto expected = entry.toMap();
            c.selectSample(expected["sample_index"].toInt());
            QCOMPARE(c.prediction()["class_index"].toInt(), expected["predicted_index"].toInt());
            QVERIFY(std::abs(c.prediction()["confidence"].toDouble() -
                             expected["confidence"].toDouble()) < 1e-5);
        }
        const QString model = c.modelPath(), dataset = c.datasetPath();
        QVERIFY(c.loadDataset(QUrl::fromLocalFile(dataset)));
        QCOMPARE(c.modelPath(), model);
        QVERIFY(c.sampleUrl("../outside.png").isEmpty());
        QFile originalModel(model);
        QVERIFY(originalModel.open(QIODevice::ReadOnly));
        const auto originalBytes = originalModel.readAll();
        c.freezeBenchmark(15);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QCOMPARE(c.cycle()["benchmark_samples"].toInt(), 75);
        // Held-out images cannot become development cases, even after changes.
        QVERIFY(!c.canCollect());
        c.collectCase();
        QCOMPARE(c.cases().size(), 0);
        c.scanStress();
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QCOMPARE(c.stress()["samples"].toInt(), 60);
        QVERIFY(c.cases().size() > 0);
        for (const auto &v : c.cases())
        {
            const auto item = v.toMap();
            QCOMPARE(item["source"].toString(), QString("development"));
            QVERIFY(item["camera"].toMap()["blur_sigma_px"].toDouble() <= 2);
            QVERIFY(item["camera"].toMap()["tilt_deg"].toDouble() >= 18);
        }
        const auto benchmark = dataset + "/improvement/" + c.cycle()["benchmark"].toString();
        QFile frozenManifest(benchmark + "/dataset.json");
        QVERIFY(frozenManifest.open(QIODevice::ReadOnly));
        const auto frozenBytes = frozenManifest.readAll();
        c.prepareImprovement(15);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 120000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QFile augmentation(dataset + "/improvement/" + c.cycle()["augmentation"].toString() + "/dataset.json");
        QVERIFY(augmentation.open(QIODevice::ReadOnly));
        const auto extra = QJsonDocument::fromJson(augmentation.readAll()).object();
        QCOMPARE(extra["purpose"].toString(), QString("augmentation"));
        for (const auto &sample : extra["samples"].toArray())
            QVERIFY(sample.toObject()["split"].toString() != "test");
        c.trainImprovement(40);
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 60000);
        QVERIFY2(c.error().isEmpty(), qPrintable(c.error()));
        QVERIFY(c.modelPath() != model);
        QCOMPARE(c.versions().size(), 2);
        QCOMPARE(c.comparison()["after"].toMap()["samples"].toInt(), 75);
        QCOMPARE(c.comparison()["threshold"].toDouble(), .75);
        QCOMPARE(c.comparison()["reference_test"].toMap()["after"].toMap()["samples"].toInt(), 15);
        QCOMPARE(c.evaluation()["split_counts"].toMap()["train"].toInt(), 95);
        QVERIFY(QFile::exists(QFileInfo(c.modelPath()).absolutePath() + "/comparison.html"));
        originalModel.seek(0);
        QCOMPARE(originalModel.readAll(), originalBytes);
        frozenManifest.seek(0);
        QCOMPARE(frozenManifest.readAll(), frozenBytes);
        const auto candidate = c.modelPath();
        const auto comparisonRows = c.comparison()["predictions"].toList();
        const auto frozen = QJsonDocument::fromJson(frozenBytes).object();
        const auto benchmarkSamples = frozen["samples"].toArray();
        QFile benchmarkFeatures(benchmark + "/features.f32");
        QVERIFY(benchmarkFeatures.open(QIODevice::ReadOnly));
        QDataStream featureStream(&benchmarkFeatures);
        featureStream.setByteOrder(QDataStream::LittleEndian);
        featureStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        slx::Model before(model), after(candidate);
        for (int i = 0; i < comparisonRows.size(); ++i)
        {
            slx::Frame frame;
            frame.features.resize(slx::FeatureCount);
            for (float &value : frame.features) featureStream >> value;
            const auto sample = benchmarkSamples[i].toObject();
            QFile annotation(benchmark + "/" + sample["annotation"].toString());
            QVERIFY(annotation.open(QIODevice::ReadOnly));
            frame.annotations = QJsonDocument::fromJson(annotation.readAll()).object();
            for (const auto &entry : QList<QPair<QString, slx::Model *>>{{"before", &before}, {"after", &after}})
            {
                const auto actual = entry.second->predict(frame, .75);
                const auto expected = comparisonRows[i].toMap()[entry.first].toMap();
                QCOMPARE(actual["class_index"].toInt(), expected["predicted_index"].toInt());
                QCOMPARE(actual["accepted"].toBool(), expected["accepted"].toBool());
                QVERIFY(std::abs(actual["confidence"].toDouble() - expected["confidence"].toDouble()) < 1e-5);
            }
        }
        QVERIFY(c.loadDataset(QUrl::fromLocalFile(dataset)));
        QCOMPARE(c.modelPath(), candidate);
        QVERIFY(c.caseUrl("../../outside.png").isEmpty());
        // Reopening refuses a modified frozen benchmark rather than silently
        // comparing on a different test after an iteration.
        frozenManifest.close();
        QVERIFY(frozenManifest.open(QIODevice::Append));
        frozenManifest.write(" ");
        frozenManifest.close();
        c.prepareImprovement(15);
        QVERIFY(!c.busy());
        QVERIFY(c.error().contains("Frozen benchmark changed"));
    }
};
QTEST_GUILESS_MAIN(WorkflowTests)
#include "workflow_tests.moc"
