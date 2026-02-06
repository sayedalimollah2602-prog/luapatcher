#ifndef GAMEINFO_H
#define GAMEINFO_H

#include <QString>

struct GameInfo {
    QString id;
    QString name;
    bool hasFix = false;
    
    bool operator==(const GameInfo& other) const {
        return id == other.id;
    }
    
    // For QSet or hashing if needed later
    friend uint qHash(const GameInfo& key, uint seed = 0) {
        return qHash(key.id, seed);
    }
};

#endif // GAMEINFO_H
