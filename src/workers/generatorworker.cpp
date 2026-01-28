#include "generatorworker.h"
#include "../utils/paths.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QUrl>

GeneratorWorker::GeneratorWorker(const QString& appId, QObject* parent)
    : QThread(parent)
    , m_appId(appId)
{
}

void GeneratorWorker::run() {
    try {
        emit log("Starting generation process...", "INFO");
        emit status("Fetching game data...");
        
        // Build URL
        QString url = QString("https://crackworld.vercel.app/api/free-download?appid=%1&user=luamanifest").arg(m_appId);
        QString cacheDirStr = Paths::getLocalCacheDir();
        QString archivePath = QDir(cacheDirStr).filePath(m_appId + "_gen.zip");
        QString extractDir = QDir(cacheDirStr).filePath(m_appId + "_gen");
        
        emit log(QString("Target App ID: %1").arg(m_appId), "INFO");
        emit log(QString("Request URL: %1").arg(url), "INFO");
        
        // Ensure cache directory exists
        QDir cacheDir(cacheDirStr);
        if (!cacheDir.exists()) {
            cacheDir.mkpath(".");
        }
        
        // Clean up previous runs
        if (QFile::exists(archivePath)) QFile::remove(archivePath);
        if (QDir(extractDir).exists()) QDir(extractDir).removeRecursively();
        
        emit log("Sending request...", "INFO");
        QNetworkAccessManager manager;
        QNetworkRequest request;
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::UserAgentHeader, "genshinreya");
        
        QEventLoop loop;
        QNetworkReply* reply = manager.get(request);
        
        connect(reply, &QNetworkReply::downloadProgress, 
                [this](qint64 received, qint64 total) {
                    emit progress(received, total);
                });
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        // Timeout
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000); 
        
        loop.exec();
        
        if (timer.isActive()) {
            timer.stop();
        } else {
             emit log("Request timed out", "ERROR");
             throw std::runtime_error("Connection timed out");
        }
        
        if (reply->error() != QNetworkReply::NoError) {
             emit log(QString("Network error: %1").arg(reply->errorString()), "ERROR");
             throw std::runtime_error(reply->errorString().toStdString());
        }
        
        QByteArray data = reply->readAll();
        reply->deleteLater();
        
        if (data.startsWith("PK")) {
            emit log("Received ZIP archive. Saving...", "INFO");
            QFile file(archivePath);
            if (!file.open(QIODevice::WriteOnly)) {
                throw std::runtime_error("Failed to save zip file");
            }
            file.write(data);
            file.close();
            
            // Extract using PowerShell
            emit log("Extracting archive...", "INFO");
            QProcess process;
            // PowerShell command: Expand-Archive -Path "..." -DestinationPath "..." -Force
            QString cmd = QString("Expand-Archive -Path \"%1\" -DestinationPath \"%2\" -Force").arg(archivePath).arg(extractDir);
            
            process.start("powershell", QStringList() << "-Command" << cmd);
            if (!process.waitForFinished(10000)) {
                 throw std::runtime_error("Extraction timed out");
            }
            if (process.exitCode() != 0) {
                QString err = process.readAllStandardError();
                emit log("Extraction failed: " + err, "ERROR");
                 throw std::runtime_error("Failed to extract archive");
            }
            
            // Find Lua file
            QDir dir(extractDir);
            QStringList filters;
            filters << "*.lua";
            dir.setNameFilters(filters);
            QStringList files = dir.entryList(QDir::Files);
            
            if (files.isEmpty()) {
                throw std::runtime_error("No .lua file found in the archive");
            }
            
            QString luaFile = dir.filePath(files.first());
            emit log(QString("Found Lua file: %1").arg(files.first()), "SUCCESS");
            
            emit finished(luaFile);
            
        } else {
            // Not a zip?
            emit log("Response is not a ZIP archive.", "WARN");
            emit log(QString("Data: %1").arg(QString(data.left(200))), "WARN");
            throw std::runtime_error("Unexpected response format (not a ZIP)");
        }
        
    } catch (const std::exception& e) {
        emit log(QString("Error: %1").arg(e.what()), "ERROR");
        emit error(QString::fromStdString(e.what()));
    }
}
