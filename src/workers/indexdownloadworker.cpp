#include "indexdownloadworker.h"
#include "../config.h"
#include "../utils/paths.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QUrlQuery>
#include <QDateTime>

IndexDownloadWorker::IndexDownloadWorker(QObject* parent)
    : QThread(parent)
{
}

void IndexDownloadWorker::run() {
    try {
        emit progress("Connecting...");
        
        QString cacheDir = Paths::getLocalCacheDir();
        QDir dir;
        dir.mkpath(cacheDir);
        
        QString indexPath = Paths::getLocalIndexPath();
        QJsonObject indexData;

        // Explicitly remove old cache as requested
        if (QFile::exists(indexPath)) {
            QFile::remove(indexPath);
        }

        // Try to download
        emit progress("Syncing library...");
        
        QNetworkAccessManager manager;
        // Add timestamp to query to prevent server-side caching
        QString urlStr = Config::gamesIndexUrl();
        if (urlStr.contains("?")) {
            urlStr += "&_t=" + QString::number(QDateTime::currentMSecsSinceEpoch());
        } else {
            urlStr += "?_t=" + QString::number(QDateTime::currentMSecsSinceEpoch());
        }

        QUrl requestUrl{urlStr};
        QNetworkRequest request{requestUrl};
        request.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
        request.setRawHeader("X-Access-Token", Config::ACCESS_TOKEN.toUtf8());
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        
        QEventLoop loop;
        QNetworkReply* reply = manager.get(request);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000); // 30 second timeout
        
        loop.exec();
        
        if (reply->error() == QNetworkReply::NoError && timer.isActive()) {
            // Download successful
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            indexData = doc.object();
            
            // Save to cache
            QFile file(indexPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(doc.toJson());
                file.close();
            }
        } else {
            // Network error, try cache
            emit progress("Offline mode...");
            QFile file(indexPath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                indexData = doc.object();
                file.close();
            } else {
                throw std::runtime_error("Network error & no cache");
            }
        }
        
        reply->deleteLater();
        
        // Extract app IDs
        // Extract games
        QList<GameInfo> games;
        QJsonArray arr = indexData["games"].toArray();
        for (const QJsonValue& val : arr) {
            QJsonObject obj = val.toObject();
            GameInfo game;
            game.id = obj["id"].toString();
            game.name = obj["name"].toString();
            game.thumbnailUrl = ""; // Will be generated when needed
            game.hasFix = obj["has_fix"].toBool(false);
            games.append(game);
        }
        
        emit finished(games);

        
    } catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }
}
