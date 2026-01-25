#ifndef INDEXDOWNLOADWORKER_H
#define INDEXDOWNLOADWORKER_H

#include <QThread>
#include <QSet>
#include "../utils/gameinfo.h"

#include <QString>

class IndexDownloadWorker : public QThread {
    Q_OBJECT

public:
    explicit IndexDownloadWorker(QObject* parent = nullptr);

signals:
    void finished(QList<GameInfo> games);

    void progress(QString message);
    void error(QString errorMessage);

protected:
    void run() override;
};

#endif // INDEXDOWNLOADWORKER_H
