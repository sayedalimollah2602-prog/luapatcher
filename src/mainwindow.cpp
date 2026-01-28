#include "mainwindow.h"
#include "glassbutton.h"
#include "loadingspinner.h"
#include "workers/indexdownloadworker.h"
#include "workers/luadownloadworker.h"
#include "workers/restartworker.h"
#include "utils/colors.h"
#include "utils/paths.h"
#include "config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QLinearGradient>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QFile>
#include <QDir>
#include <QPixmap>
#include <QPainterPath>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_activeReply(nullptr)
    , m_currentSearchId(0)
    , m_syncWorker(nullptr)
    , m_dlWorker(nullptr)
    , m_restartWorker(nullptr)
{
    setWindowTitle("Steam Lua Patcher");
    setFixedSize(900, 600);
    
    // Try to load icon
    QString iconPath = Paths::getResourcePath("logo.ico");
    if (QFile::exists(iconPath)) {
        setWindowIcon(QIcon(iconPath));
    }
    
    // Setup debounce timer
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::doSearch);
    
    // Setup network manager
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onSearchFinished);
    
    initUI();
    startSync();
}

MainWindow::~MainWindow() {
    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
    }
}

void MainWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, Colors::toQColor(Colors::BG_GRADIENT_START));
    grad.setColorAt(1, Colors::toQColor(Colors::BG_GRADIENT_END));
    painter.fillRect(rect(), grad);
}

void MainWindow::initUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(40);
    
    // ─── LEFT COLUMN: ACTIONS ───
    QVBoxLayout* leftCol = new QVBoxLayout();
    leftCol->setSpacing(16);
    
    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* icon = new QLabel("⚡");
    icon->setStyleSheet(QString("font-size: 32px; color: %1; font-weight: bold;")
                       .arg(Colors::ACCENT_BLUE));
    QLabel* title = new QLabel("Lua Patcher");
    title->setStyleSheet(QString("font-size: 24px; font-weight: 800; color: %1;")
                        .arg(Colors::TEXT_PRIMARY));
    headerLayout->addWidget(icon);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    leftCol->addLayout(headerLayout);
    
    leftCol->addSpacing(20);
    
    // Status
    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
                                .arg(Colors::TEXT_SECONDARY));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    leftCol->addWidget(m_statusLabel);
    
    leftCol->addStretch();
    
    // Buttons
    m_btnPatch = new GlassButton("⬇", "Patch Game", 
                                 "Install Lua patch for selected game",
                                 Colors::ACCENT_GREEN);
    m_btnPatch->setEnabled(false);
    connect(m_btnPatch, &QPushButton::clicked, this, &MainWindow::doPatch);
    leftCol->addWidget(m_btnPatch);
    
    m_btnRestart = new GlassButton("↻", "Restart Steam",
                                   "Apply changes by restarting Steam",
                                   Colors::ACCENT_PURPLE);
    connect(m_btnRestart, &QPushButton::clicked, this, &MainWindow::doRestart);
    leftCol->addWidget(m_btnRestart);
    
    leftCol->addStretch();
    
    // Version
    QLabel* versionLabel = new QLabel(QString("v%1").arg(Config::APP_VERSION));
    versionLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
                               .arg(Colors::TEXT_SECONDARY));
    versionLabel->setAlignment(Qt::AlignCenter);
    leftCol->addWidget(versionLabel);
    
    // Creator
    QLabel* creatorLabel = new QLabel(
        "created by <a href=\"https://github.com/sayedalimollah2602-prog\" "
        "style=\"color: #94A3B8; text-decoration: none;\">leVI</a> & "
        "<a href=\"https://github.com/raxnmint\" "
        "style=\"color: #94A3B8; text-decoration: none;\">raxnmint</a>");
    creatorLabel->setStyleSheet("font-size: 11px;");
    creatorLabel->setAlignment(Qt::AlignCenter);
    creatorLabel->setOpenExternalLinks(true);
    leftCol->addWidget(creatorLabel);
    
    mainLayout->addLayout(leftCol, 35);
    
    // ─── RIGHT COLUMN: SEARCH & LIST ───
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(16);
    
    // Search Bar
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Find a game...");
    connect(m_searchInput, &QLineEdit::textChanged, 
            this, &MainWindow::onSearchChanged);
    
    // Add shadow
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(m_searchInput);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 4);
    m_searchInput->setGraphicsEffect(shadow);
    
    rightCol->addWidget(m_searchInput);
    
    // Stack for Loading / List
    m_stack = new QStackedWidget();
    
    // Loading Page
    QWidget* pageLoading = new QWidget();
    QVBoxLayout* layLoading = new QVBoxLayout(pageLoading);
    layLoading->setAlignment(Qt::AlignCenter);
    m_spinner = new LoadingSpinner();
    layLoading->addWidget(m_spinner);
    m_stack->addWidget(pageLoading);
    
    // Results Page
    m_resultsList = new QListWidget();
    m_resultsList->setIconSize(QSize(40, 40));
    m_resultsList->setWordWrap(true);
    m_resultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultsList->setTextElideMode(Qt::ElideNone);
    connect(m_resultsList, &QListWidget::itemPressed,
            this, &MainWindow::onGameSelected);
    m_stack->addWidget(m_resultsList);
    
    rightCol->addWidget(m_stack);
    
    // Progress Bar
    m_progress = new QProgressBar();
    m_progress->setFixedHeight(4);
    m_progress->setTextVisible(false);
    m_progress->setStyleSheet(QString(
        "QProgressBar {"
        "    background: %1;"
        "    border-radius: 2px;"
        "}"
        "QProgressBar::chunk {"
        "    background: %2;"
        "    border-radius: 2px;"
        "}")
        .arg(Colors::GLASS_BG)
        .arg(Colors::ACCENT_GREEN));
    m_progress->hide();
    rightCol->addWidget(m_progress);
    
    mainLayout->addLayout(rightCol, 65);
}

void MainWindow::startSync() {
    m_stack->setCurrentIndex(0); // Loading
    m_spinner->start();
    
    m_syncWorker = new IndexDownloadWorker(this);
    connect(m_syncWorker, &IndexDownloadWorker::finished,
            this, &MainWindow::onSyncDone);
    connect(m_syncWorker, &IndexDownloadWorker::progress,
            m_statusLabel, &QLabel::setText);
    connect(m_syncWorker, &IndexDownloadWorker::error,
            this, &MainWindow::onSyncError);
    m_syncWorker->start();
}

void MainWindow::onSyncDone(QList<GameInfo> games) {
    m_supportedGames = games;
    m_spinner->stop();
    m_stack->setCurrentIndex(1); // List
    m_statusLabel->setText(QString("Online • %1 supported games")
                          .arg(games.size()));
    m_searchInput->setFocus();
    
    // Trigger initial empty search to show list if needed, or just clear
    if (!m_searchInput->text().isEmpty()) {
        doSearch();
    }
}


void MainWindow::onSyncError(QString error) {
    m_spinner->stop();
    m_stack->setCurrentIndex(1);
    m_statusLabel->setText("Offline Mode");
    QMessageBox::warning(this, "Connection Error",
                        QString("Could not sync library:\n%1").arg(error));
}

void MainWindow::onSearchChanged(const QString& text) {
    m_debounceTimer->stop();
    if (!text.trimmed().isEmpty()) {
        m_debounceTimer->start(400);
    } else {
        m_resultsList->clear();
    }
}

void MainWindow::doSearch() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) return;
    
    m_currentSearchId++;
    m_statusLabel->setText("Searching...");
    
    // 1. Local Search (Instant)
    QJsonArray localResults;
    for (const auto& game : m_supportedGames) {
        if (game.name.contains(query, Qt::CaseInsensitive) || game.id == query) {
            QJsonObject item;
            item["id"] = game.id;
            item["name"] = game.name;
            item["supported_local"] = true; // Marker for local result
            localResults.append(item);
        }
    }
    
    // Display local results immediately
    displayResults(localResults);
    
    // 2. Remote Search (Fallback/Supplement)
    // Cancel previous active searches (we might have multiple now)
    // For simplicity in this structure, we'll just let them finish or strictly rely on ID check
    // But ideally we should track them. For now, let's keep it simple: 
    // Increment search ID invalidates old results logic in onSearchFinished.
    
    bool isNumeric;
    query.toInt(&isNumeric);
    
    if (isNumeric) {
        // --- Numeric App ID Search ---
        // 1. Steam Store AppDetails (Primary)
        QUrl urlStore(QString("https://store.steampowered.com/api/appdetails?appids=%1").arg(query));
        QNetworkRequest reqStore(urlStore);
        QNetworkReply* repStore = m_networkManager->get(reqStore);
        repStore->setProperty("sid", m_currentSearchId);
        repStore->setProperty("type", "steam_details");
        repStore->setProperty("query_id", query);
    } else {
        // --- Text Name Search ---
        if (m_activeReply) {
            m_activeReply->abort();
        }
        
        QUrl url("https://store.steampowered.com/api/storesearch");
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("term", query);
        urlQuery.addQueryItem("l", "english");
        urlQuery.addQueryItem("cc", "US");
        url.setQuery(urlQuery);
        
        QNetworkRequest request(url);
        m_activeReply = m_networkManager->get(request);
        m_activeReply->setProperty("sid", m_currentSearchId);
        m_activeReply->setProperty("type", "store_search");
    }
}


void MainWindow::onSearchFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply == m_activeReply) m_activeReply = nullptr;
    
    if (reply->error() == QNetworkReply::OperationCanceledError) return;
    int sid = reply->property("sid").toInt();
    if (sid != m_currentSearchId) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        // Only show error if we have absolutely nothing and it's a primary search
        if (m_resultsList->count() == 0 && reply->property("type").toString() == "store_search") {
            m_statusLabel->setText("Search failed");
        }
        return;
    }
    
    QString type = reply->property("type").toString();
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    
    QList<QJsonObject> newItems;
    
    if (type == "store_search") {
        // Standard Store Search
        QJsonArray remoteItems = obj["items"].toArray();
        for (const QJsonValue& val : remoteItems) {
            newItems.append(val.toObject());
        }
    } 
    else if (type == "steam_details") {
        // API returns { "appid": { "success": true, "data": { ... } } }
        QString qId = reply->property("query_id").toString();
        bool steamSuccess = false;
        if (obj.contains(qId)) {
            QJsonObject root = obj[qId].toObject();
            if (root["success"].toBool() && root.contains("data")) {
                QJsonObject dataObj = root["data"].toObject();
                QJsonObject item;
                item["id"] = dataObj["steam_appid"].toInt();
                item["name"] = dataObj["name"].toString();
                newItems.append(item);
                steamSuccess = true;
            }
        }
        
        // Fallback to SteamSpy if Steam API failed or returned no data
        if (!steamSuccess) {
            QUrl urlSpy(QString("https://steamspy.com/api.php?request=appdetails&appid=%1").arg(qId));
            QNetworkRequest reqSpy(urlSpy);
            QNetworkReply* repSpy = m_networkManager->get(reqSpy);
            repSpy->setProperty("sid", sid);
            repSpy->setProperty("type", "steamspy_details");
        }
    }
    else if (type == "steamspy_details") {
        // API returns { "appid": 123, "name": "Game Name", ... }
        if (obj.contains("name") && !obj["name"].toString().isEmpty()) {
            QJsonObject item;
            item["id"] = obj["appid"].isDouble() ? obj["appid"].toInt() : obj["appid"].toString().toInt();
            item["name"] = obj["name"].toString();
            newItems.append(item);
        }
    }
    
    // Merge logic
    QSet<QString> existingIds;
    for (int i = 0; i < m_resultsList->count(); ++i) {
        QMap<QString, QString> data = m_resultsList->item(i)->data(Qt::UserRole).value<QMap<QString, QString>>();
        existingIds.insert(data["appid"]);
    }
    
    bool addedRemote = false;
    
    for (const auto& item : newItems) {
        QString id = QString::number(item["id"].toInt());
        
        if (!existingIds.contains(id)) {
            QString name = item["name"].toString("Unknown");
            
            // Re-check support status (in case local search missed it somehow, or logic requires strict id match)
            bool supported = false;
            for(const auto& g : m_supportedGames) {
                if(g.id == id) {
                    supported = true; 
                    break;
                }
            }
            
            QString statusText = supported ? "Supported" : "Not Indexed";
            QString displayText = QString("%1\n%2 • ID: %3")
                                .arg(name).arg(statusText).arg(id);
            
            QListWidgetItem* listItem = new QListWidgetItem(displayText);
            
            QMap<QString, QString> data;
            data["name"] = name;
            data["appid"] = id;
            data["supported"] = supported ? "true" : "false";
            listItem->setData(Qt::UserRole, QVariant::fromValue(data));
            
            listItem->setIcon(createStatusIcon(supported));
            
            if (supported) {
                listItem->setForeground(Colors::toQColor(Colors::ACCENT_GREEN));
            } else {
                listItem->setForeground(Colors::toQColor(Colors::TEXT_SECONDARY));
            }
            
            m_resultsList->addItem(listItem);
            existingIds.insert(id); // Prevent dupes if both APIs return same ID quickly
            addedRemote = true;
        }
    }
    
    if (addedRemote || m_resultsList->count() > 0) {
        m_statusLabel->setText(QString("Found %1 results").arg(m_resultsList->count()));
    } else {
         m_statusLabel->setText("No results found");
    }
}


QIcon MainWindow::createStatusIcon(bool supported) {
    int size = 64;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Color & Shape
    QColor color = supported ? Colors::toQColor(Colors::ACCENT_GREEN) 
                             : Colors::toQColor(Colors::ACCENT_RED);
    QColor bgColor = color;
    bgColor.setAlpha(40);
    
    // Draw Circle Background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(bgColor));
    painter.drawEllipse(4, 4, size-8, size-8);
    
    // Draw Border
    painter.setPen(QPen(color, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(4, 4, size-8, size-8);
    
    // Draw Symbol
    QFont font("Segoe UI Symbol", 28, QFont::Bold);
    painter.setFont(font);
    painter.setPen(color);
    QString symbol = supported ? "✓" : "✕";
    painter.drawText(pixmap.rect(), Qt::AlignCenter, symbol);
    
    return QIcon(pixmap);
}

void MainWindow::displayResults(const QJsonArray& items) {
    m_resultsList->clear();
    m_selectedGame.clear();
    m_btnPatch->setEnabled(false);
    
    if (items.isEmpty()) {
        // m_statusLabel->setText("No results found"); // Don't show this yet, might be waiting for remote
        return; 
    }
    
    for (const QJsonValue& val : items) {
        QJsonObject item = val.toObject();
        QString name = item["name"].toString("Unknown");
        QString appid = item.contains("id") ? (item["id"].isString() ? item["id"].toString() : QString::number(item["id"].toInt())) : "0";
        bool supported = false;
        
        // If it came from local search, we know it's supported
        if (item.contains("supported_local")) {
            supported = true;
        } else {
            // Double check
             for(const auto& g : m_supportedGames) {
               if(g.id == appid) {
                   supported = true; 
                   break;
               }
           }
        }

        
        QString statusText = supported ? "Supported" : "Not Indexed";
        QString displayText = QString("%1\n%2 • ID: %3")
                             .arg(name).arg(statusText).arg(appid);
        
        QListWidgetItem* listItem = new QListWidgetItem(displayText);
        
        // Store data
        QMap<QString, QString> data;
        data["name"] = name;
        data["appid"] = appid;
        data["supported"] = supported ? "true" : "false";
        listItem->setData(Qt::UserRole, QVariant::fromValue(data));
        
        listItem->setIcon(createStatusIcon(supported));
        
        // Custom styling
        if (supported) {
            listItem->setForeground(Colors::toQColor(Colors::ACCENT_GREEN));
        } else {
            listItem->setForeground(Colors::toQColor(Colors::TEXT_SECONDARY));
        }
        
        m_resultsList->addItem(listItem);
    }
    
    m_statusLabel->setText(QString("Found %1 results").arg(items.size()));
}

void MainWindow::onGameSelected(QListWidgetItem* item) {
    QMap<QString, QString> data = item->data(Qt::UserRole)
                                      .value<QMap<QString, QString>>();
    
    if (data["supported"] == "true") {
        m_selectedGame = data;
        m_btnPatch->setEnabled(true);
        m_btnPatch->setDescription(QString("Install patch for %1")
                                  .arg(data["name"]));
        m_statusLabel->setText(QString("Selected: %1").arg(data["name"]));
    } else {
        m_selectedGame.clear();
        m_btnPatch->setEnabled(false);
        m_btnPatch->setDescription("Patch unavailable for this game");
        m_statusLabel->setText("Game not supported");
    }
}

void MainWindow::doPatch() {
    if (m_selectedGame.isEmpty()) return;
    
    m_btnPatch->setEnabled(false);
    m_progress->setValue(0);
    m_progress->show();
    
    m_dlWorker = new LuaDownloadWorker(m_selectedGame["appid"], this);
    connect(m_dlWorker, &LuaDownloadWorker::finished,
            this, &MainWindow::onPatchDone);
    connect(m_dlWorker, &LuaDownloadWorker::progress,
            [this](qint64 downloaded, qint64 total) {
                if (total > 0) {
                    m_progress->setValue(static_cast<int>(downloaded * 100 / total));
                }
            });
    connect(m_dlWorker, &LuaDownloadWorker::status,
            m_statusLabel, &QLabel::setText);
    connect(m_dlWorker, &LuaDownloadWorker::error,
            this, &MainWindow::onPatchError);
    m_dlWorker->start();
}

void MainWindow::onPatchDone(QString path) {
    try {
        QStringList targetDirs = Config::getAllSteamPluginDirs();
        if (targetDirs.isEmpty()) {
            targetDirs.append(Config::getSteamPluginDir());
        }

        bool atLeastOneSuccess = false;
        QString lastError;

        for (const QString& pluginDir : targetDirs) {
            QString dest = QDir(pluginDir).filePath(m_selectedGame["appid"] + ".lua");
            
            QDir dir;
            if (!dir.exists(pluginDir)) {
                dir.mkpath(pluginDir);
            }
            
            if (QFile::exists(dest)) {
                QFile::remove(dest);
            }
            
            if (QFile::copy(path, dest)) {
                atLeastOneSuccess = true;
            } else {
                lastError = "Failed to copy patch file to " + pluginDir;
            }
        }

        if (!atLeastOneSuccess) {
             throw std::runtime_error(lastError.toStdString());
        }
        
        // Clean up the downloaded file after copying to all destinations
        QFile::remove(path);
        
        m_progress->hide();
        m_btnPatch->setEnabled(true);
        m_statusLabel->setText("Patch Installed!");
        QMessageBox::information(this, "Success",
                                QString("Patch installed for %1")
                                .arg(m_selectedGame["name"]));
    } catch (const std::exception& e) {
        onPatchError(QString::fromStdString(e.what()));
    }
}

void MainWindow::onPatchError(QString error) {
    m_progress->hide();
    m_btnPatch->setEnabled(true);
    m_statusLabel->setText("Error");
    QMessageBox::critical(this, "Error", error);
}

void MainWindow::doRestart() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Restart Steam?", "Close Steam and all games?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) return;
    
    m_restartWorker = new RestartWorker(this);
    connect(m_restartWorker, &RestartWorker::finished,
            m_statusLabel, &QLabel::setText);
    m_restartWorker->start();
}
