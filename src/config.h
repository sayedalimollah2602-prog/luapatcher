#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

namespace Config {
    const QString APP_VERSION = "1.3.0";
    const QString WEBSERVER_BASE_URL = "https://webserver-ecru.vercel.app";
    
    // Server access token - check local file first, then macro
    inline QString getAccessToken() {
        QFile file("server_token.txt");
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString token = QString::fromUtf8(file.readAll()).trimmed();
            if (!token.isEmpty()) return token;
        }
        #ifndef SERVER_ACCESS_TOKEN
        return "dev-token-replace-in-prod";
        #else
        return QString(SERVER_ACCESS_TOKEN);
        #endif
    }
    
    // Use inline functions to avoid static initialization issues
    inline QString gamesIndexUrl() {
        return WEBSERVER_BASE_URL + "/api/games_index.json";
    }
    
    inline QString luaFileUrl() {
        return WEBSERVER_BASE_URL + "/lua/";
    }
    
    inline QString gameFixUrl() {
        return WEBSERVER_BASE_URL + "/fix/";
    }
    
    // Steam paths - check all drives for Steam installation
    inline QStringList getAllSteamPluginDirs() {
        QStringList paths;
        
        // Common Steam installation paths to check on each drive
        QStringList steamPaths = {
            ":/Program Files (x86)/Steam/config/stplug-in",
            ":/Program Files/Steam/config/stplug-in",
            ":/Steam/config/stplug-in"
        };
        
        // Check all drives from A to Z
        for (char drive = 'A'; drive <= 'Z'; drive++) {
            for (const QString& steamPath : steamPaths) {
                QString fullPath = QString(drive) + steamPath;
                QDir dir(fullPath);
                if (dir.exists()) {
                    paths.append(fullPath);
                }
            }
        }
        
        return paths;
    }
    
    inline QString getSteamPluginDir() {
        QStringList paths = getAllSteamPluginDirs();
        return paths.isEmpty() ? "C:/Program Files (x86)/Steam/config/stplug-in" : paths.first();
    }
    
    inline QStringList getAllSteamExePaths() {
        QStringList paths;
        
        // Common Steam executable paths to check on each drive
        QStringList steamExePaths = {
            ":/Program Files (x86)/Steam/Steam.exe",
            ":/Program Files/Steam/Steam.exe",
            ":/Steam/Steam.exe"
        };
        
        // Check all drives from A to Z
        for (char drive = 'A'; drive <= 'Z'; drive++) {
            for (const QString& exePath : steamExePaths) {
                QString fullPath = QString(drive) + exePath;
                QFileInfo fileInfo(fullPath);
                if (fileInfo.exists() && fileInfo.isFile()) {
                    paths.append(fullPath);
                }
            }
        }
        
        return paths;
    }
    
    inline QString getSteamExePath() {
        QStringList paths = getAllSteamExePaths();
        return paths.isEmpty() ? "C:/Program Files (x86)/Steam/Steam.exe" : paths.first();
    }
}

#endif // CONFIG_H
