#include "paths.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

QString Paths::getResourcePath(const QString& relativePath) {
    QString basePath = QCoreApplication::applicationDirPath();
    return QDir(basePath).filePath(relativePath);
}

QString Paths::getLocalCacheDir() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appData);
    dir.mkpath("SteamLuaPatcher");
    return dir.filePath("SteamLuaPatcher");
}

QString Paths::getLocalIndexPath() {
    return QDir(getLocalCacheDir()).filePath("games_index.json");
}
