#ifndef INDEXDOWNLOADWORKER_H
#define INDEXDOWNLOADWORKER_H

#include <QThread>
#include <QSet>
#include <QString>

class IndexDownloadWorker : public QThread {
    Q_OBJECT

public:
    explicit IndexDownloadWorker(QObject* parent = nullptr);

signals:
    void finished(QSet<QString> appIds);
    void progress(QString message);
    void error(QString errorMessage);

protected:
    void run() override;
};

#endif // INDEXDOWNLOADWORKER_H
