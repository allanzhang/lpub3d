#include "studiotexmap.h"

#include <QObject>
#include <QStringList>
#include <QVector3D>
#include <QtMath>

namespace {

bool projectionPoints(const StudioTexMapData &data,
                      QVector3D &p0,
                      QVector3D &p1,
                      QVector3D &p2)
{
    if (!data.valid || data.values.size() != 16)
        return false;

    const float t0 = data.values.at(0);
    const float t1 = data.values.at(1);
    const float t2 = data.values.at(2);
    const float t3 = data.values.at(3);
    const float t5 = data.values.at(5);
    const float t6 = data.values.at(6);
    const float t8 = data.values.at(8);
    const float t9 = data.values.at(9);
    const float t11 = data.values.at(11);

    const QVector3D axisU(t3, t6, -t9);
    const QVector3D axisV(-t5, -t8, t11);
    if (axisU.lengthSquared() < 1.0e-8f || axisV.lengthSquared() < 1.0e-8f)
        return false;

    const QVector3D origin(t0, t1, -t2);
    const float minU = data.values.at(12);
    const float minV = data.values.at(13);
    const float diffU = data.values.at(14) - minU;
    const float diffV = data.values.at(15) - minV;
    if (qFuzzyIsNull(diffU) || qFuzzyIsNull(diffV))
        return false;

    p0 = origin + axisU * minU + axisV * minV;
    p1 = p0 + axisU * diffU;
    p2 = p0 + axisV * diffV;
    return true;
}

} // namespace

StudioTexMapData StudioTexMap::parse(const QString &line)
{
    StudioTexMapData result;

    const QStringList tokens = line.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (tokens.size() < 19 || tokens.at(0) != QLatin1String("0") ||
        tokens.at(1).compare(QLatin1String("PE_TEX_INFO"), Qt::CaseInsensitive) != 0) {
        result.errorMessage = QObject::tr("Invalid PE_TEX_INFO record.");
        return result;
    }

    result.values.reserve(16);
    for (int i = 0; i < 16; ++i) {
        bool ok = false;
        const float value = tokens.at(2 + i).toFloat(&ok);
        if (!ok) {
            result.errorMessage = QObject::tr("Invalid PE_TEX_INFO numeric value at index %1.").arg(i);
            return result;
        }
        result.values.append(value);
    }

    result.pngData = QByteArray::fromBase64(tokens.at(18).toLatin1());
    if (result.pngData.isEmpty()) {
        result.values.clear();
        result.errorMessage = QObject::tr("PE_TEX_INFO texture image is empty or invalid.");
        return result;
    }

    result.pointMin = QPointF(result.values.at(12), result.values.at(13));
    result.pointMax = QPointF(result.values.at(14), result.values.at(15));
    result.valid = true;
    return result;
}

bool StudioTexMap::isStudioTextureDirective(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (!trimmed.startsWith(QLatin1String("0 !STUDIO_TEXMAP"), Qt::CaseInsensitive))
        return false;

    const QStringList tokens = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (tokens.size() < 3)
        return false;

    return tokens.at(2).compare(QLatin1String("START"), Qt::CaseInsensitive) == 0 ||
           tokens.at(2).compare(QLatin1String("END"), Qt::CaseInsensitive) == 0;
}

QString StudioTexMap::texMapStart(const QVector3D &p0, const QVector3D &p1, const QVector3D &p2, const QString &textureFile)
{
    return QStringLiteral("0 !TEXMAP START PLANAR %1 %2 %3 %4 %5 %6 %7 %8 %9 %10")
        .arg(p0.x(), 0, 'f', 6)
        .arg(p0.y(), 0, 'f', 6)
        .arg(p0.z(), 0, 'f', 6)
        .arg(p1.x(), 0, 'f', 6)
        .arg(p1.y(), 0, 'f', 6)
        .arg(p1.z(), 0, 'f', 6)
        .arg(p2.x(), 0, 'f', 6)
        .arg(p2.y(), 0, 'f', 6)
        .arg(p2.z(), 0, 'f', 6)
        .arg(textureFile);
}

QString StudioTexMap::texMapStart(const StudioTexMapData &data, const QString &textureFile)
{
    QVector3D p0, p1, p2;
    if (!projectionPoints(data, p0, p1, p2) || textureFile.isEmpty())
        return QString();

    return QStringLiteral("0 !TEXMAP START PLANAR %1 %2 %3 %4 %5 %6 %7 %8 %9 %10")
        .arg(p0.x(), 0, 'f', 6)
        .arg(p0.y(), 0, 'f', 6)
        .arg(p0.z(), 0, 'f', 6)
        .arg(p1.x(), 0, 'f', 6)
        .arg(p1.y(), 0, 'f', 6)
        .arg(p1.z(), 0, 'f', 6)
        .arg(p2.x(), 0, 'f', 6)
        .arg(p2.y(), 0, 'f', 6)
        .arg(p2.z(), 0, 'f', 6)
        .arg(textureFile);
}

QString StudioTexMap::texMapQuad(const StudioTexMapData &data, const QString &textureFile)
{
    QVector3D p0, p1, p2;
    if (!projectionPoints(data, p0, p1, p2) || textureFile.isEmpty())
        return QString();

    const QVector3D p3 = p1 + (p2 - p0);
    return QStringLiteral("%1\n4 16 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13\n0 !TEXMAP END")
        .arg(texMapStart(data, textureFile))
        .arg(p0.x(), 0, 'f', 6).arg(p0.y(), 0, 'f', 6).arg(p0.z(), 0, 'f', 6)
        .arg(p1.x(), 0, 'f', 6).arg(p1.y(), 0, 'f', 6).arg(p1.z(), 0, 'f', 6)
        .arg(p3.x(), 0, 'f', 6).arg(p3.y(), 0, 'f', 6).arg(p3.z(), 0, 'f', 6)
        .arg(p2.x(), 0, 'f', 6).arg(p2.y(), 0, 'f', 6).arg(p2.z(), 0, 'f', 6);
}
