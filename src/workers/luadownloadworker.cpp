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
        emit log("Starting patch process...", "INFO");
        emit status("Downloading patch...");
        
        // Build URL
        QString url = Config::luaFileUrl() + m_appId + ".lua";
        QString cachePath = QDir(Paths::getLocalCacheDir()).filePath(m_appId + ".lua");
        
        emit log(QString("Target App ID: %1").arg(m_appId), "INFO");
        emit log(QString("Download URL: %1").arg(url), "INFO");
        emit log(QString("Cache path: %1").arg(cachePath), "INFO");
        
        // Ensure cache directory exists
        QDir cacheDir(Paths::getLocalCacheDir());
        if (!cacheDir.exists()) {
            emit log("Creating cache directory...", "INFO");
            cacheDir.mkpath(".");
        }
        
        emit log("Initializing network request...", "INFO");
        QNetworkAccessManager manager;
        QUrl qurl{url};
        QNetworkRequest request{qurl};
        request.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
        request.setRawHeader("X-Access-Token", Config::getAccessToken().toUtf8());
        
        emit log("Connecting to server...", "INFO");
        QEventLoop loop;
        QNetworkReply* reply = manager.get(request);
        
        // Connect progress
        connect(reply, &QNetworkReply::downloadProgress, 
                [this](qint64 received, qint64 total) {
                    emit progress(received, total);
                    if (total > 0) {
                        int percent = static_cast<int>(received * 100 / total);
                        if (percent % 25 == 0 && received > 0) {  // Log at 25%, 50%, 75%, 100%
                            emit log(QString("Download progress: %1%").arg(percent), "INFO");
                        }
                    }
                });
        
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000); // 30 second timeout
        
        emit log("Downloading Lua patch file...", "INFO");
        loop.exec();
        
        if (!timer.isActive()) {
            emit log("Download timed out after 30 seconds", "ERROR");
            throw std::runtime_error("Connection timed out");
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            emit log(QString("Network error: %1").arg(reply->errorString()), "ERROR");
            throw std::runtime_error(reply->errorString().toStdString());
        }
        
        emit log("Download completed successfully", "SUCCESS");
        
        // Save to cache
        QByteArray data = reply->readAll();
        emit log(QString("Received %1 bytes").arg(data.size()), "INFO");
        
        emit log(QString("Writing to cache: %1").arg(cachePath), "INFO");
        QFile file(cachePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit log("Failed to open cache file for writing", "ERROR");
            throw std::runtime_error("Failed to write cache file");
        }
        
        file.write(data);
        file.close();
        reply->deleteLater();
        
        emit log("Cache file written successfully", "SUCCESS");
        emit finished(cachePath);
        
    } catch (const std::exception& e) {
        emit log(QString("Error: %1").arg(e.what()), "ERROR");
        emit error(QString::fromStdString(e.what()));
    }
}