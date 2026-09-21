#ifndef STUDIOTEXMAP_H
#define STUDIOTEXMAP_H

#include <QByteArray>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QVector3D>

struct StudioTexMapData
{
    bool valid = false;
    QVector<float> values;
    QPointF pointMin;
    QPointF pointMax;
    QByteArray pngData;
    QString errorMessage;
};

class StudioTexMap
{
public:
    static StudioTexMapData parse(const QString &line);
    static bool isStudioTextureDirective(const QString &line);
    static QString texMapStart(const StudioTexMapData &data, const QString &textureFile);
    static QString texMapStart(const QVector3D &p0, const QVector3D &p1, const QVector3D &p2, const QString &textureFile);
    static QString texMapQuad(const StudioTexMapData &data, const QString &textureFile);
};

#endif // STUDIOTEXMAP_H
