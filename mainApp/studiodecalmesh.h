#ifndef STUDIODECALMESH_H
#define STUDIODECALMESH_H

#include <QHash>
#include <QString>
#include <QVector>
#include <QVector3D>

#include "studiotexmap.h"

struct StudioMeshTriangle
{
    QVector3D p0;
    QVector3D p1;
    QVector3D p2;
};

class StudioDecalMesh
{
public:
    static QString texMapStart(const StudioTexMapData &textureData,
                               const QString &textureFile);

    static QString build(const StudioTexMapData &textureData,
                         const QHash<QString, QString> &partFiles,
                         const QString &basePart,
                         const QString &textureFile,
                         int *triangleCount = nullptr);
};

#endif // STUDIODECALMESH_H
