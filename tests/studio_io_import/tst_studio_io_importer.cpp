#include <QtTest/QtTest>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <algorithm>

#include "studioioimporter.h"
#include "studio_mesh_processor.h"
#include "studiotexmap.h"

class StudioIoImporterTest : public QObject
{
    Q_OBJECT

private slots:
    void recognizesStudioProjectExtensions();
    void importsPrimaryModelAndCustomParts();
    void importsPasswordProtectedPackage();
    void preservesSourceArchive();
    void parsesStudioTextureMetadata();
    void recognizesStudioTextureDirective();
    void importedModelUsesScopedStudioTextureMarker();
    void studioMeshProcessorAssignsSeedUv();
    void studioMeshProcessorExpandsConnectedTriangles();
};

void StudioIoImporterTest::recognizesStudioProjectExtensions()
{
    QVERIFY(StudioIoImporter::isStudioProject(QStringLiteral("model.io")));
    QVERIFY(StudioIoImporter::isStudioProject(QStringLiteral("model.mo")));
    QVERIFY(!StudioIoImporter::isStudioProject(QStringLiteral("model.ldr")));
}

void StudioIoImporterTest::importsPrimaryModelAndCustomParts()
{
    const QString fixture = qEnvironmentVariable("STUDIO_IO_FIXTURE");
    QVERIFY2(!fixture.isEmpty(), "STUDIO_IO_FIXTURE is required");
    QVERIFY2(QFileInfo::exists(fixture), qPrintable(fixture));

    const StudioIoImportResult result = StudioIoImporter::import(fixture);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(result.modelFilePath.endsWith(QStringLiteral("studio-io-import.ldr")));
    QVERIFY(QFileInfo::exists(result.modelFilePath));
    QFile imported(result.modelFilePath);
    QVERIFY(imported.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString importedContents = QString::fromUtf8(imported.readAll());
    QVERIFY(importedContents.contains(QStringLiteral("0 FILE m28d837e8_2026624_114514.dat")));
    QVERIFY(importedContents.contains(QStringLiteral("0 !LDRAW_ORG Unofficial_Part")));
    QVERIFY(importedContents.contains(QStringLiteral("0 PE_TEX_INFO")));
    QVERIFY(importedContents.count(QStringLiteral("0 !STUDIO_TEXMAP START PLANAR")) >= 2);
    QVERIFY(!result.customSearchDirs.isEmpty());
    QVERIFY(QDir(result.cacheDir + QStringLiteral("/textures")).entryInfoList(QDir::Files).size() >= 2);

    QVERIFY(StudioIoImporter::removeCache(result));
    QVERIFY(!QDir(result.cacheDir).exists());
}

void StudioIoImporterTest::importsPasswordProtectedPackage()
{
    const QString fixture = qEnvironmentVariable("STUDIO_IO_ENCRYPTED_FIXTURE");
    if (fixture.isEmpty())
        QSKIP("STUDIO_IO_ENCRYPTED_FIXTURE is not set");
    QVERIFY2(QFileInfo::exists(fixture), qPrintable(fixture));

    const StudioIoImportResult result = StudioIoImporter::import(fixture);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(QFileInfo::exists(result.modelFilePath));
    QVERIFY(StudioIoImporter::removeCache(result));
}

static QByteArray fileHash(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

void StudioIoImporterTest::preservesSourceArchive()
{
    const QString fixture = qEnvironmentVariable("STUDIO_IO_FIXTURE");
    QVERIFY2(!fixture.isEmpty(), "STUDIO_IO_FIXTURE is required");
    const QByteArray before = fileHash(fixture);
    QVERIFY(!before.isEmpty());

    const StudioIoImportResult result = StudioIoImporter::import(fixture);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(fileHash(fixture), before);
    QVERIFY(StudioIoImporter::removeCache(result));
}

void StudioIoImporterTest::parsesStudioTextureMetadata()
{
    const QString line = QStringLiteral(
        "0 PE_TEX_INFO -0.3333 0.5 -0.915 0.6683 0 0 0 0 1.0083 0 0.22 0 "
        "-80 12 160 -12 iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAFgwJ/lFs5WQAAAABJRU5ErkJggg==");
    const StudioTexMapData data = StudioTexMap::parse(line);

    QVERIFY2(data.valid, qPrintable(data.errorMessage));
    QCOMPARE(data.values.size(), 16);
    QCOMPARE(data.pointMin, QPointF(-80.0, 12.0));
    QCOMPARE(data.pointMax, QPointF(160.0, -12.0));
    QVERIFY(!data.pngData.isEmpty());
}

void StudioIoImporterTest::recognizesStudioTextureDirective()
{
    QVERIFY(StudioTexMap::isStudioTextureDirective(QStringLiteral("0 !STUDIO_TEXMAP START PLANAR 0 0 0 texture")));
    QVERIFY(StudioTexMap::isStudioTextureDirective(QStringLiteral("  0 !studio_texmap END")));
    QVERIFY(!StudioTexMap::isStudioTextureDirective(QStringLiteral("0 !TEXMAP START PLANAR 0 0 0 texture")));
    QVERIFY(!StudioTexMap::isStudioTextureDirective(QStringLiteral("0 // STUDIO_TEXMAP")));
}

void StudioIoImporterTest::importedModelUsesScopedStudioTextureMarker()
{
    const QString fixture = qEnvironmentVariable("STUDIO_IO_FIXTURE");
    QVERIFY2(!fixture.isEmpty(), "STUDIO_IO_FIXTURE is required");
    QVERIFY2(QFileInfo::exists(fixture), qPrintable(fixture));

    const StudioIoImportResult result = StudioIoImporter::import(fixture);
    QVERIFY2(result.success, qPrintable(result.errorMessage));

    QFile imported(result.modelFilePath);
    QVERIFY(imported.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(imported.readAll());

    QVERIFY(contents.contains(QStringLiteral("0 !STUDIO_TEXMAP START PLANAR")));
    QVERIFY(contents.contains(QStringLiteral("1 16 0.000000 0.000000 0.000000")));
    QVERIFY(contents.contains(QStringLiteral("6112.dat")));
    QVERIFY(contents.contains(QStringLiteral("37352.dat")));
    QVERIFY(!contents.contains(QStringLiteral("0 !TEXMAP START PLANAR")));

    QVERIFY(StudioIoImporter::removeCache(result));
}

static StudioMeshProcessor::Input seedUvFixture()
{
    StudioMeshProcessor::Input input;
    input.vertices = {
        {{0.10f, 1.0f, 0.10f}, {0.0f, -1.0f, 0.0f}},
        {{0.20f, 1.0f, 0.10f}, {0.0f, -1.0f, 0.0f}},
        {{0.10f, 1.0f, 0.20f}, {0.0f, -1.0f, 0.0f}},
        {{0.70f, 1.0f, 0.70f}, {0.0f, -1.0f, 0.0f}},
        {{0.80f, 1.0f, 0.70f}, {0.0f, -1.0f, 0.0f}},
        {{0.70f, 1.0f, 0.80f}, {0.0f, -1.0f, 0.0f}}
    };
    input.triangles = {{{0, 1, 2}}, {{3, 4, 5}}};
    input.texture.values = {0.0f, 0.0f, 0.0f,
                            1.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 1.0f,
                            0.0f, 0.0f, 1.0f, 1.0f};
    return input;
}

void StudioIoImporterTest::studioMeshProcessorAssignsSeedUv()
{
    const StudioMeshProcessor::Result result = StudioMeshProcessor::assign(seedUvFixture());

    QCOMPARE(result.selectedTriangles.size(), 1);
    QCOMPARE(result.selectedTriangles.at(0), 0);
    QVERIFY(result.uvAssigned.at(0));
    QVERIFY(result.uvAssigned.at(1));
    QVERIFY(result.uvAssigned.at(2));
    QVERIFY(!result.uvAssigned.at(3));

    QCOMPARE(result.uv.at(0).x, 0.10f);
    QCOMPARE(result.uv.at(0).y, 0.10f);
    QCOMPARE(result.uv.at(1).x, 0.20f);
    QCOMPARE(result.uv.at(1).y, 0.10f);
    QCOMPARE(result.uv.at(2).x, 0.10f);
    QCOMPARE(result.uv.at(2).y, 0.20f);
}

void StudioIoImporterTest::studioMeshProcessorExpandsConnectedTriangles()
{
    StudioMeshProcessor::Input input = seedUvFixture();
    input.vertices.push_back({{0.90f, 1.0f, 0.90f}, {0.0f, -1.0f, 0.0f}});
    input.vertices.push_back({{1.00f, 1.0f, 0.90f}, {0.0f, -1.0f, 0.0f}});
    input.triangles.push_back({{2, 6, 7}});

    const StudioMeshProcessor::Result result = StudioMeshProcessor::assign(input);

    QCOMPARE(result.selectedTriangles.size(), 2);
    QVERIFY(std::find(result.selectedTriangles.begin(), result.selectedTriangles.end(), 0) != result.selectedTriangles.end());
    QVERIFY(std::find(result.selectedTriangles.begin(), result.selectedTriangles.end(), 2) != result.selectedTriangles.end());
    QVERIFY(std::find(result.selectedTriangles.begin(), result.selectedTriangles.end(), 1) == result.selectedTriangles.end());
}

QTEST_GUILESS_MAIN(StudioIoImporterTest)

#include "tst_studio_io_importer.moc"
