#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QString>
#include <QMap>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class GlassButton;
#include "utils/gameinfo.h"
#include "workers/luagenerationworker.h"

class LoadingSpinner;
class IndexDownloadWorker;

class LuaDownloadWorker;
class LuaGenerationWorker;
class RestartWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSyncDone(QList<GameInfo> games);

    void onSyncError(QString error);
    void onSearchChanged(const QString& text);
    void doSearch();
    void onSearchFinished(QNetworkReply* reply);
    void onGameSelected(QListWidgetItem* item);
    void doPatch();
    void onPatchDone(QString path);
    void onPatchError(QString error);
    
    void doGenerate();
    void onGenerationFinished(QString path);
    void onGenerationError(QString error);
    void onGenerationStatus(QString status);

    void doRestart();

private:
    void initUI();
    void startSync();
    void displayResults(const QJsonArray& items);
    QIcon createStatusIcon(bool supported);

    // UI Components
    QLabel* m_statusLabel;
    QLineEdit* m_searchInput;
    QStackedWidget* m_stack;
    LoadingSpinner* m_spinner;
    QListWidget* m_resultsList;
    QProgressBar* m_progress;
    GlassButton* m_btnPatch;
    GlassButton* m_btnGenerate;
    GlassButton* m_btnRestart;

    // Data
    QList<GameInfo> m_supportedGames;

    QMap<QString, QString> m_selectedGame; // name, appid, supported
    
    // Network
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_activeReply;
    
    // Search debounce
    QTimer* m_debounceTimer;
    int m_currentSearchId;
    
    // Workers
    IndexDownloadWorker* m_syncWorker;
    LuaDownloadWorker* m_dlWorker;
    LuaGenerationWorker* m_genWorker;
    RestartWorker* m_restartWorker;
};

#endif // MAINWINDOW_H
