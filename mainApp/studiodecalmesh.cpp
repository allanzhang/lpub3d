#include "studiodecalmesh.h"

#include <QHash>
#include <QStringList>
#include <QVector3D>
#include <QtMath>

#include <limits>

namespace {

struct Matrix3
{
    float m[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
};

struct StudioTransform
{
    QVector3D translation;
    Matrix3 rotation;
    float scale[3] = {1, 1, 1};
};

Matrix3 identityMatrix()
{
    return Matrix3();
}

Matrix3 multiply(const Matrix3 &a, const Matrix3 &b)
{
    Matrix3 result;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            result.m[r][c] = 0.0f;
            for (int k = 0; k < 3; ++k)
                result.m[r][c] += a.m[r][k] * b.m[k][c];
        }
    }
    return result;
}

Matrix3 transpose(const Matrix3 &matrix)
{
    Matrix3 result;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            result.m[r][c] = matrix.m[c][r];
    return result;
}

float determinant(const Matrix3 &matrix)
{
    return matrix.m[0][0] * (matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1]) -
           matrix.m[0][1] * (matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0]) +
           matrix.m[0][2] * (matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0]);
}

QVector3D matrixColumn(const Matrix3 &matrix, int column)
{
    return QVector3D(matrix.m[0][column], matrix.m[1][column], matrix.m[2][column]);
}

QVector3D multiply(const Matrix3 &matrix, const QVector3D &vector)
{
    return QVector3D(
        matrix.m[0][0] * vector.x() + matrix.m[0][1] * vector.y() + matrix.m[0][2] * vector.z(),
        matrix.m[1][0] * vector.x() + matrix.m[1][1] * vector.y() + matrix.m[1][2] * vector.z(),
        matrix.m[2][0] * vector.x() + matrix.m[2][1] * vector.y() + matrix.m[2][2] * vector.z());
}

Matrix3 studioLinearMatrix(const StudioTexMapData &data)
{
    Matrix3 matrix;
    matrix.m[0][0] = data.values.at(3);
    matrix.m[0][1] = data.values.at(4);
    matrix.m[0][2] = -data.values.at(5);
    matrix.m[1][0] = data.values.at(6);
    matrix.m[1][1] = data.values.at(7);
    matrix.m[1][2] = -data.values.at(8);
    matrix.m[2][0] = -data.values.at(9);
    matrix.m[2][1] = -data.values.at(10);
    matrix.m[2][2] = data.values.at(11);
    return matrix;
}

StudioTransform decomposeStudioTransform(const StudioTexMapData &data)
{
    const Matrix3 source = studioLinearMatrix(data);
    Matrix3 current = source;
    Matrix3 accumulated = identityMatrix();

    for (int d = 0; d < 3; ++d) {
        Matrix3 lower = identityMatrix();
        for (int row = d; row < 3; ++row)
            for (int column = d; column < 3; ++column)
                lower.m[row][column] = current.m[row][column];

        QVector3D vector = matrixColumn(lower, d);
        const float magnitude = vector.length();
        vector[d] -= magnitude;
        if (vector.lengthSquared() > 1.0e-12f)
            vector.normalize();
        else
            vector = QVector3D();

        Matrix3 householder = identityMatrix();
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
                householder.m[row][column] -= 2.0f * vector[row] * vector[column];

        current = multiply(householder, current);
        accumulated = multiply(householder, accumulated);
    }

    Matrix3 r = multiply(accumulated, source);
    Matrix3 rotation = transpose(accumulated);
    if (determinant(rotation) < 0.0f) {
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column) {
                rotation.m[row][column] = -rotation.m[row][column];
                r.m[row][column] = -r.m[row][column];
            }
    }

    StudioTransform result;
    result.translation = QVector3D(data.values.at(0), data.values.at(1), -data.values.at(2));
    result.rotation = rotation;
    result.scale[0] = r.m[0][0];
    result.scale[1] = r.m[1][1];
    result.scale[2] = r.m[2][2];
    return result;
}

QVector3D fromStudioCoordinates(const QVector3D &point)
{
    return QVector3D(point.x(), point.y(), -point.z());
}

QVector3D studioPointToLDraw(const StudioTransform &transform, const QVector3D &point)
{
    const QVector3D local(
        point.x() * (transform.scale[0] < 0.0f ? -1.0f : 1.0f),
        point.y() * (transform.scale[1] < 0.0f ? -1.0f : 1.0f),
        point.z() * (transform.scale[2] < 0.0f ? -1.0f : 1.0f));
    return fromStudioCoordinates(transform.translation + multiply(transform.rotation, local));
}

struct LDrawTransform
{
    float m[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
};

LDrawTransform multiply(const LDrawTransform &a, const LDrawTransform &b)
{
    LDrawTransform result;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            result.m[r][c] = 0.0f;
            for (int k = 0; k < 4; ++k)
                result.m[r][c] += a.m[r][k] * b.m[k][c];
        }
    }
    return result;
}

float determinant3(const LDrawTransform &transform)
{
    return transform.m[0][0] * (transform.m[1][1] * transform.m[2][2] - transform.m[1][2] * transform.m[2][1]) -
           transform.m[0][1] * (transform.m[1][0] * transform.m[2][2] - transform.m[1][2] * transform.m[2][0]) +
           transform.m[0][2] * (transform.m[1][0] * transform.m[2][1] - transform.m[1][1] * transform.m[2][0]);
}

QVector3D transformPoint(const LDrawTransform &transform, const QVector3D &point)
{
    const float x = point.x();
    const float y = point.y();
    const float z = point.z();
    return QVector3D(
        transform.m[0][0] * x + transform.m[0][1] * y + transform.m[0][2] * z + transform.m[0][3],
        transform.m[1][0] * x + transform.m[1][1] * y + transform.m[1][2] * z + transform.m[1][3],
        transform.m[2][0] * x + transform.m[2][1] * y + transform.m[2][2] * z + transform.m[2][3]);
}

LDrawTransform ldrawTransform(const QStringList &tokens)
{
    LDrawTransform transform;
    if (tokens.size() < 15)
        return transform;

    transform.m[0][0] = tokens.at(5).toFloat();
    transform.m[0][1] = tokens.at(6).toFloat();
    transform.m[0][2] = tokens.at(7).toFloat();
    transform.m[0][3] = tokens.at(2).toFloat();
    transform.m[1][0] = tokens.at(8).toFloat();
    transform.m[1][1] = tokens.at(9).toFloat();
    transform.m[1][2] = tokens.at(10).toFloat();
    transform.m[1][3] = tokens.at(3).toFloat();
    transform.m[2][0] = tokens.at(11).toFloat();
    transform.m[2][1] = tokens.at(12).toFloat();
    transform.m[2][2] = tokens.at(13).toFloat();
    transform.m[2][3] = tokens.at(4).toFloat();
    return transform;
}

QString normalizedName(const QString &name)
{
    QString result = name.trimmed().toLower();
    result.replace('\\', '/');
    return result;
}

QString findPartFile(const QHash<QString, QString> &partFiles, const QString &name)
{
    const QString normalized = normalizedName(name);
    if (partFiles.contains(normalized))
        return partFiles.value(normalized);

    const QString primitiveName = QStringLiteral("p/") + normalized;
    if (partFiles.contains(primitiveName))
        return partFiles.value(primitiveName);

    const QString baseName = normalized.section('/', -1);
    for (auto it = partFiles.cbegin(); it != partFiles.cend(); ++it) {
        if (normalizedName(it.key()).section('/', -1) == baseName)
            return it.value();
    }
    return QString();
}

void appendTriangle(QVector<StudioMeshTriangle> &triangles,
                    const LDrawTransform &transform,
                    const QVector3D &a,
                    const QVector3D &b,
                    const QVector3D &c,
                    bool invertWinding)
{
    StudioMeshTriangle triangle;
    triangle.p0 = transformPoint(transform, a);
    triangle.p1 = transformPoint(transform, invertWinding ? c : b);
    triangle.p2 = transformPoint(transform, invertWinding ? b : c);
    triangles.append(triangle);
}

bool readMesh(const QString &fileName,
              const LDrawTransform &parentTransform,
              const QHash<QString, QString> &partFiles,
              QVector<StudioMeshTriangle> &triangles,
              QStringList &stack,
              bool invertWinding)
{
    const QString content = findPartFile(partFiles, fileName);
    if (content.isEmpty())
        return false;

    const QString normalized = normalizedName(fileName);
    if (stack.contains(normalized))
        return false;
    stack.append(normalized);

    const QStringList lines = content.split(QLatin1Char('\n'));
    bool invertNext = false;
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith(QLatin1String("0 BFC INVERTNEXT"), Qt::CaseInsensitive)) {
            invertNext = true;
            continue;
        }
        if (line.startsWith(QLatin1String("0 ")))
            continue;

        const QStringList tokens = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (tokens.size() < 3)
            continue;

        if (tokens.at(0) == QLatin1String("1") && tokens.size() == 15) {
            const LDrawTransform localTransform = ldrawTransform(tokens);
            const LDrawTransform child = multiply(parentTransform, localTransform);
            const bool childInvertWinding = invertWinding ^ invertNext ^ (determinant3(localTransform) < 0.0f);
            readMesh(tokens.at(14), child, partFiles, triangles, stack, childInvertWinding);
            invertNext = false;
        } else if (tokens.at(0) == QLatin1String("3") && tokens.size() == 11) {
            const QVector3D p0(tokens.at(2).toFloat(), tokens.at(3).toFloat(), tokens.at(4).toFloat());
            const QVector3D p1(tokens.at(5).toFloat(), tokens.at(6).toFloat(), tokens.at(7).toFloat());
            const QVector3D p2(tokens.at(8).toFloat(), tokens.at(9).toFloat(), tokens.at(10).toFloat());
            appendTriangle(triangles, parentTransform, p2, p1, p0, invertWinding);
        } else if (tokens.at(0) == QLatin1String("4") && tokens.size() == 14) {
            const QVector3D p0(tokens.at(2).toFloat(), tokens.at(3).toFloat(), tokens.at(4).toFloat());
            const QVector3D p1(tokens.at(5).toFloat(), tokens.at(6).toFloat(), tokens.at(7).toFloat());
            const QVector3D p2(tokens.at(8).toFloat(), tokens.at(9).toFloat(), tokens.at(10).toFloat());
            const QVector3D p3(tokens.at(11).toFloat(), tokens.at(12).toFloat(), tokens.at(13).toFloat());

            // Studio stores quads in reverse LDraw order and repairs the diagonal
            // so both triangles have the same winding direction.
            QVector3D q0 = p3;
            QVector3D q1 = p2;
            QVector3D q2 = p1;
            QVector3D q3 = p0;
            const QVector3D normalA = QVector3D::crossProduct(q1 - q0, q2 - q0);
            const QVector3D normalB = QVector3D::crossProduct(q2 - q0, q3 - q0);
            if (QVector3D::dotProduct(normalA, normalB) < 0.0f)
                qSwap(q2, q3);

            appendTriangle(triangles, parentTransform, q0, q1, q2, invertWinding);
            appendTriangle(triangles, parentTransform, q0, q2, q3, invertWinding);
        }
    }

    stack.removeLast();
    return true;
}

QString formatPoint(const QVector3D &point)
{
    return QStringLiteral("%1 %2 %3")
        .arg(point.x(), 0, 'f', 6)
        .arg(point.y(), 0, 'f', 6)
        .arg(point.z(), 0, 'f', 6);
}

} // namespace

QString StudioDecalMesh::texMapStart(const StudioTexMapData &textureData, const QString &textureFile)
{
    if (!textureData.valid || textureData.values.size() != 16 || textureFile.isEmpty())
        return QString();

    const StudioTransform transform = decomposeStudioTransform(textureData);
    const float minU = textureData.pointMin.x();
    const float maxU = textureData.pointMax.x();
    const float minV = textureData.pointMin.y();
    const float maxV = textureData.pointMax.y();

    const QVector3D p0 = studioPointToLDraw(transform, QVector3D(minU, 0.0f, maxV));
    const QVector3D p1 = studioPointToLDraw(transform, QVector3D(maxU, 0.0f, maxV));
    const QVector3D p2 = studioPointToLDraw(transform, QVector3D(minU, 0.0f, minV));
    return StudioTexMap::texMapStart(p0, p1, p2, textureFile);
}

QString StudioDecalMesh::build(const StudioTexMapData &textureData,
                               const QHash<QString, QString> &partFiles,
                               const QString &basePart,
                               const QString &textureFile,
                               int *triangleCount)
{
    if (!textureData.valid || textureData.values.size() != 16 ||
        basePart.isEmpty() || textureFile.isEmpty())
        return QString();

    QVector<StudioMeshTriangle> triangles;
    QStringList stack;
    if (!readMesh(basePart, LDrawTransform(), partFiles, triangles, stack, false))
        return QString();

    const StudioTransform transform = decomposeStudioTransform(textureData);
    if (triangles.isEmpty())
        return QString();

    const float minU = textureData.pointMin.x();
    const float maxU = textureData.pointMax.x();
    const float minV = textureData.pointMin.y();
    const float maxV = textureData.pointMax.y();

    // Match Studio's inverse transform in UV space, then flip V so the PNG
    // orientation follows the same top-to-bottom convention as Studio's
    // imported texture when consumed by lclib's planar TEXMAP projection.
    const QVector3D p0 = studioPointToLDraw(transform, QVector3D(minU, 0.0f, maxV));
    const QVector3D p1 = studioPointToLDraw(transform, QVector3D(maxU, 0.0f, maxV));
    const QVector3D p2 = studioPointToLDraw(transform, QVector3D(minU, 0.0f, minV));

    QString result = StudioTexMap::texMapStart(p0, p1, p2, textureFile);
    if (result.isEmpty())
        return QString();

    for (const StudioMeshTriangle &triangle : triangles) {
        result += QLatin1Char('\n') + QStringLiteral("3 16 ") +
                  formatPoint(triangle.p0) + QLatin1Char(' ') +
                  formatPoint(triangle.p1) + QLatin1Char(' ') +
                  formatPoint(triangle.p2);
    }
    result += QStringLiteral("\n0 !TEXMAP END");

    if (triangleCount)
        *triangleCount = triangles.size();
    return result;
}
