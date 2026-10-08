// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Pixel-only inference, independent component edits and acquisition gates.
#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>
#include <slx/core.h>

class AssemblyTests : public QObject
{
    Q_OBJECT
private slots:
    void missingComponentsAndUnseenPoses()
    {
        slx::Engine engine(QStringLiteral(SLX_ASSET_DIR));
        const QString model = QStringLiteral(SLX_PROJECT_DIR) + "/assets/models/assembly-inspector.json";
        slx::Camera camera;
        const auto nominal = engine.render(0, camera, {640, 480});
        const auto normalReport = engine.inspectAssembly(nominal.image, camera, model);
        QCOMPARE(normalReport["components"].toInt(), 46);
        QCOMPARE(normalReport["defects"].toInt(), 0);
        const auto reduced = engine.inspectAssembly(nominal.image.scaled(320, 240), camera, model);
        QCOMPARE(reduced["review"].toInt(), 46);
        QCOMPARE(reduced["passed"].toInt(), 0);
        int detected = 0, deferred = 0, missed = 0;
        for (const auto &v : engine.components())
        {
            const auto reference = v.toObject()["reference"].toString();
            auto frame = engine.renderAssembly(QJsonArray{QJsonObject{{"reference", reference}, {"removed", true}}}, camera, {640, 480});
            // Annotation/mask/feature data are discarded: inference sees RGB only.
            frame.annotations = {};
            frame.instanceMask = {};
            frame.features = {};
            const auto report = engine.inspectAssembly(frame.image.copy(), camera, model);
            for (const auto &entry : report["results"].toArray())
            {
                const auto row = entry.toObject();
                if (row["reference"].toString() != reference) continue;
                detected += row["decision"].toString() == "DEFECT";
                deferred += row["decision"].toString() == "REVIEW";
                missed += row["decision"].toString() == "PASS";
            }
        }
        qInfo() << "46 independent removals:" << detected << "defects," << deferred << "reviews," << missed << "missed";
        QCOMPARE(missed, 0);
        QVERIFY(detected >= 35);
        for (const auto &reference : {QString("U4"), QString("C1"), QString("J2"), QString("R7")})
        {
            const auto frame = engine.renderAssembly(QJsonArray{QJsonObject{{"reference", reference},
                {"dx_mm", -4.3}, {"dy_mm", 1.7}, {"rotation_deg", 37}}}, camera, {640, 480});
            const auto report = engine.inspectAssembly(frame.image, camera, model);
            for (const auto &entry : report["results"].toArray())
            {
                const auto row = entry.toObject();
                if (row["reference"].toString() == reference)
                    QVERIFY2(row["decision"].toString() != "PASS", qPrintable(reference));
            }
        }
        camera.tilt = 60;
        const auto unsupported = engine.render(0, camera, {640, 480});
        const auto uncertain = engine.inspectAssembly(unsupported.image, camera, model);
        QCOMPARE(uncertain["passed"].toInt(), 0);
        QCOMPARE(uncertain["defects"].toInt(), 0);
        QCOMPARE(uncertain["review"].toInt(), 46);
    }
};
QTEST_GUILESS_MAIN(AssemblyTests)
#include "assembly_tests.moc"
