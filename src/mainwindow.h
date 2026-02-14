#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QString>
#include <QMap>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScrollArea>
#include <QGridLayout>
#include <QSet>

class GlassButton;
class GameCard;
#include "utils/gameinfo.h"
#include "terminaldialog.h"

class LoadingSpinner;
class IndexDownloadWorker;

class LuaDownloadWorker;
class RestartWorker;
class GeneratorWorker;
class FixDownloadWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class AppMode {
        LuaPatcher,
        FixManager,
        Library
    };

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onSyncDone(QList<GameInfo> games);
    void onSyncError(QString error);
    void onSearchChanged(const QString& text);
    void doSearch();
    void onSearchFinished(QNetworkReply* reply);
    void onGameNameFetched(QNetworkReply* reply);
    void onThumbnailDownloaded(QNetworkReply* reply);
    void onCardClicked(GameCard* card);
    void doAddGame();
    void runPatchLogic();
    void runGenerateLogic();
    void onPatchDone(QString path);
    void onPatchError(QString error);
    void doRestart();
    void doApplyFix();
    void doRemoveGame();
    void switchMode(AppMode mode);
    void updateModeUI();
    void processNextNameFetch();
    void populateFixList();
    void loadVisibleThumbnails();

private:
    void initUI();
    void startSync();
    void displayResults(const QJsonArray& items);
    void startBatchNameFetch();
    void cancelNameFetches();
    void clearGameCards();
    void displayRandomGames();
    void displayLibrary();

    // UI Components
    QLabel* m_statusLabel;
    QLineEdit* m_searchInput;
    QStackedWidget* m_stack;
    LoadingSpinner* m_spinner;
    QProgressBar* m_progress;
    
    // Grid components
    QScrollArea* m_scrollArea;
    QWidget* m_gridContainer;
    QGridLayout* m_gridLayout;
    QList<GameCard*> m_gameCards;
    GameCard* m_selectedCard = nullptr;
    
    // Header & Tabs
    GlassButton* m_tabLua;
    GlassButton* m_tabFix;
    GlassButton* m_tabLibrary;
    AppMode m_currentMode;

    GlassButton* m_btnAddToLibrary;
    GlassButton* m_btnApplyFix;
    GlassButton* m_btnRemove;
    GlassButton* m_btnRestart;
    TerminalDialog* m_terminalDialog;

    // Data
    QList<GameInfo> m_supportedGames;
    QMap<QString, QString> m_selectedGame;
    
    // Network
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_activeReply;
    
    // Search debounce
    QTimer* m_debounceTimer;
    int m_currentSearchId;
    
    // Workers
    IndexDownloadWorker* m_syncWorker;
    LuaDownloadWorker* m_dlWorker;
    GeneratorWorker* m_genWorker;
    FixDownloadWorker* m_fixWorker;
    RestartWorker* m_restartWorker;
    
    // Batch name fetching
    QStringList m_pendingNameFetchIds;
    QList<QNetworkReply*> m_activeNameFetches;
    bool m_fetchingNames;
    int m_nameFetchSearchId;
    
    // Thumbnail cache
    QMap<QString, QPixmap> m_thumbnailCache;
    QSet<QString> m_activeThumbnailDownloads;
};

#endif // MAINWINDOW_H
