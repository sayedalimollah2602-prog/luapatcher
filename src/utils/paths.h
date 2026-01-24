#ifndef PATHS_H
#define PATHS_H

#include <QString>

class Paths {
public:
    static QString getResourcePath(const QString& relativePath);
    static QString getLocalCacheDir();
    static QString getLocalIndexPath();
};

#endif // PATHS_H
