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
class LoadingSpinner;
class IndexDownloadWorker;
class LuaDownloadWorker;
class RestartWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSyncDone(QSet<QString> appIds);
    void onSyncError(QString error);
    void onSearchChanged(const QString& text);
    void doSearch();
    void onSearchFinished(QNetworkReply* reply);
    void onGameSelected(QListWidgetItem* item);
    void doPatch();
    void onPatchDone(QString path);
    void onPatchError(QString error);
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
    GlassButton* m_btnRestart;

    // Data
    QSet<QString> m_cachedAppIds;
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
    RestartWorker* m_restartWorker;
};

#endif // MAINWINDOW_H
