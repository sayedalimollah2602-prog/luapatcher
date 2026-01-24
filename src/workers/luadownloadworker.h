#ifndef LUADOWNLOADWORKER_H
#define LUADOWNLOADWORKER_H

#include <QThread>
#include <QString>

class LuaDownloadWorker : public QThread {
    Q_OBJECT

public:
    explicit LuaDownloadWorker(const QString& appId, QObject* parent = nullptr);

signals:
    void finished(QString cachePath);
    void progress(qint64 downloaded, qint64 total);
    void status(QString message);
    void error(QString errorMessage);

protected:
    void run() override;

private:
    QString m_appId;
};

#endif // LUADOWNLOADWORKER_H
