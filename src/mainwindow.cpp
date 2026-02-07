#include "mainwindow.h"
#include "glassbutton.h"
#include "loadingspinner.h"
#include "workers/indexdownloadworker.h"
#include "workers/luadownloadworker.h"
#include "workers/generatorworker.h"
#include "workers/fixdownloadworker.h"
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
#include <QFileDialog>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_currentMode(AppMode::LuaPatcher)
    , m_networkManager(nullptr)
    , m_activeReply(nullptr)
    , m_currentSearchId(0)
    , m_syncWorker(nullptr)
    , m_dlWorker(nullptr)
    , m_genWorker(nullptr)
    , m_restartWorker(nullptr)
    , m_fetchingNames(false)
    , m_nameFetchSearchId(0)
{
    setWindowTitle("Steam Lua Patcher");
    setFixedSize(900, 600);
    
    // Try to load icon
    QString iconPath = Paths::getResourcePath("logo.ico");
    if (QFile::exists(iconPath)) {
        setWindowIcon(QIcon(iconPath));
    }
    
    // Initialize UI components first
    initUI();
    
    // Setup debounce timer
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::doSearch);
    
    // Defer network initialization and sync to allow window to show first
    QTimer::singleShot(10, this, [this]() {
        m_networkManager = new QNetworkAccessManager(this);
        connect(m_networkManager, &QNetworkAccessManager::finished,
                this, &MainWindow::onSearchFinished);
        startSync();
    });
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
    
    // Root Layout (Vertical)
    QVBoxLayout* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    
    // ---------------------------------------------------------
    // 1. Global Header
    // ---------------------------------------------------------
    QWidget* headerWidget = new QWidget();
    headerWidget->setStyleSheet("background-color: rgba(20, 20, 30, 0.95); border-bottom: 1px solid rgba(255, 255, 255, 0.1);");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 15, 20, 15);
    headerLayout->setSpacing(20);
    
    // Icon & Title
    QLabel* icon = new QLabel("⚡");
    icon->setStyleSheet(QString("font-size: 28px; color: %1; font-weight: bold; background: transparent; border: none;").arg(Colors::ACCENT_BLUE));
    
    QLabel* title = new QLabel("Lua Patcher");
    title->setStyleSheet("font-size: 20px; font-weight: 800; color: #E2E8F0; background: transparent; border: none;");
    
    headerLayout->addWidget(icon);
    headerLayout->addWidget(title);
    headerLayout->addSpacing(40);
    
    // Tabs (Navigation)
    m_tabLua = new GlassButton("⬇", "Lua Patcher", "", Colors::ACCENT_BLUE);
    m_tabLua->setFixedSize(160, 45);
    connect(m_tabLua, &QPushButton::clicked, this, [this](){ switchMode(AppMode::LuaPatcher); });
    
    m_tabFix = new GlassButton("🔧", "Fix Manager", "", Colors::ACCENT_PURPLE); 
    m_tabFix->setFixedSize(160, 45);
    connect(m_tabFix, &QPushButton::clicked, this, [this](){ switchMode(AppMode::FixManager); });
    
    headerLayout->addWidget(m_tabLua);
    headerLayout->addWidget(m_tabFix);
    headerLayout->addStretch();
    
    rootLayout->addWidget(headerWidget);

    // ---------------------------------------------------------
    // 2. Content Area
    // ---------------------------------------------------------
    QWidget* contentWidget = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(40);
    
    // ─── LEFT COLUMN: ACTIONS ───
    QVBoxLayout* leftCol = new QVBoxLayout();
    leftCol->setSpacing(16);
    
    // Status
    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(Colors::TEXT_SECONDARY));
    m_statusLabel->setWordWrap(true);
    leftCol->addWidget(m_statusLabel);
    
    leftCol->addStretch();
    
    // Buttons
    m_btnPatch = new GlassButton("⬇", "Patch Game", "Install Lua patch for selected game", Colors::ACCENT_GREEN);
    m_btnPatch->setEnabled(false);
    connect(m_btnPatch, &QPushButton::clicked, this, &MainWindow::doPatch);
    leftCol->addWidget(m_btnPatch);
    
    m_btnGenerate = new GlassButton("⚙", "Generate Patch", "Fetch data for unknown game", Colors::ACCENT_BLUE);
    m_btnGenerate->setEnabled(false);
    m_btnGenerate->hide();
    connect(m_btnGenerate, &QPushButton::clicked, this, &MainWindow::doGenerate);
    leftCol->addWidget(m_btnGenerate);
    
    m_btnApplyFix = new GlassButton("🔧", "Apply Fix", "Download and apply game fix files", Colors::ACCENT_PURPLE);
    m_btnApplyFix->setEnabled(false);
    m_btnApplyFix->hide();
    connect(m_btnApplyFix, &QPushButton::clicked, this, &MainWindow::doApplyFix);
    leftCol->addWidget(m_btnApplyFix);
    
    m_btnRestart = new GlassButton("↻", "Restart Steam", "Apply changes by restarting Steam", Colors::ACCENT_PURPLE);
    connect(m_btnRestart, &QPushButton::clicked, this, &MainWindow::doRestart);
    leftCol->addWidget(m_btnRestart);
    
    leftCol->addStretch();
    
    // Version & Creator
    QLabel* versionLabel = new QLabel(QString("v%1").arg(Config::APP_VERSION));
    versionLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;").arg(Colors::TEXT_SECONDARY));
    versionLabel->setAlignment(Qt::AlignCenter);
    leftCol->addWidget(versionLabel);
    
    QLabel* creatorLabel = new QLabel("created by <a href=\"https://github.com/sayedalimollah2602-prog\" style=\"color: #94A3B8; text-decoration: none;\">leVI</a> & <a href=\"https://github.com/raxnmint\" style=\"color: #94A3B8; text-decoration: none;\">raxnmint</a>");
    creatorLabel->setStyleSheet("font-size: 11px;");
    creatorLabel->setAlignment(Qt::AlignCenter);
    creatorLabel->setOpenExternalLinks(true);
    leftCol->addWidget(creatorLabel);
    
    mainLayout->addLayout(leftCol, 35);
    
    // ─── RIGHT COLUMN: SEARCH & LIST ───
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(16);
    
    // Search Bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(8);
    
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Find a game...");
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(m_searchInput);
    shadow->setBlurRadius(20); shadow->setColor(QColor(0, 0, 0, 80)); shadow->setOffset(0, 4);
    m_searchInput->setGraphicsEffect(shadow);
    searchLayout->addWidget(m_searchInput);
    
    QPushButton* refreshBtn = new QPushButton("↻");
    refreshBtn->setFixedSize(36, 36);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QString("QPushButton { background: %1; border: 1px solid %2; border-radius: 8px; font-size: 16px; font-weight: bold; color: %3; } QPushButton:hover { background: %4; border-color: %5; } QPushButton:pressed { background: %6; }").arg(Colors::GLASS_BG).arg(Colors::GLASS_BORDER).arg(Colors::TEXT_PRIMARY).arg(Colors::GLASS_HOVER).arg(Colors::ACCENT_BLUE).arg(Colors::GLASS_BG));
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_searchInput->text().trimmed().isEmpty()) startSync(); else doSearch();
    });
    searchLayout->addWidget(refreshBtn);
    rightCol->addLayout(searchLayout);
    
    // Stack for Loading / List
    m_stack = new QStackedWidget();
    
    QWidget* pageLoading = new QWidget();
    QVBoxLayout* layLoading = new QVBoxLayout(pageLoading);
    layLoading->setAlignment(Qt::AlignCenter);
    m_spinner = new LoadingSpinner();
    layLoading->addWidget(m_spinner);
    m_stack->addWidget(pageLoading);
    
    m_resultsList = new QListWidget();
    m_resultsList->setIconSize(QSize(40, 40));
    m_resultsList->setWordWrap(true);
    m_resultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultsList->setTextElideMode(Qt::ElideNone);
    connect(m_resultsList, &QListWidget::itemPressed, this, &MainWindow::onGameSelected);
    m_stack->addWidget(m_resultsList);
    rightCol->addWidget(m_stack);
    
    m_progress = new QProgressBar();
    m_progress->setFixedHeight(4);
    m_progress->setTextVisible(false);
    m_progress->setStyleSheet(QString("QProgressBar { background: %1; border-radius: 2px; } QProgressBar::chunk { background: %2; border-radius: 2px; }").arg(Colors::GLASS_BG).arg(Colors::ACCENT_GREEN));
    m_progress->hide();
    rightCol->addWidget(m_progress);
    
    mainLayout->addLayout(rightCol, 65);
    rootLayout->addWidget(contentWidget);
    
    m_terminalDialog = new TerminalDialog(this);
    
    // Trigger initial UI state
    updateModeUI();
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
    m_statusLabel->setText("Ready");
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
    
    // Safety check if triggered before init
    if (!m_networkManager) return;
    
    // Cancel any in-progress name fetches
    cancelNameFetches();
    
    m_currentSearchId++;
    m_statusLabel->setText("Searching...");
    
    // 1. Local Search (Instant)
    QJsonArray localResults;
    for (const auto& game : m_supportedGames) {
        // FILTER: If in Fix Manager mode, only show games with hasFix = true
        if (m_currentMode == AppMode::FixManager && !game.hasFix) {
            continue;
        }

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
    
    // In Fix Manager mode, we ONLY search local supported games (because we can't apply fix to unsupported games)
    if (m_currentMode == AppMode::FixManager) {
        if (m_resultsList->count() == 0) {
            m_statusLabel->setText("No fixes found for this game");
        } else {
             m_statusLabel->setText(QString("Found %1 games with fixes").arg(m_resultsList->count()));
        }
        m_stack->setCurrentIndex(1);
        m_spinner->stop();
        return; 
    }
    
    // Show spinner if we need remote data
    m_spinner->start();
    if (m_resultsList->count() == 0) {
        m_stack->setCurrentIndex(0); // Show spinner page if no local results
    }
    
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
        
        if (!steamSuccess) {
            QUrl urlSpy(QString("https://steamspy.com/api.php?request=appdetails&appid=%1").arg(qId));
            QNetworkRequest reqSpy(urlSpy);
            QNetworkReply* repSpy = m_networkManager->get(reqSpy);
            repSpy->setProperty("sid", sid);
            repSpy->setProperty("type", "steamspy_details");
            return; // Don't finalize yet, let SteamSpy finish
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
    
    // Search is finished (either standard or the final fallback)
    m_spinner->stop();
    m_stack->setCurrentIndex(1);
    
    // Merge logic
    QMap<QString, QListWidgetItem*> itemMap;
    for (int i = 0; i < m_resultsList->count(); ++i) {
        QListWidgetItem* existingItem = m_resultsList->item(i);
        QMap<QString, QString> itemData = existingItem->data(Qt::UserRole).value<QMap<QString, QString>>();
        itemMap.insert(itemData["appid"], existingItem);
    }
    
    bool resultsChanged = false;
    
    for (const auto& item : newItems) {
        QString id = QString::number(item["id"].toInt());
        QString name = item["name"].toString("Unknown");
        
        // Re-check support status
        bool supported = false;
        bool hasFix = false;
        for(const auto& g : m_supportedGames) {
            if(g.id == id) {
                supported = true;
                hasFix = g.hasFix;
                break;
            }
        }

        if (itemMap.contains(id)) {
            // Update existing if it's "Unknown"
            QListWidgetItem* existingItem = itemMap[id];
            QMap<QString, QString> existingData = existingItem->data(Qt::UserRole).value<QMap<QString, QString>>();
            
            if (existingData["name"].contains("Unknown Game", Qt::CaseInsensitive) || existingData["name"] == id) {
                QString statusText = supported ? "Supported" : "Not Indexed • Supports Auto Generate";
                existingItem->setText(QString("%1\n%2 • ID: %3").arg(name).arg(statusText).arg(id));
                
                existingData["name"] = name;
                existingData["supported"] = supported ? "true" : "false";
                existingData["hasFix"] = hasFix ? "true" : "false";
                existingItem->setData(Qt::UserRole, QVariant::fromValue(existingData));
                
                existingItem->setIcon(createStatusIcon(supported));
                if (supported) {
                    existingItem->setForeground(Colors::toQColor(Colors::ACCENT_GREEN));
                } else {
                    existingItem->setForeground(Colors::toQColor(Colors::TEXT_SECONDARY));
                }
                resultsChanged = true;
            }
        } else {
            // Add new item
            QString statusText = supported ? "Supported" : "Not Indexed • Supports Auto Generate";
            QString displayText = QString("%1\n%2 • ID: %3").arg(name).arg(statusText).arg(id);
            
            QListWidgetItem* listItem = new QListWidgetItem(displayText);
            
            QMap<QString, QString> data;
            data["name"] = name;
            data["appid"] = id;
            data["supported"] = supported ? "true" : "false";
            data["hasFix"] = hasFix ? "true" : "false";
            listItem->setData(Qt::UserRole, QVariant::fromValue(data));
            
            listItem->setIcon(createStatusIcon(supported));
            if (supported) {
                listItem->setForeground(Colors::toQColor(Colors::ACCENT_GREEN));
            } else {
                listItem->setForeground(Colors::toQColor(Colors::TEXT_SECONDARY));
            }
            
            m_resultsList->addItem(listItem);
            itemMap.insert(id, listItem);
            resultsChanged = true;
        }
    }
    
    if (resultsChanged || m_resultsList->count() > 0) {
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
                             : Colors::toQColor(Colors::ACCENT_BLUE);
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
    QString symbol = supported ? "✓" : "⚙";
    painter.drawText(pixmap.rect(), Qt::AlignCenter, symbol);
    
    return QIcon(pixmap);
}

void MainWindow::displayResults(const QJsonArray& items) {
    m_resultsList->clear();
    m_selectedGame.clear();
    m_btnPatch->setEnabled(false);
    m_pendingNameFetchIds.clear();
    cancelNameFetches();
    
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
        bool hasFix = false;
        if (item.contains("supported_local")) {
            supported = true;
            // Check if this game has a fix
            for(const auto& g : m_supportedGames) {
               if(g.id == appid) {
                   hasFix = g.hasFix;
                   break;
               }
           }
        } else {
            // Double check
             for(const auto& g : m_supportedGames) {
               if(g.id == appid) {
                   supported = true;
                   hasFix = g.hasFix;
                   break;
               }
           }
        }

        
        QString statusText = supported ? "Supported" : "Not Indexed • Supports Auto Generate";
        QString displayText = QString("%1\n%2 • ID: %3")
                             .arg(name).arg(statusText).arg(appid);
        
        QListWidgetItem* listItem = new QListWidgetItem(displayText);
        
        // Store data
        QMap<QString, QString> data;
        data["name"] = name;
        data["appid"] = appid;
        data["supported"] = supported ? "true" : "false";
        data["hasFix"] = hasFix ? "true" : "false";
        listItem->setData(Qt::UserRole, QVariant::fromValue(data));
        
        listItem->setIcon(createStatusIcon(supported));
        
        // Custom styling
        if (supported) {
            listItem->setForeground(Colors::toQColor(Colors::ACCENT_GREEN));
        } else {
            listItem->setForeground(Colors::toQColor(Colors::TEXT_SECONDARY));
        }
        
        m_resultsList->addItem(listItem);
        
        // Track unknown games for batch fetch
        if (name.startsWith("Unknown Game") || name == "Unknown") {
            m_pendingNameFetchIds.append(appid);
        }
        
        // Also check if we just generated a patch for this game to update status effectively? 
        // For now, no extra logic needed.
    }
    
    m_statusLabel->setText(QString("Found %1 results").arg(items.size()));
    
    // Start fetching names for unknown games
    if (!m_pendingNameFetchIds.isEmpty()) {
        startBatchNameFetch();
    }
}

void MainWindow::onGameSelected(QListWidgetItem* item) {
    if (!item) {
        m_selectedGame.clear();
        m_btnPatch->setEnabled(false);
        m_btnGenerate->hide();
        m_btnApplyFix->hide();
        m_statusLabel->setText("Ready");
        return;
    }

    QMap<QString, QString> data = item->data(Qt::UserRole)
                                      .value<QMap<QString, QString>>();
    
    m_selectedGame = data;
    bool hasFix = (data["hasFix"] == "true");
    bool isSupported = (data["supported"] == "true");
    
    if (m_currentMode == AppMode::LuaPatcher) {
        // Patcher Mode: Show Patch/Generate, Hide Fix
        m_btnApplyFix->hide();
        m_btnPatch->show(); // Ensure visible in case it was hidden
        
        if (isSupported) {
            m_btnPatch->setEnabled(true);
            m_btnPatch->setDescription(QString("Install patch for %1").arg(data["name"]));
            m_statusLabel->setText(QString("Selected: %1").arg(data["name"]));
            
            m_btnGenerate->hide();
        } else {
            m_selectedGame["supported"] = "false"; 
            
            m_btnPatch->setEnabled(false);
            m_btnPatch->setDescription("Patch unavailable for this game");
            
            m_btnGenerate->setEnabled(true);
            m_btnGenerate->show();
            m_btnGenerate->setDescription(QString("Generate patch for %1").arg(data["name"]));
            
            m_statusLabel->setText("Game not supported (Generator available)");
        }
    }
    else if (m_currentMode == AppMode::FixManager) {
        // Fix Mode: Hide Patch/Generate, Show Fix
        m_btnPatch->hide();
        m_btnGenerate->hide();
        
        if (hasFix) {
            m_btnApplyFix->setEnabled(true);
            m_btnApplyFix->show();
            m_btnApplyFix->setDescription(QString("Apply fix for %1").arg(data["name"]));
            m_statusLabel->setText(QString("Selected: %1").arg(data["name"]));
        } else {
            m_btnApplyFix->hide();
            m_statusLabel->setText("No fix available for this game");
        }
    }
}

void MainWindow::doPatch() {
    if (m_selectedGame.isEmpty()) return;
    
    m_btnPatch->setEnabled(false);
    m_progress->setValue(0);
    // m_progress->show(); // Hide progress bar as we use terminal now
    
    // Setup Terminal Dialog
    m_terminalDialog->clear();
    m_terminalDialog->appendLog(QString("Initializing patch for: %1").arg(m_selectedGame["name"]), "INFO");
    m_terminalDialog->show();
    
    m_dlWorker = new LuaDownloadWorker(m_selectedGame["appid"], this);
    
    // Connect Signals
    connect(m_dlWorker, &LuaDownloadWorker::finished,
            this, &MainWindow::onPatchDone);
            
    connect(m_dlWorker, &LuaDownloadWorker::progress,
            [this](qint64 downloaded, qint64 total) {
                if (total > 0) {
                    m_progress->setValue(static_cast<int>(downloaded * 100 / total));
                }
            });
            
    connect(m_dlWorker, &LuaDownloadWorker::status,
            [this](QString msg) {
                m_statusLabel->setText(msg);
            });
            
    // Connect detailed logging
    connect(m_dlWorker, &LuaDownloadWorker::log, 
            m_terminalDialog, &TerminalDialog::appendLog);
            
    connect(m_dlWorker, &LuaDownloadWorker::error,
            this, &MainWindow::onPatchError);
            
    m_dlWorker->start();
}

void MainWindow::onPatchDone(QString path) {
    try {
        m_terminalDialog->appendLog("Patch file downloaded. Installing...", "INFO");
        
        QStringList targetDirs = Config::getAllSteamPluginDirs();
        if (targetDirs.isEmpty()) {
            targetDirs.append(Config::getSteamPluginDir());
            m_terminalDialog->appendLog("No cached plugin paths found, using default.", "WARN");
        }

        bool atLeastOneSuccess = false;
        QString lastError;

        for (const QString& pluginDir : targetDirs) {
            m_terminalDialog->appendLog(QString("checking for stplug folder: %1").arg(pluginDir), "INFO");
            
            QDir dir(pluginDir);
            if (dir.exists()) {
                m_terminalDialog->appendLog(QString("found stplug in %1").arg(pluginDir), "INFO");
            } else {
                m_terminalDialog->appendLog(QString("creating stplug folder in %1").arg(pluginDir), "INFO");
                if (!dir.mkpath(pluginDir)) {
                    m_terminalDialog->appendLog(QString("Failed to create directory: %1").arg(pluginDir), "ERROR");
                    continue;
                }
            }
            
            QString dest = dir.filePath(m_selectedGame["appid"] + ".lua");
            
            if (QFile::exists(dest)) {
                m_terminalDialog->appendLog("Removing existing patch file...", "INFO");
                QFile::remove(dest);
            }
            
            m_terminalDialog->appendLog(QString("Copying patch to %1").arg(dest), "INFO");
            if (QFile::copy(path, dest)) {
                m_terminalDialog->appendLog("Copy successful", "SUCCESS");
                atLeastOneSuccess = true;
            } else {
                lastError = "Failed to copy patch file to " + pluginDir;
                m_terminalDialog->appendLog(lastError, "ERROR");
            }
        }

        if (!atLeastOneSuccess) {
             throw std::runtime_error(lastError.toStdString());
        }
        
        // Clean up the downloaded file after copying to all destinations
        QFile::remove(path);
        
        m_progress->hide();
        m_btnPatch->setEnabled(true);
        if(m_btnGenerate->isVisible()) m_btnGenerate->setEnabled(true);
        
        m_statusLabel->setText("Patch Installed!");
        
        m_terminalDialog->appendLog("All operations completed successfully.", "SUCCESS");
        m_terminalDialog->setFinished(true);
        
    } catch (const std::exception& e) {
        onPatchError(QString::fromStdString(e.what()));
    }
}

void MainWindow::onPatchError(QString error) {
    m_progress->hide();
    m_btnPatch->setEnabled(true);
    if(m_btnGenerate->isVisible()) m_btnGenerate->setEnabled(true);
    
    m_statusLabel->setText("Error");
    
    m_terminalDialog->appendLog(QString("Process failed: %1").arg(error), "ERROR");
    m_terminalDialog->setFinished(false);
}

void MainWindow::doGenerate() {
    if (m_selectedGame.isEmpty()) return;
    
    m_btnGenerate->setEnabled(false);
    m_progress->setValue(0);
    
    // Setup Terminal Dialog
    m_terminalDialog->clear();
    m_terminalDialog->appendLog(QString("Initializing generation for: %1 (%2)").arg(m_selectedGame["name"]).arg(m_selectedGame["appid"]), "INFO");
    m_terminalDialog->show();
    
    m_genWorker = new GeneratorWorker(m_selectedGame["appid"], this);
    
    // Connect Signals - GeneratorWorker now handles the file copy internally
    connect(m_genWorker, &GeneratorWorker::finished,
            this, [this](QString path) {
                m_progress->hide();
                m_btnGenerate->setEnabled(true);
                m_statusLabel->setText("Patch Generated & Installed!");
                m_terminalDialog->setFinished(true);
                
                // Update the list item to show green checkmark
                QString appId = m_selectedGame["appid"];
                for (int i = 0; i < m_resultsList->count(); ++i) {
                    QListWidgetItem* item = m_resultsList->item(i);
                    QMap<QString, QString> data = item->data(Qt::UserRole).value<QMap<QString, QString>>();
                    
                    if (data["appid"] == appId) {
                        // Update status to patched/supported
                        data["supported"] = "true";
                        item->setData(Qt::UserRole, QVariant::fromValue(data));
                        
                        // Update display text
                        QString name = data["name"];
                        item->setText(QString("%1\nPatched • ID: %2").arg(name).arg(appId));
                        
                        // Update icon to green checkmark
                        item->setIcon(createStatusIcon(true));
                        item->setForeground(Colors::toQColor(Colors::ACCENT_GREEN));
                        
                        break;
                    }
                }
                
                // Hide the generate button since it's now patched
                m_btnGenerate->hide();
                
                // Enable the patch button for future use
                m_btnPatch->setEnabled(true);
                m_btnPatch->setDescription(QString("Re-patch %1").arg(m_selectedGame["name"]));
            });
            
    connect(m_genWorker, &GeneratorWorker::progress,
            [this](qint64 downloaded, qint64 total) {
                if (total > 0) {
                    m_progress->setValue(static_cast<int>(downloaded * 100 / total));
                }
            });
            
    connect(m_genWorker, &GeneratorWorker::status,
            [this](QString msg) {
                m_statusLabel->setText(msg);
            });
            
    connect(m_genWorker, &GeneratorWorker::log, 
            m_terminalDialog, &TerminalDialog::appendLog);
            
    connect(m_genWorker, &GeneratorWorker::error,
            this, &MainWindow::onPatchError);
            
    m_genWorker->start();
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

void MainWindow::doApplyFix() {
    if (m_selectedGame.isEmpty()) return;
    
    // Prompt user to select the game installation folder
    QString gamePath = QFileDialog::getExistingDirectory(
        this,
        QString("Select Game Folder for %1").arg(m_selectedGame["name"]),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (gamePath.isEmpty()) {
        m_statusLabel->setText("Fix cancelled - no folder selected");
        return;
    }
    
    m_btnApplyFix->setEnabled(false);
    m_progress->setValue(0);
    
    // Setup Terminal Dialog
    m_terminalDialog->clear();
    m_terminalDialog->appendLog(QString("Initializing fix for: %1").arg(m_selectedGame["name"]), "INFO");
    m_terminalDialog->appendLog(QString("Target folder: %1").arg(gamePath), "INFO");
    m_terminalDialog->show();
    
    m_fixWorker = new FixDownloadWorker(m_selectedGame["appid"], gamePath, this);
    
    // Connect Signals
    connect(m_fixWorker, &FixDownloadWorker::finished,
            this, [this](QString path) {
                m_progress->hide();
                m_btnApplyFix->setEnabled(true);
                m_statusLabel->setText("Fix Applied Successfully!");
                m_terminalDialog->setFinished(true);
            });
            
    connect(m_fixWorker, &FixDownloadWorker::progress,
            [this](qint64 downloaded, qint64 total) {
                if (total > 0) {
                    m_progress->setValue(static_cast<int>(downloaded * 100 / total));
                }
            });
            
    connect(m_fixWorker, &FixDownloadWorker::status,
            [this](QString msg) {
                m_statusLabel->setText(msg);
            });
            
    connect(m_fixWorker, &FixDownloadWorker::log, 
            m_terminalDialog, &TerminalDialog::appendLog);
            
    connect(m_fixWorker, &FixDownloadWorker::error,
            this, &MainWindow::onPatchError);
            
    m_fixWorker->start();
}

void MainWindow::cancelNameFetches() {
    m_fetchingNames = false;
    for (QNetworkReply* reply : m_activeNameFetches) {
        if (reply) {
            reply->abort();
            reply->deleteLater();
        }
    }
    m_activeNameFetches.clear();
    m_pendingNameFetchIds.clear();
}

void MainWindow::switchMode(AppMode mode) {
    if (m_currentMode == mode) return;
    m_currentMode = mode;
    updateModeUI();
    
    // Clear selection and re-run search/filter
    m_resultsList->clearSelection();
    onGameSelected(nullptr);
    doSearch(); 
}

void MainWindow::updateModeUI() {
    // Update Tab Styles
    QString activeStyle = QString("background: %1; border: 1px solid %2; color: %3; border-radius: 6px; font-weight: bold;")
                          .arg(Colors::GLASS_HOVER)
                          .arg(Colors::ACCENT_BLUE)
                          .arg(Colors::TEXT_PRIMARY);
                          
    QString inactiveStyle = QString("background: transparent; border: 1px solid transparent; color: %1; border-radius: 6px;")
                            .arg(Colors::TEXT_SECONDARY);

    if (m_currentMode == AppMode::LuaPatcher) {
        m_tabLua->setStyleSheet(activeStyle);
        m_tabFix->setStyleSheet(inactiveStyle);
        m_stack->setCurrentIndex(1); // Show list
    } else {
        m_tabLua->setStyleSheet(inactiveStyle);
        m_tabFix->setStyleSheet(activeStyle);
        m_stack->setCurrentIndex(1); // Show list
    }
}

void MainWindow::startBatchNameFetch() {
    if (m_pendingNameFetchIds.isEmpty()) {
        m_fetchingNames = false;
        m_spinner->stop();
        return;
    }
    
    m_fetchingNames = true;
    m_nameFetchSearchId = m_currentSearchId;
    m_spinner->start();
    m_statusLabel->setText(QString("Found %1 results • Fetching game names...")
                          .arg(m_resultsList->count()));
    
    // Process up to 5 concurrent requests
    int concurrentLimit = 5;
    for (int i = 0; i < concurrentLimit && !m_pendingNameFetchIds.isEmpty(); ++i) {
        processNextNameFetch();
    }
}

void MainWindow::processNextNameFetch() {
    if (m_pendingNameFetchIds.isEmpty() || !m_fetchingNames) {
        // Check if all fetches are complete
        if (m_activeNameFetches.isEmpty() && m_fetchingNames) {
            m_fetchingNames = false;
            m_spinner->stop();
            m_statusLabel->setText(QString("Found %1 results").arg(m_resultsList->count()));
        }
        return;
    }
    
    QString appId = m_pendingNameFetchIds.takeFirst();
    
    // First try Steam Store API
    QUrl url(QString("https://store.steampowered.com/api/appdetails?appids=%1").arg(appId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("fetch_appid", appId);
    reply->setProperty("fetch_type", "steam_store");
    reply->setProperty("fetch_sid", m_nameFetchSearchId);
    
    m_activeNameFetches.append(reply);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGameNameFetched(reply);
    });
}

void MainWindow::onGameNameFetched(QNetworkReply* reply) {
    reply->deleteLater();
    m_activeNameFetches.removeOne(reply);
    
    // Ignore if search changed
    int fetchSid = reply->property("fetch_sid").toInt();
    if (fetchSid != m_nameFetchSearchId || !m_fetchingNames) {
        processNextNameFetch();
        return;
    }
    
    QString appId = reply->property("fetch_appid").toString();
    QString fetchType = reply->property("fetch_type").toString();
    QString gameName;
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        
        if (fetchType == "steam_store") {
            // Steam Store response: { "appid": { "success": true, "data": { "name": "..." } } }
            if (obj.contains(appId)) {
                QJsonObject root = obj[appId].toObject();
                if (root["success"].toBool() && root.contains("data")) {
                    QJsonObject dataObj = root["data"].toObject();
                    gameName = dataObj["name"].toString();
                }
            }
        } else if (fetchType == "steamspy") {
            // SteamSpy response: { "appid": 123, "name": "..." }
            if (obj.contains("name") && !obj["name"].toString().isEmpty()) {
                gameName = obj["name"].toString();
            }
        }
    }
    
    // If Steam Store failed, try SteamSpy
    if (gameName.isEmpty() && fetchType == "steam_store") {
        QUrl spyUrl(QString("https://steamspy.com/api.php?request=appdetails&appid=%1").arg(appId));
        QNetworkRequest spyRequest(spyUrl);
        spyRequest.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
        
        QNetworkReply* spyReply = m_networkManager->get(spyRequest);
        spyReply->setProperty("fetch_appid", appId);
        spyReply->setProperty("fetch_type", "steamspy");
        spyReply->setProperty("fetch_sid", m_nameFetchSearchId);
        
        m_activeNameFetches.append(spyReply);
        
        connect(spyReply, &QNetworkReply::finished, this, [this, spyReply]() {
            onGameNameFetched(spyReply);
        });
        return;
    }
    
    // Update list item if we got a name
    if (!gameName.isEmpty()) {
        for (int i = 0; i < m_resultsList->count(); ++i) {
            QListWidgetItem* item = m_resultsList->item(i);
            QMap<QString, QString> data = item->data(Qt::UserRole).value<QMap<QString, QString>>();
            
            if (data["appid"] == appId) {
                bool supported = (data["supported"] == "true");
                QString statusText = supported ? "Supported" : "Not Indexed";
                item->setText(QString("%1\n%2 • ID: %3").arg(gameName).arg(statusText).arg(appId));
                
                data["name"] = gameName;
                item->setData(Qt::UserRole, QVariant::fromValue(data));
                break;
            }
        }
    }
    
    // Process next in queue
    processNextNameFetch();
}