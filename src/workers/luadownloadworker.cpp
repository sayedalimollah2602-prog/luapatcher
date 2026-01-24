#include "luadownloadworker.h"
#include "../config.h"
#include "../utils/paths.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QTimer>

LuaDownloadWorker::LuaDownloadWorker(const QString& appId, QObject* parent)
    : QThread(parent)
    , m_appId(appId)
{
}

void LuaDownloadWorker::run() {
    try {
        emit status("Downloading patch...");
        
        QString url = Config::LUA_FILE_URL + m_appId + ".lua";
        QString cachePath = QDir(Paths::getLocalCacheDir()).filePath(m_appId + ".lua");
        
        QNetworkAccessManager manager;
        QUrl qurl(url);
        QNetworkRequest request(qurl);
        request.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
        
        QEventLoop loop;
        QNetworkReply* reply = manager.get(request);
        
        // Connect progress
        connect(reply, &QNetworkReply::downloadProgress, 
                [this](qint64 received, qint64 total) {
                    emit progress(received, total);
                });
        
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000); // 30 second timeout
        
        loop.exec();
        
        if (reply->error() != QNetworkReply::NoError || !timer.isActive()) {
            throw std::runtime_error(reply->errorString().toStdString());
        }
        
        // Save to cache
        QByteArray data = reply->readAll();
        QFile file(cachePath);
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Failed to write cache file");
        }
        
        file.write(data);
        file.close();
        reply->deleteLater();
        
        emit finished(cachePath);
        
    } catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }
}