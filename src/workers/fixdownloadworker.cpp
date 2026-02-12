#include "fixdownloadworker.h"
#include "../config.h"
#include "../utils/paths.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QProcess>
#include <QTemporaryFile>

FixDownloadWorker::FixDownloadWorker(const QString& appId, const QString& targetPath, QObject* parent)
    : QThread(parent)
    , m_appId(appId)
    , m_targetPath(targetPath)
{
}

void FixDownloadWorker::run() {
    try {
        emit log("Starting game fix download...", "INFO");
        emit status("Downloading fix...");
        
        // Build URL
        QString url = Config::gameFixUrl() + m_appId + ".zip";
        QString tempPath = QDir(Paths::getLocalCacheDir()).filePath(m_appId + "_fix.zip");
        
        emit log(QString("Target App ID: %1").arg(m_appId), "INFO");
        emit log(QString("Download URL: %1").arg(url), "INFO");
        emit log(QString("Temp path: %1").arg(tempPath), "INFO");
        emit log(QString("Target path: %1").arg(m_targetPath), "INFO");
        
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
                        if (percent % 25 == 0 && received > 0) {
                            emit log(QString("Download progress: %1%").arg(percent), "INFO");
                        }
                    }
                });
        
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(120000); // 120 second timeout for larger files
        
        emit log("Downloading fix zip file...", "INFO");
        loop.exec();
        
        if (!timer.isActive()) {
            emit log("Download timed out after 120 seconds", "ERROR");
            throw std::runtime_error("Connection timed out");
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            emit log(QString("Network error: %1").arg(reply->errorString()), "ERROR");
            throw std::runtime_error(reply->errorString().toStdString());
        }
        
        emit log("Download completed successfully", "SUCCESS");
        
        // Save to temp
        QByteArray data = reply->readAll();
        emit log(QString("Received %1 bytes").arg(data.size()), "INFO");
        
        emit log(QString("Writing temp file: %1").arg(tempPath), "INFO");
        QFile file(tempPath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit log("Failed to open temp file for writing", "ERROR");
            throw std::runtime_error("Failed to write temp file");
        }
        
        file.write(data);
        file.close();
        reply->deleteLater();
        
        emit log("Temp file written successfully", "SUCCESS");
        
        // Extract zip
        emit status("Extracting fix...");
        emit log(QString("Extracting to: %1").arg(m_targetPath), "INFO");
        
        if (!extractZip(tempPath, m_targetPath)) {
            emit log("Failed to extract zip file", "ERROR");
            throw std::runtime_error("Failed to extract zip file");
        }
        
        // Clean up temp file
        QFile::remove(tempPath);
        emit log("Temp file cleaned up", "INFO");
        
        emit log("Game fix applied successfully!", "SUCCESS");
        emit finished(m_targetPath);
        
    } catch (const std::exception& e) {
        emit log(QString("Error: %1").arg(e.what()), "ERROR");
        emit error(QString::fromStdString(e.what()));
    }
}

bool FixDownloadWorker::extractZip(const QString& zipPath, const QString& destPath) {
    emit log("Using PowerShell to extract zip...", "INFO");
    
    // Use PowerShell's Expand-Archive for Windows (built-in, no dependencies)
    QProcess process;
    
    // Escape paths for PowerShell
    QString escapedZip = zipPath;
    escapedZip.replace("'", "''");
    QString escapedDest = destPath;
    escapedDest.replace("'", "''");
    
    QStringList args;
    args << "-NoProfile" << "-NonInteractive" << "-Command";
    args << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
            .arg(escapedZip)
            .arg(escapedDest);
    
    emit log(QString("Running: powershell %1").arg(args.join(" ")), "INFO");
    
    process.start("powershell.exe", args);
    
    if (!process.waitForStarted(10000)) {
        emit log("Failed to start PowerShell", "ERROR");
        return false;
    }
    
    if (!process.waitForFinished(60000)) { // 60 second timeout
        emit log("PowerShell extraction timed out", "ERROR");
        process.kill();
        return false;
    }
    
    int exitCode = process.exitCode();
    QString stdOut = QString::fromUtf8(process.readAllStandardOutput());
    QString stdErr = QString::fromUtf8(process.readAllStandardError());
    
    if (!stdOut.isEmpty()) {
        emit log(QString("PowerShell output: %1").arg(stdOut.trimmed()), "INFO");
    }
    
    if (exitCode != 0) {
        emit log(QString("PowerShell error (exit code %1): %2").arg(exitCode).arg(stdErr.trimmed()), "ERROR");
        return false;
    }
    
    emit log("Extraction completed successfully", "SUCCESS");
    return true;
}
