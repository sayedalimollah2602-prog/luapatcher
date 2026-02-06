#ifndef FIXDOWNLOADWORKER_H
#define FIXDOWNLOADWORKER_H

#include <QThread>
#include <QString>

class FixDownloadWorker : public QThread {
    Q_OBJECT

public:
    explicit FixDownloadWorker(const QString& appId, const QString& targetPath, QObject* parent = nullptr);

signals:
    void progress(qint64 downloaded, qint64 total);
    void status(QString message);
    void log(QString message, QString level);
    void finished(QString path);
    void error(QString message);

protected:
    void run() override;

private:
    QString m_appId;
    QString m_targetPath;
    
    bool extractZip(const QString& zipPath, const QString& destPath);
};

#endif // FIXDOWNLOADWORKER_H
