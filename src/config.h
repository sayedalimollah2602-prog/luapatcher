#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

namespace Config {
    const QString APP_VERSION = "1.2.1";
    const QString WEBSERVER_BASE_URL = "https://webserver-ecru.vercel.app";
    
    // Server access token - passed via macro during compilation
    #ifndef SERVER_ACCESS_TOKEN
    #define SERVER_ACCESS_TOKEN "dev-token-replace-in-prod"
    #endif
    const QString ACCESS_TOKEN = QString(SERVER_ACCESS_TOKEN);
    
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
