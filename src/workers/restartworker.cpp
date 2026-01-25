#include "restartworker.h"
#include "../config.h"
#include <QProcess>
#include <QThread>
#include <QFile>

RestartWorker::RestartWorker(QObject* parent)
    : QThread(parent)
{
}

void RestartWorker::run() {
    try {
        // Kill Steam
        QProcess killProcess;
        killProcess.start("taskkill", QStringList() << "/F" << "/IM" << "steam.exe");
        killProcess.waitForFinished();
        
        // Wait 2 seconds
        QThread::sleep(2);
        
        // Restart Steam
        QString steamExe = Config::getSteamExePath();
        if (QFile::exists(steamExe)) {
            QProcess::startDetached(steamExe, QStringList());
            emit finished("Steam launched!");
        } else {
            QProcess::startDetached("cmd", QStringList() << "/c" << "start" << "steam://open/main");
            emit finished("Restart command sent.");
        }
        
    } catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    } catch (...) {
        emit error("Unknown error occurred");
    }
}
