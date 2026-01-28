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
        
        // Try to download
        emit progress("Syncing library...");
        
        QNetworkAccessManager manager;
        QUrl requestUrl{Config::gamesIndexUrl()};
        QNetworkRequest request{requestUrl};
        request.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
        
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
            games.append({obj["id"].toString(), obj["name"].toString()});
        }
        
        // Client-side Fallback: Check for missing/unknown names and fetch from Store API
        int missingCount = 0;
        for (const auto& game : games) {
            if (game.name.isEmpty() || game.name.startsWith("Unknown Game") || game.name == game.id) {
                missingCount++;
            }
        }

        if (missingCount > 0) {
             int fetchLimit = 50;
             int fetched = 0;
             emit progress(QString("Fetching details for %1 games...").arg(qMin(missingCount, fetchLimit)));
             QNetworkAccessManager storeMgr;
             
             for (auto& game : games) {
                 if (fetched >= fetchLimit) break;
                 if (game.name.isEmpty() || game.name.startsWith("Unknown Game") || game.name == game.id) {
                     fetched++;
                     // ... rest of the fetch logic ...
                     QUrl storeUrl("https://store.steampowered.com/api/appdetails");
                     QUrlQuery query;
                     query.addQueryItem("appids", game.id);
                     query.addQueryItem("filters", "basic");
                     storeUrl.setQuery(query);
                     
                     QNetworkRequest req(storeUrl);
                     req.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
                     
                     QEventLoop storeLoop;
                     QNetworkReply* storeReply = storeMgr.get(req);
                     QTimer storeTimer;
                     storeTimer.setSingleShot(true);
                     connect(&storeTimer, &QTimer::timeout, &storeLoop, &QEventLoop::quit); 
                     connect(storeReply, &QNetworkReply::finished, &storeLoop, &QEventLoop::quit);
                     storeTimer.start(5000); 
                     
                     storeLoop.exec();
                     
                     if (storeReply->error() == QNetworkReply::NoError) {
                         QJsonDocument storeDoc = QJsonDocument::fromJson(storeReply->readAll());
                         QJsonObject storeRoot = storeDoc.object();
                         if (storeRoot.contains(game.id)) {
                             QJsonObject appData = storeRoot[game.id].toObject();
                             if (appData["success"].toBool()) {
                                 QString newName = appData["data"].toObject()["name"].toString();
                                 if (!newName.isEmpty()) {
                                     game.name = newName;
                                 }
                             }
                         }
                     }
                     storeReply->deleteLater();
                     QThread::msleep(200);
                 }
             }
        }
        
        emit finished(games);

        
    } catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }
}
