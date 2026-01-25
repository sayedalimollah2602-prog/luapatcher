#ifndef LUAGENERATIONWORKER_H
#define LUAGENERATIONWORKER_H

#include <QObject>
#include <QWebEnginePage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class LuaGenerationWorker : public QObject
{
    Q_OBJECT
public:
    explicit LuaGenerationWorker(const QString& appId, QObject* parent = nullptr);
    ~LuaGenerationWorker();

    void start();

signals:
    void finished(QString filePath);
    void error(QString message);
    void status(QString message);

private slots:
    void onPageLoadFinished(bool success);
    void onJsResult(const QVariant& result);
    void onDownloadFinished();

private:
    QString m_appId;
    QWebEnginePage* m_page;
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_reply;
    
    void runGeneration();
    void downloadFile(const QString& url);
};

#endif // LUAGENERATIONWORKER_H
