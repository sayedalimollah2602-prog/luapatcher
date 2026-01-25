#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

namespace Config {
    const QString APP_VERSION = "1.0";
    const QString WEBSERVER_BASE_URL = "https://webserver-ecru.vercel.app";
    
    // Use inline functions to avoid static initialization issues
    inline QString gamesIndexUrl() {
        return WEBSERVER_BASE_URL + "/api/games_index.json";
    }
    
    inline QString luaFileUrl() {
        return WEBSERVER_BASE_URL + "/lua/";
    }
    
    // Steam paths
    const QString STEAM_PLUGIN_DIR = "C:/Program Files (x86)/Steam/config/stplug-in";
    const QString STEAM_EXE_PATH = "C:/Program Files (x86)/Steam/Steam.exe";
}

#endif // CONFIG_H
