#ifndef STUDIOIOIMPORTER_H
#define STUDIOIOIMPORTER_H

#include <QString>
#include <QStringList>

struct StudioIoImportResult
{
    bool success = false;
    QString sourceFilePath;
    QString modelFilePath;
    QString cacheDir;
    QStringList customSearchDirs;
    QStringList warnings;
    QString errorMessage;
};

class StudioIoImporter
{
public:
    static bool isStudioProject(const QString &filePath);
    static StudioIoImportResult import(const QString &filePath);
    static bool removeCache(const StudioIoImportResult &result);
};

#endif // STUDIOIOIMPORTER_H
