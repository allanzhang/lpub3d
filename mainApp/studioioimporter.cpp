#include "studioioimporter.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QUuid>
#include <QSet>

#include "studiotexmap.h"

#include <quazip.h>
#include <quazipfile.h>

namespace {

constexpr char kDefaultArchivePassword[] = "soho0909";
constexpr char kCustomPartMarker[] = "0 !LDRAW_ORG Unofficial_Part";

QString normalizedEntryName(const QString &name)
{
    QString normalized = name;
    normalized.replace('\\', '/');
    normalized = QDir::cleanPath(normalized);
    while (normalized.startsWith('/'))
        normalized.remove(0, 1);
    return normalized;
}

bool isSafeEntryName(const QString &name)
{
    const QString normalized = normalizedEntryName(name);
    if (normalized.isEmpty() || normalized == QLatin1String("."))
        return false;
    if (normalized == QLatin1String("..") || normalized.startsWith(QLatin1String("../")))
        return false;
    if (normalized.size() >= 2 && normalized.at(1) == QLatin1Char(':'))
        return false;
    return true;
}

QString findEntry(const QStringList &entries, const QString &candidate)
{
    for (const QString &entry : entries) {
        if (entry.compare(candidate, Qt::CaseInsensitive) == 0)
            return entry;
    }
    return QString();
}

bool openEntry(QuaZip &zip, const QString &entryName, QuaZipFile &source)
{
    if (!zip.setCurrentFile(entryName))
        return false;

    QuaZipFileInfo64 fileInfo;
    if (!zip.getCurrentFileInfo(&fileInfo))
        return false;

    if (fileInfo.isEncrypted())
        return source.open(QIODevice::ReadOnly, kDefaultArchivePassword);
    return source.open(QIODevice::ReadOnly);
}

bool readEntryText(QuaZip &zip, const QString &entryName, QString &text)
{
    QuaZipFile source(&zip);
    if (!openEntry(zip, entryName, source))
        return false;

    QByteArray data = source.readAll();
    if (source.getZipError() != 0 || data.isNull())
        return false;

    if (data.startsWith("\xEF\xBB\xBF"))
        data.remove(0, 3);
    text = QString::fromUtf8(data);
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return true;
}

bool writeTextFile(const QString &filePath, const QString &text)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(text.toUtf8()) == text.toUtf8().size() && file.flush();
}

bool writeBinaryFile(const QString &filePath, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(data) == data.size() && file.flush();
}

QString studioTextureDirective(const StudioTexMapData &textureData, const QString &textureFile)
{
    QString directive = QStringLiteral("0 !STUDIO_TEXMAP START PLANAR");
    for (const float value : textureData.values)
        directive += QStringLiteral(" %1").arg(value, 0, 'f', 6);
    directive += QLatin1Char(' ') + textureFile;
    return directive;
}

QString wrapStudioTextureReference(const QString &customPart,
                                   const StudioTexMapData &textureData,
                                   const QString &textureFile)
{
    QStringList lines = customPart.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    for (qsizetype index = 0; index < lines.size(); ++index) {
        const QString trimmed = lines.at(index).trimmed();
        if (!trimmed.startsWith(QLatin1String("1 ")))
            continue;

        const QStringList tokens = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (tokens.size() != 15)
            continue;

        lines.insert(index, studioTextureDirective(textureData, textureFile));
        lines.insert(index + 2, QStringLiteral("0 !STUDIO_TEXMAP END"));
        return lines.join(QLatin1Char('\n'));
    }

    return customPart;
}

QString addCustomPartHeader(QString content)
{
    if (content.contains(QLatin1String(kCustomPartMarker), Qt::CaseInsensitive))
        return content;

    const QString nameMarker = QStringLiteral("0 Name:");
    const qsizetype namePosition = content.indexOf(nameMarker, 0, Qt::CaseInsensitive);
    if (namePosition < 0)
        return content;

    qsizetype lineEnd = content.indexOf(QLatin1Char('\n'), namePosition);
    if (lineEnd < 0)
        lineEnd = content.size();
    content.insert(lineEnd, QStringLiteral("\n") + QString::fromLatin1(kCustomPartMarker));
    return content;
}

StudioIoImportResult failure(const QString &sourceFilePath, const QString &cacheDir, const QString &message)
{
    StudioIoImportResult result;
    result.sourceFilePath = sourceFilePath;
    result.cacheDir = cacheDir;
    result.errorMessage = message;
    if (!cacheDir.isEmpty())
        QDir(cacheDir).removeRecursively();
    return result;
}

} // namespace

bool StudioIoImporter::isStudioProject(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QLatin1String("io") || suffix == QLatin1String("mo");
}

StudioIoImportResult StudioIoImporter::import(const QString &filePath)
{
    const QFileInfo sourceInfo(filePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable())
        return failure(filePath, QString(), QObject::tr("Studio project file is not readable: %1").arg(filePath));
    if (!isStudioProject(filePath))
        return failure(filePath, QString(), QObject::tr("File is not a Studio IO project: %1").arg(filePath));

    QuaZip zip(filePath);
    if (!zip.open(QuaZip::mdUnzip))
        return failure(filePath, QString(), QObject::tr("Unable to open Studio project archive: %1").arg(filePath));

    const QStringList entries = zip.getFileNameList();
    const QString standardModel = findEntry(entries, QStringLiteral("model.ldr"));
    const QString embeddedModel = findEntry(entries, QStringLiteral("model2.ldr"));
    if (standardModel.isEmpty() && embeddedModel.isEmpty()) {
        zip.close();
        return failure(filePath, QString(), QObject::tr("Studio project contains no loadable model: %1").arg(filePath));
    }

    const QString cacheRoot = QDir::tempPath() + QStringLiteral("/myLPub3D-studio-io");
    const QString cacheDir = cacheRoot + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!QDir().mkpath(cacheDir)) {
        zip.close();
        return failure(filePath, cacheDir, QObject::tr("Unable to create Studio project cache: %1").arg(cacheDir));
    }

    StudioIoImportResult result;
    result.sourceFilePath = sourceInfo.absoluteFilePath();
    result.cacheDir = QDir::cleanPath(cacheDir);

    if (!standardModel.isEmpty()) {
        QString topLevel;
        if (!readEntryText(zip, standardModel, topLevel)) {
            zip.close();
            return failure(filePath, result.cacheDir, QObject::tr("Unable to read Studio model: %1").arg(standardModel));
        }

        QHash<QString, QString> partFiles;
        for (const QString &entry : entries) {
            const QString normalized = normalizedEntryName(entry);
            if (!normalized.startsWith(QLatin1String("CustomParts/"), Qt::CaseInsensitive) ||
                !normalized.endsWith(QLatin1String(".dat"), Qt::CaseInsensitive))
                continue;

            QString partText;
            if (!readEntryText(zip, entry, partText))
                continue;
            const QString key = normalized.mid(QStringLiteral("CustomParts/").size()).toLower();
            partFiles.insert(key, partText);
            const QString baseName = key.section(QLatin1Char('/'), -1);
            if (!partFiles.contains(baseName))
                partFiles.insert(baseName, partText);
        }

        QString combinedModel = topLevel.trimmed() + QLatin1Char('\n');
        for (const QString &entry : entries) {
            const QString normalized = normalizedEntryName(entry);
            if (!normalized.startsWith(QLatin1String("CustomParts/"), Qt::CaseInsensitive))
                continue;
            if (!normalized.endsWith(QLatin1String(".dat"), Qt::CaseInsensitive))
                continue;
            if (normalized.startsWith(QLatin1String("CustomParts/p/"), Qt::CaseInsensitive))
                continue;
            if (!isSafeEntryName(normalized))
                continue;

            QString customPart;
            if (!readEntryText(zip, entry, customPart)) {
                zip.close();
                return failure(filePath, result.cacheDir,
                               QObject::tr("Unable to read Studio custom part: %1").arg(entry));
            }

            if (customPart.contains(QLatin1String("0 CustomBrick"), Qt::CaseInsensitive))
                customPart = addCustomPartHeader(customPart);

            const QStringList lines = customPart.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
            for (const QString &line : lines) {
                if (!line.trimmed().startsWith(QLatin1String("0 PE_TEX_INFO"), Qt::CaseInsensitive))
                    continue;

                const StudioTexMapData textureData = StudioTexMap::parse(line);
                if (!textureData.valid) {
                    result.warnings << QObject::tr("Skipped invalid Studio texture in %1: %2")
                                           .arg(entry, textureData.errorMessage);
                    break;
                }

                const QString textureName =
                    QStringLiteral("DECAL_") +
                    QString::fromLatin1(QCryptographicHash::hash(textureData.pngData,
                                                                 QCryptographicHash::Sha256).toHex()).toUpper();
                const QString textureRelative = QStringLiteral("textures/") + textureName;
                const QString texturePath = result.cacheDir + QLatin1Char('/') + textureRelative + QStringLiteral(".png");
                if (!writeBinaryFile(texturePath, textureData.pngData)) {
                    result.warnings << QObject::tr("Unable to write Studio texture for %1.").arg(entry);
                    break;
                }
                result.customSearchDirs << result.cacheDir
                                        << result.cacheDir + QStringLiteral("/textures");
                customPart = wrapStudioTextureReference(customPart, textureData, textureRelative);
                break;
            }

            combinedModel += QLatin1Char('\n') + customPart.trimmed() + QLatin1Char('\n');
        }

        result.modelFilePath = result.cacheDir + QStringLiteral("/studio-io-import.ldr");
        if (!writeTextFile(result.modelFilePath, combinedModel)) {
            zip.close();
            return failure(filePath, result.cacheDir, QObject::tr("Unable to write imported Studio model."));
        }
    } else {
        QuaZipFile source(&zip);
        if (!openEntry(zip, embeddedModel, source)) {
            zip.close();
            return failure(filePath, result.cacheDir,
                           QObject::tr("Unable to read embedded Studio model: %1").arg(embeddedModel));
        }
        result.modelFilePath = result.cacheDir + QStringLiteral("/model2.ldr");
        QFile output(result.modelFilePath);
        if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            zip.close();
            return failure(filePath, result.cacheDir, QObject::tr("Unable to write embedded Studio model."));
        }
        const QByteArray data = source.readAll();
        if (source.getZipError() != 0 || output.write(data) != data.size()) {
            zip.close();
            return failure(filePath, result.cacheDir, QObject::tr("Unable to extract embedded Studio model."));
        }
        output.close();
    }

    zip.close();
    result.success = true;
    return result;
}

bool StudioIoImporter::removeCache(const StudioIoImportResult &result)
{
    if (result.cacheDir.isEmpty())
        return true;
    return QDir(result.cacheDir).removeRecursively();
}
