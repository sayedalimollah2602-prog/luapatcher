#include "generatorworker.h"
#include "../utils/paths.h"
#include "../config.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QDirIterator>
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
        emit log(QString("Cache directory: %1").arg(cacheDirStr), "INFO");
        
        // Ensure cache directory exists
        QDir cacheDir(cacheDirStr);
        if (!cacheDir.exists()) {
            if (!cacheDir.mkpath(".")) {
                emit log("Failed to create cache directory", "ERROR");
                throw std::runtime_error("Failed to create cache directory");
            }
        }
        emit log("Cache directory ready", "INFO");
        
        // Clean up previous runs
        if (QFile::exists(archivePath)) {
            emit log("Removing previous archive...", "INFO");
            QFile::remove(archivePath);
        }
        if (QDir(extractDir).exists()) {
            emit log("Removing previous extraction directory...", "INFO");
            QDir(extractDir).removeRecursively();
        }
        
        emit log("Sending HTTP request...", "INFO");
        QNetworkAccessManager manager;
        QNetworkRequest request;
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::UserAgentHeader, "genshinreya");
        request.setRawHeader("Accept", "*/*");
        
        QEventLoop loop;
        QNetworkReply* reply = manager.get(request);
        
        connect(reply, &QNetworkReply::downloadProgress, 
                [this](qint64 received, qint64 total) {
                    emit progress(received, total);
                    if (total > 0) {
                        emit log(QString("Downloading: %1 / %2 bytes").arg(received).arg(total), "INFO");
                    }
                });
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        // Timeout - 60 seconds for slower connections
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(60000); 
        
        loop.exec();
        
        if (timer.isActive()) {
            timer.stop();
        } else {
            emit log("Request timed out after 60 seconds", "ERROR");
            reply->abort();
            reply->deleteLater();
            throw std::runtime_error("Connection timed out");
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            QString errorStr = reply->errorString();
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            emit log(QString("Network error (HTTP %1): %2").arg(httpStatus).arg(errorStr), "ERROR");
            reply->deleteLater();
            throw std::runtime_error(errorStr.toStdString());
        }
        
        QByteArray data = reply->readAll();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit log(QString("Response received: HTTP %1, %2 bytes").arg(httpStatus).arg(data.size()), "INFO");
        reply->deleteLater();
        
        if (data.isEmpty()) {
            emit log("Response is empty", "ERROR");
            throw std::runtime_error("Empty response from server");
        }
        
        // Check if response is a ZIP (starts with PK)
        if (data.startsWith("PK")) {
            emit log("Received ZIP archive. Saving to disk...", "INFO");
            
            QFile file(archivePath);
            if (!file.open(QIODevice::WriteOnly)) {
                emit log(QString("Failed to open file for writing: %1").arg(archivePath), "ERROR");
                throw std::runtime_error("Failed to save zip file");
            }
            qint64 written = file.write(data);
            file.close();
            
            emit log(QString("Archive saved: %1 bytes written to %2").arg(written).arg(archivePath), "INFO");
            
            // Create extraction directory
            QDir extractDirObj(extractDir);
            if (!extractDirObj.exists()) {
                if (!extractDirObj.mkpath(".")) {
                    emit log("Failed to create extraction directory", "ERROR");
                    throw std::runtime_error("Failed to create extraction directory");
                }
            }
            
            // Extract using PowerShell (Windows)
            emit log("Extracting archive using PowerShell...", "INFO");
            QProcess process;
            process.setProcessChannelMode(QProcess::MergedChannels);
            
            // Use native path separators for Windows
            QString nativeArchivePath = QDir::toNativeSeparators(archivePath);
            QString nativeExtractDir = QDir::toNativeSeparators(extractDir);
            
            QString cmd = QString("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
                .arg(nativeArchivePath)
                .arg(nativeExtractDir);
            
            emit log(QString("PowerShell command: %1").arg(cmd), "INFO");
            
            process.start("powershell", QStringList() << "-NoProfile" << "-NonInteractive" << "-Command" << cmd);
            
            if (!process.waitForStarted(5000)) {
                emit log("Failed to start PowerShell process", "ERROR");
                throw std::runtime_error("Failed to start extraction process");
            }
            
            if (!process.waitForFinished(30000)) {
                emit log("Extraction process timed out", "ERROR");
                process.kill();
                throw std::runtime_error("Extraction timed out");
            }
            
            QString processOutput = QString::fromUtf8(process.readAll());
            if (!processOutput.isEmpty()) {
                emit log(QString("PowerShell output: %1").arg(processOutput.trimmed()), "INFO");
            }
            
            if (process.exitCode() != 0) {
                emit log(QString("Extraction failed with exit code: %1").arg(process.exitCode()), "ERROR");
                throw std::runtime_error("Failed to extract archive");
            }
            
            emit log("Archive extracted successfully", "SUCCESS");
            
            // Find Lua file in extraction directory
            QDir dir(extractDir);
            QStringList filters;
            filters << "*.lua";
            dir.setNameFilters(filters);
            
            // Search recursively
            QStringList files = dir.entryList(QDir::Files);
            
            // If no files found at top level, search subdirectories
            if (files.isEmpty()) {
                emit log("No Lua files at top level, searching subdirectories...", "INFO");
                QDirIterator it(extractDir, filters, QDir::Files, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    QString foundFile = it.next();
                    files.append(foundFile);
                    emit log(QString("Found: %1").arg(foundFile), "INFO");
                }
            } else {
                // Convert to full paths
                QStringList fullPaths;
                for (const QString& f : files) {
                    fullPaths.append(dir.filePath(f));
                }
                files = fullPaths;
            }
            
            if (files.isEmpty()) {
                emit log("No .lua file found in the archive", "ERROR");
                throw std::runtime_error("No .lua file found in the archive");
            }
            
            QString luaFile = files.first();
            emit log(QString("Found Lua file: %1").arg(luaFile), "SUCCESS");
            
            // Now copy directly to plugin folder instead of just returning path
            QStringList targetDirs = Config::getAllSteamPluginDirs();
            if (targetDirs.isEmpty()) {
                targetDirs.append(Config::getSteamPluginDir());
                emit log("No plugin paths found, using default path", "WARN");
            }
            
            bool atLeastOneSuccess = false;
            QString destFile;
            
            for (const QString& pluginDir : targetDirs) {
                emit log(QString("Checking plugin folder: %1").arg(pluginDir), "INFO");
                
                QDir pDir(pluginDir);
                if (!pDir.exists()) {
                    emit log(QString("Creating plugin folder: %1").arg(pluginDir), "INFO");
                    if (!pDir.mkpath(".")) {
                        emit log(QString("Failed to create folder: %1").arg(pluginDir), "WARN");
                        continue;
                    }
                }
                
                destFile = pDir.filePath(m_appId + ".lua");
                
                // Remove existing file
                if (QFile::exists(destFile)) {
                    emit log(QString("Removing existing: %1").arg(destFile), "INFO");
                    QFile::remove(destFile);
                }
                
                // Copy the lua file
                emit log(QString("Copying to: %1").arg(destFile), "INFO");
                if (QFile::copy(luaFile, destFile)) {
                    emit log(QString("Successfully installed to: %1").arg(destFile), "SUCCESS");
                    atLeastOneSuccess = true;
                } else {
                    emit log(QString("Failed to copy to: %1").arg(destFile), "WARN");
                }
            }
            
            // Cleanup
            emit log("Cleaning up temporary files...", "INFO");
            QFile::remove(archivePath);
            QDir(extractDir).removeRecursively();
            
            if (!atLeastOneSuccess) {
                throw std::runtime_error("Failed to install Lua file to any plugin folder");
            }
            
            emit log("Generation and installation complete!", "SUCCESS");
            emit finished(destFile);
            
        } else {
            // Not a ZIP - log the response for debugging
            emit log("Response is not a ZIP archive", "ERROR");
            QString preview = QString::fromUtf8(data.left(500));
            emit log(QString("Response preview: %1").arg(preview), "WARN");
            
            // Check for common error indicators
            if (data.contains("error") || data.contains("Error")) {
                emit log("Server returned an error response", "ERROR");
            }
            if (data.contains("<!DOCTYPE") || data.contains("<html")) {
                emit log("Server returned HTML instead of ZIP (possibly a redirect or error page)", "ERROR");
            }
            
            throw std::runtime_error("Unexpected response format (not a ZIP file)");
        }
        
    } catch (const std::exception& e) {
        emit log(QString("Generation failed: %1").arg(e.what()), "ERROR");
        emit error(QString::fromStdString(e.what()));
    }
}
