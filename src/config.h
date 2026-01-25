#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

namespace Config {
    const QString APP_VERSION = "1.0";
    const QString WEBSERVER_BASE_URL = "https://webserver-ecru.vercel.app";
    const QString GAMES_INDEX_URL = WEBSERVER_BASE_URL + "/api/games_index.json";
    const QString LUA_FILE_URL = WEBSERVER_BASE_URL + "/lua/";
    
    // Steam paths
    const QString STEAM_PLUGIN_DIR = "C:/Program Files (x86)/Steam/config/stplug-in";
    const QString STEAM_EXE_PATH = "C:/Program Files (x86)/Steam/Steam.exe";
}

#endif // CONFIG_H
