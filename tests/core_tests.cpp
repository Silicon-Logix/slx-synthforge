// Silicon LogiX / SLX SynthForge
// Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
// Author: Marco Pezzullo
// License: Silicon LogiX Evaluation License 1.0. See LICENSE.

/// @file
/// @brief Checks actual GLB rendering, absence masks and invariant model regions.
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>
#include <slx/core.h>
#include <stdexcept>
class CoreTests : public QObject
{
    Q_OBJECT
    std::unique_ptr<slx::Engine> engine;
    int countId(const QImage &mask, int id)
    {
        int count = 0;
        for (int y = 0; y < mask.height(); ++y)
        {
            auto p = reinterpret_cast<const quint16 *>(mask.constScanLine(y));
            for (int x = 0; x < mask.width(); ++x)
                count += p[x] == id;
        }
        return count;
    }
  private slots:
    void editableAssembly()
    {
        const slx::Camera camera;
        const QSize size(640, 480);
        const auto nominal = engine->render(0, camera, size);
        const QJsonArray edits{QJsonObject{{"reference", "C2"}, {"removed", true}},
                               QJsonObject{{"reference", "U4"}, {"dx_mm", 3.7}, {"dy_mm", -1.1}, {"rotation_deg", 23}}};
        const auto edited = engine->renderAssembly(edits, camera, size);
        QCOMPARE(engine->components().size(), 46);
        QCOMPARE(engine->renderAssembly({}, camera, size).image, nominal.image);
        QVERIFY(edited.image != nominal.image);
        const auto c2 = engine->mesh("C2");
        QCOMPARE(countId(edited.instanceMask, c2.instance), 0);
        QVERIFY(countId(nominal.instanceMask, c2.instance) > 16);
        QVERIFY(countId(edited.instanceMask, engine->mesh("U4").instance) > 0);
        QCOMPARE(edited.annotations["component_regions"], nominal.annotations["component_regions"]);
        QCOMPARE(engine->renderAssembly(edits, camera, size).image, edited.image);
        QVERIFY(!c2.vertices.isEmpty());
        QVERIFY(engine->inspect(edited, "C2").contains("bbox"));
        QVERIFY(engine->highlight(edited, "C2") != edited.image);
        bool picked = false;
        for (int y = 1; y < size.height() - 1 && !picked; ++y)
        {
            const auto row = reinterpret_cast<const quint16 *>(nominal.instanceMask.constScanLine(y));
            for (int x = 1; x < size.width() - 1 && !picked; ++x)
                if (row[x] == 4 && row[x - 1] == 4 && row[x + 1] == 4)
                {
                    QCOMPARE(engine->pickAssembly({}, camera, size, QPointF(x + .5, y + .5)), "C1");
                    picked = true;
                }
        }
        QVERIFY(picked);
        QVERIFY_EXCEPTION_THROWN(engine->renderAssembly(QJsonArray{QJsonObject{{"reference", "NO_SUCH_COMPONENT"}}}, camera), std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(engine->renderAssembly(QJsonArray{QJsonObject{{"reference", "U4"}, {"dx_mm", 11}}}, camera), std::runtime_error);
    }
    void initTestCase()
    {
        engine = std::make_unique<slx::Engine>(QStringLiteral(SLX_ASSET_DIR));
        QCOMPARE(engine->scenarios().size(), 5);
        QCOMPARE(engine->assetSummary()["references"].toInt(), 50);
    }
    void pixelsAndAbsenceAgree()
    {
        const auto normal = engine->render(0, slx::Camera(), {640, 480});
        const auto missing = engine->render(1, slx::Camera(), {640, 480});
        const auto bridge = engine->render(4, slx::Camera(), {640, 480});
        QVERIFY(countId(normal.instanceMask, 4) > 100);
        QCOMPARE(countId(missing.instanceMask, 4), 0);
        QCOMPARE(countId(normal.instanceMask, 51), 0);
        QVERIFY(countId(bridge.instanceMask, 51) > 0);
        QVERIFY(normal.image != missing.image);
        QCOMPARE(normal.features.size(), slx::FeatureCount);
        QCOMPARE(normal.annotations["inspection_regions"],
                 missing.annotations["inspection_regions"]);
        QCOMPARE(normal.annotations["inspection_regions"],
                 bridge.annotations["inspection_regions"]);
        for (float value : normal.features)
            QVERIFY(value >= 0 && value <= 1);
        for (const auto &v : normal.annotations["instances"].toArray())
        {
            auto o = v.toObject();
            auto b = o["bbox"].toArray();
            QVERIFY(b[0].toInt() >= 0 && b[1].toInt() >= 0);
            QVERIFY(b[0].toInt() + b[2].toInt() <= 640);
            QVERIFY(b[1].toInt() + b[3].toInt() <= 480);
            QVERIFY(o["visible_pixels"].toInt() > 0);
        }
    }
    void deterministicAndObservable()
    {
        slx::Camera c;
        c.noiseSeed = 42;
        const auto a = engine->render(3, c, {320, 240}), b = engine->render(3, c, {320, 240});
        QCOMPARE(a.image, b.image);
        QCOMPARE(a.features, b.features);
        QVERIFY(a.annotations["polarity_observable"].toBool());
        c.tilt = 0;
        QVERIFY(!engine->render(3, c, {320, 240}).annotations["polarity_observable"].toBool());
        c.span = 0;
        QVERIFY_EXCEPTION_THROWN(engine->render(0, c), std::runtime_error);
    }
    void modelBoundaryAndReview()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QJsonArray means, scales, weights;
        for (int i = 0; i < slx::FeatureCount; ++i)
        {
            means.append(0);
            scales.append(1);
            for (int j = 0; j < 5; ++j)
                weights.append(0);
        }
        QJsonObject o{{"schema", "slx.roi-softmax.v1"},
                      {"feature_count", slx::FeatureCount},
                      {"classes", QJsonArray::fromStringList(slx::classNames())},
                      {"dataset_id", "contract-test"},
                      {"mean", means},
                      {"scale", scales},
                      {"weights", weights},
                      {"bias", QJsonArray{5, 0, 0, 0, 0}}};
        const auto path = dir.path() + "/test.slxmodel.json";
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(o).toJson());
        f.close();
        slx::Model model(path);
        auto frame = engine->render(0, slx::Camera(), {320, 240});
        auto prediction = model.predict(frame, .75);
        QCOMPARE(prediction["class_index"].toInt(), 0);
        QCOMPARE(prediction["decision"].toString(), QString("PASS"));
        QVERIFY(prediction["confidence"].toDouble() > .97);
        slx::Camera blurred;
        blurred.blur = 5;
        const auto badImage = engine->render(0, blurred, {320, 240});
        const auto guarded = model.predict(badImage, .75);
        QVERIFY(guarded["confidence"].toDouble() > .97);
        QVERIFY(!guarded["accepted"].toBool());
        QCOMPARE(guarded["action"].toString(), QString("REACQUIRE"));
        QVERIFY(!guarded["image_usable"].toBool());
        const auto evidence = model.evidence(frame, 0);
        QCOMPARE(evidence.size(), frame.image.size());
        slx::Camera c;
        c.tilt = 0;
        QCOMPARE(model.predict(engine->render(0, c, {320, 240}), .75)["decision"].toString(),
                 QString("REVIEW"));
        o["schema"] = "unknown";
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(o).toJson());
        f.close();
        QVERIFY_EXCEPTION_THROWN(slx::Model invalid(path), std::runtime_error);
    }
    void opticalChangesPreserveGeometryAndCadLinks()
    {
        slx::Camera camera;
        const auto original = engine->render(4, camera, {640, 480});
        camera.reflection = 4;
        camera.lightAzimuth = 65;
        camera.blur = 1.7;
        const auto stressed = engine->render(4, camera, {640, 480});
        QCOMPARE(original.instanceMask, stressed.instanceMask);
        QCOMPARE(original.annotations["defects"], stressed.annotations["defects"]);
        QVERIFY(original.image != stressed.image);
        QVERIFY(original.features != stressed.features);
        QCOMPARE(stressed.image, engine->render(4, camera, {640, 480}).image);
        const auto inspected = engine->inspect(stressed, "U4");
        QCOMPARE(inspected["cad"].toObject()["instance_id"].toInt(), 10);
        QVERIFY(inspected["visible"].toBool());
        const auto b = inspected["bbox"].toArray();
        QCOMPARE(engine->referenceAt(stressed, QPoint(b[0].toInt() + b[2].toInt() / 2,
                                                     b[1].toInt() + b[3].toInt() / 2)), QString("U4"));
        QVERIFY(engine->highlight(stressed, "U4") != stressed.image);
        const auto missing = engine->render(1, slx::Camera(), {320, 240});
        const auto expected = engine->inspect(missing, "C1");
        QVERIFY(!expected["visible"].toBool());
        QVERIFY(expected.contains("bbox"));
        camera.blur = -1;
        QVERIFY_EXCEPTION_THROWN(engine->render(0, camera), std::runtime_error);
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
