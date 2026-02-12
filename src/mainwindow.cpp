#include "mainwindow.h"
#include "glassbutton.h"
#include "gamecard.h"
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
#include <QScrollBar>
#include <QRandomGenerator>
#include <algorithm>

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
    setMinimumSize(900, 600);
    resize(900, 600);
    
    QString iconPath = Paths::getResourcePath("logo.ico");
    if (QFile::exists(iconPath)) {
        setWindowIcon(QIcon(iconPath));
    }
    
    initUI();
    
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &MainWindow::doSearch);
    
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
    
    QHBoxLayout* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    
    // ---- Sidebar ----
    QWidget* sidebarWidget = new QWidget();
    sidebarWidget->setFixedWidth(200);
    sidebarWidget->setStyleSheet(QString(
        "background-color: %1; border-right: 1px solid %2;"
    ).arg(Colors::GLASS_BG).arg(Colors::GLASS_BORDER));
    
    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(15, 25, 15, 25);
    sidebarLayout->setSpacing(15);
    
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* icon = new QLabel(QString::fromUtf8("⚡"));
    icon->setStyleSheet(QString("font-size: 20px; color: %1; font-weight: bold; background: transparent; border: none;").arg(Colors::ACCENT_BLUE));
    QLabel* title = new QLabel("Lua Patcher");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #E2E8F0; background: transparent; border: none;");
    headerLayout->addWidget(icon);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    sidebarLayout->addLayout(headerLayout);
    sidebarLayout->addSpacing(20);
    
    m_tabLua = new GlassButton(QString::fromUtf8("⬇"), " Lua Patcher", "", Colors::ACCENT_BLUE);
    m_tabLua->setFixedHeight(40);
    connect(m_tabLua, &QPushButton::clicked, this, [this](){ switchMode(AppMode::LuaPatcher); });
    sidebarLayout->addWidget(m_tabLua);
    
    m_tabFix = new GlassButton(QString::fromUtf8("🔧"), " Fix Manager", "", Colors::ACCENT_PURPLE);
    m_tabFix->setFixedHeight(40);
    connect(m_tabFix, &QPushButton::clicked, this, [this](){ switchMode(AppMode::FixManager); });
    sidebarLayout->addWidget(m_tabFix);
    
    sidebarLayout->addSpacing(10);
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background: rgba(255, 255, 255, 0.1);");
    sidebarLayout->addWidget(line);
    sidebarLayout->addSpacing(10);
    
    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Colors::TEXT_SECONDARY));
    m_statusLabel->setWordWrap(true);
    sidebarLayout->addWidget(m_statusLabel);
    sidebarLayout->addStretch();
    
    m_btnAddToLibrary = new GlassButton(QString::fromUtf8("➕"), "Add to Library", "Install / Generate Patch", Colors::ACCENT_GREEN);
    m_btnAddToLibrary->setFixedHeight(50);
    m_btnAddToLibrary->setEnabled(false);
    connect(m_btnAddToLibrary, &QPushButton::clicked, this, &MainWindow::doAddGame);
    sidebarLayout->addWidget(m_btnAddToLibrary);
    
    m_btnApplyFix = new GlassButton(QString::fromUtf8("🔧"), "Apply Fix", "Apply Fix Files", Colors::ACCENT_PURPLE);
    m_btnApplyFix->setFixedHeight(50);
    m_btnApplyFix->setEnabled(false);
    m_btnApplyFix->hide();
    connect(m_btnApplyFix, &QPushButton::clicked, this, &MainWindow::doApplyFix);
    sidebarLayout->addWidget(m_btnApplyFix);
    
    sidebarLayout->addSpacing(10);
    m_btnRestart = new GlassButton(QString::fromUtf8("↻"), "Restart Steam", "Apply Changes", Colors::ACCENT_PURPLE);
    m_btnRestart->setFixedHeight(50);
    connect(m_btnRestart, &QPushButton::clicked, this, &MainWindow::doRestart);
    sidebarLayout->addWidget(m_btnRestart);
    sidebarLayout->addSpacing(20);
    
    QLabel* infoLabel = new QLabel(QString("v%1<br>by <a href=\"https://github.com/sayedalimollah2602-prog\" style=\"color: %2; text-decoration: none;\">leVI</a> & <a href=\"https://github.com/raxnmint\" style=\"color: %2; text-decoration: none;\">raxnmint</a>").arg(Config::APP_VERSION).arg(Colors::TEXT_SECONDARY));
    infoLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold;").arg(Colors::TEXT_SECONDARY));
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    infoLabel->setOpenExternalLinks(true);
    sidebarLayout->addWidget(infoLabel);
    rootLayout->addWidget(sidebarWidget);

    // ---- Content Area ----
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(8);
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Find a game...");
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(m_searchInput);
    shadow->setBlurRadius(20); shadow->setColor(QColor(0, 0, 0, 80)); shadow->setOffset(0, 4);
    m_searchInput->setGraphicsEffect(shadow);
    searchLayout->addWidget(m_searchInput);
    
    QPushButton* refreshBtn = new QPushButton(QString::fromUtf8("↻"));
    refreshBtn->setFixedSize(36, 36);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QString(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: 8px; font-size: 16px; font-weight: bold; color: %3; }"
        "QPushButton:hover { background: %4; border-color: %5; }"
        "QPushButton:pressed { background: %6; }"
    ).arg(Colors::GLASS_BG).arg(Colors::GLASS_BORDER).arg(Colors::TEXT_PRIMARY)
     .arg(Colors::GLASS_HOVER).arg(Colors::ACCENT_BLUE).arg(Colors::GLASS_BG));
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_searchInput->text().trimmed().isEmpty()) startSync(); else doSearch();
    });
    searchLayout->addWidget(refreshBtn);
    mainLayout->addLayout(searchLayout);
    
    // Stacked widget: page 0 = loading, page 1 = grid
    m_stack = new QStackedWidget();
    
    QWidget* pageLoading = new QWidget();
    QVBoxLayout* layLoading = new QVBoxLayout(pageLoading);
    layLoading->setAlignment(Qt::AlignCenter);
    m_spinner = new LoadingSpinner();
    layLoading->addWidget(m_spinner);
    m_stack->addWidget(pageLoading); // index 0
    
    // ---- GRID PAGE (replaces QListWidget) ----
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: rgba(0,0,0,40); width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,60); border-radius: 4px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );
    
    m_gridContainer = new QWidget();
    m_gridContainer->setStyleSheet("background: transparent;");
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(4, 4, 4, 4);
    m_gridLayout->setSpacing(12);
    
    m_scrollArea->setWidget(m_gridContainer);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::loadVisibleThumbnails);
    
    m_stack->addWidget(m_scrollArea); // index 1
    mainLayout->addWidget(m_stack);
    
    m_progress = new QProgressBar();
    m_progress->setFixedHeight(4);
    m_progress->setTextVisible(false);
    m_progress->setStyleSheet(QString(
        "QProgressBar { background: %1; border-radius: 2px; }"
        "QProgressBar::chunk { background: %2; border-radius: 2px; }"
    ).arg(Colors::GLASS_BG).arg(Colors::ACCENT_GREEN));
    m_progress->hide();
    mainLayout->addWidget(m_progress);
    
    rootLayout->addWidget(contentWidget);
    m_terminalDialog = new TerminalDialog(this);
    updateModeUI();
}

// ---- Helper: clear all game cards from grid ----
void MainWindow::clearGameCards() {
    m_selectedCard = nullptr;
    for (GameCard* card : m_gameCards) {
        m_gridLayout->removeWidget(card);
        card->deleteLater();
    }
    m_gameCards.clear();
}

// ---- Display random games from supported list ----
void MainWindow::displayRandomGames() {
    clearGameCards();
    m_selectedGame.clear();
    m_btnAddToLibrary->setEnabled(false);
    cancelNameFetches();
    m_pendingNameFetchIds.clear();

    if (m_supportedGames.isEmpty()) return;

    // Copy and shuffle to pick random games
    QList<GameInfo> shuffled = m_supportedGames;
    auto *rng = QRandomGenerator::global();
    for (int i = shuffled.size() - 1; i > 0; --i) {
        int j = rng->bounded(i + 1);
        shuffled.swapItemsAt(i, j);
    }

    int count = qMin(12, shuffled.size());
    for (int i = 0; i < count; ++i) {
        const GameInfo& game = shuffled[i];

        QMap<QString, QString> cd;
        cd["name"] = (game.name.isEmpty() || game.name == game.id || game.name == "Unknown Game")
            ? "Loading..." : game.name;
        cd["appid"] = game.id;
        cd["supported"] = "true";
        cd["hasFix"] = game.hasFix ? "true" : "false";

        if (game.name.isEmpty() || game.name == game.id || game.name == "Unknown Game")
            m_pendingNameFetchIds.append(game.id);

        GameCard* card = new GameCard(m_gridContainer);
        card->setGameData(cd);
        connect(card, &GameCard::clicked, this, &MainWindow::onCardClicked);

        m_gridLayout->addWidget(card, i / 3, i % 3);
        m_gameCards.append(card);

        if (m_thumbnailCache.contains(game.id)) {
            card->setThumbnail(m_thumbnailCache[game.id]);
        }
    }

    QTimer::singleShot(50, this, &MainWindow::loadVisibleThumbnails);
    if (!m_pendingNameFetchIds.isEmpty()) startBatchNameFetch();

    m_statusLabel->setText(QString("Showing %1 random games").arg(m_gameCards.count()));
    m_stack->setCurrentIndex(1);
    m_spinner->stop();
}

// ---- Sync ----
void MainWindow::startSync() {
    m_stack->setCurrentIndex(0);
    m_spinner->start();
    m_syncWorker = new IndexDownloadWorker(this);
    connect(m_syncWorker, &IndexDownloadWorker::finished, this, &MainWindow::onSyncDone);
    connect(m_syncWorker, &IndexDownloadWorker::progress, m_statusLabel, &QLabel::setText);
    connect(m_syncWorker, &IndexDownloadWorker::error, this, &MainWindow::onSyncError);
    m_syncWorker->start();
}

void MainWindow::onSyncDone(QList<GameInfo> games) {
    m_supportedGames = games;
    m_spinner->stop();
    m_stack->setCurrentIndex(1);
    m_statusLabel->setText("Ready");
    m_searchInput->setFocus();
    if (!m_searchInput->text().isEmpty()) {
        doSearch();
    } else if (m_currentMode == AppMode::LuaPatcher) {
        displayRandomGames();
    } else {
        populateFixList();
    }
}

void MainWindow::onSyncError(QString error) {
    m_spinner->stop();
    m_stack->setCurrentIndex(1);
    m_statusLabel->setText("Offline Mode");
    QMessageBox::warning(this, "Connection Error",
                         QString("Could not sync library:\n%1").arg(error));
}

// ---- Search ----
void MainWindow::onSearchChanged(const QString& text) {
    m_debounceTimer->stop();
    if (!text.trimmed().isEmpty()) {
        m_debounceTimer->start(400);
    } else {
        clearGameCards();
        if (m_currentMode == AppMode::LuaPatcher) {
            displayRandomGames();
        } else {
            populateFixList();
        }
    }
}

void MainWindow::doSearch() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) return;
    if (!m_networkManager) return;
    
    cancelNameFetches();
    m_currentSearchId++;
    m_statusLabel->setText("Searching...");
    
    // Local search
    QJsonArray localResults;
    for (const auto& game : m_supportedGames) {
        if (m_currentMode == AppMode::FixManager && !game.hasFix) continue;
        if (game.name.contains(query, Qt::CaseInsensitive) || game.id == query) {
            QJsonObject item;
            item["id"] = game.id;
            item["name"] = game.name;
            item["supported_local"] = true;
            localResults.append(item);
        }
    }
    displayResults(localResults);
    
    if (m_currentMode == AppMode::FixManager) {
        m_statusLabel->setText(m_gameCards.isEmpty()
            ? "No fixes found for this game"
            : QString("Found %1 games with fixes").arg(m_gameCards.count()));
        m_stack->setCurrentIndex(1);
        m_spinner->stop();
        return;
    }
    
    m_spinner->start();
    if (m_gameCards.isEmpty()) m_stack->setCurrentIndex(0);
    
    bool isNumeric;
    query.toInt(&isNumeric);
    
    if (isNumeric) {
        QUrl urlStore(QString("https://store.steampowered.com/api/appdetails?appids=%1").arg(query));
        QNetworkRequest reqStore(urlStore);
        QNetworkReply* repStore = m_networkManager->get(reqStore);
        repStore->setProperty("sid", m_currentSearchId);
        repStore->setProperty("type", "steam_details");
        repStore->setProperty("query_id", query);
    } else {
        if (m_activeReply) m_activeReply->abort();
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
    
    // Skip non-search replies (thumbnails, name fetches, etc.)
    QString type = reply->property("type").toString();
    if (type.isEmpty()) return;
    
    int sid = reply->property("sid").toInt();
    if (sid != m_currentSearchId) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        if (m_gameCards.isEmpty() && type == "store_search")
            m_statusLabel->setText("Search failed");
        return;
    }
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    
    QList<QJsonObject> newItems;
    
    if (type == "store_search") {
        QJsonArray remoteItems = obj["items"].toArray();
        for (const QJsonValue& val : remoteItems)
            newItems.append(val.toObject());
    }
    else if (type == "steam_details") {
        QString qId = reply->property("query_id").toString();
        bool ok = false;
        if (obj.contains(qId)) {
            QJsonObject root = obj[qId].toObject();
            if (root["success"].toBool() && root.contains("data")) {
                QJsonObject d = root["data"].toObject();
                QJsonObject item;
                item["id"] = d["steam_appid"].toInt();
                item["name"] = d["name"].toString();
                newItems.append(item);
                ok = true;
            }
        }
        if (!ok) {
            QUrl urlSpy(QString("https://steamspy.com/api.php?request=appdetails&appid=%1").arg(qId));
            QNetworkReply* repSpy = m_networkManager->get(QNetworkRequest(urlSpy));
            repSpy->setProperty("sid", sid);
            repSpy->setProperty("type", "steamspy_details");
            return;
        }
    }
    else if (type == "steamspy_details") {
        if (obj.contains("name") && !obj["name"].toString().isEmpty()) {
            QJsonObject item;
            item["id"] = obj["appid"].isDouble() ? obj["appid"].toInt() : obj["appid"].toString().toInt();
            item["name"] = obj["name"].toString();
            newItems.append(item);
        }
    }
    
    m_spinner->stop();
    m_stack->setCurrentIndex(1);
    
    // Build map of existing cards for merge
    QMap<QString, GameCard*> cardMap;
    for (GameCard* c : m_gameCards) cardMap.insert(c->appId(), c);
    
    bool changed = false;
    
    for (const auto& item : newItems) {
        QString id = QString::number(item["id"].toInt());
        QString name = item["name"].toString("Unknown");
        
        bool supported = false;
        bool hasFix = false;
        for (const auto& g : m_supportedGames) {
            if (g.id == id) { supported = true; hasFix = g.hasFix; break; }
        }
        
        if (cardMap.contains(id)) {
            GameCard* existing = cardMap[id];
            QMap<QString, QString> ed = existing->gameData();
            if (ed["name"].contains("Unknown", Qt::CaseInsensitive) || ed["name"] == id) {
                ed["name"] = name;
                ed["supported"] = supported ? "true" : "false";
                ed["hasFix"] = hasFix ? "true" : "false";
                existing->setGameData(ed);
                changed = true;
            }
        } else {
            QMap<QString, QString> cd;
            cd["name"] = name;
            cd["appid"] = id;
            cd["supported"] = supported ? "true" : "false";
            cd["hasFix"] = hasFix ? "true" : "false";
            
            GameCard* card = new GameCard(m_gridContainer);
            card->setGameData(cd);
            connect(card, &GameCard::clicked, this, &MainWindow::onCardClicked);
            
            int idx = m_gameCards.count();
            m_gridLayout->addWidget(card, idx / 3, idx % 3);
            m_gameCards.append(card);
            cardMap.insert(id, card);
            changed = true;
            
            if (m_thumbnailCache.contains(id)) {
                card->setThumbnail(m_thumbnailCache[id]);
            } else if (!m_activeThumbnailDownloads.contains(id)) {
                m_activeThumbnailDownloads.insert(id);
                QString thumbUrl = QString("https://cdn.akamai.steamstatic.com/steam/apps/%1/header.jpg").arg(id);
                QNetworkReply* tr = m_networkManager->get(QNetworkRequest{QUrl(thumbUrl)});
                tr->setProperty("appid", id);
                connect(tr, &QNetworkReply::finished, this, [this, tr]() {
                    onThumbnailDownloaded(tr);
                });
            }
        }
    }
    
    m_statusLabel->setText(m_gameCards.isEmpty()
        ? "No results found"
        : QString("Found %1 results").arg(m_gameCards.count()));
}

// ---- Display results as grid cards ----
void MainWindow::displayResults(const QJsonArray& items) {
    clearGameCards();
    m_selectedGame.clear();
    m_btnAddToLibrary->setEnabled(false);
    m_pendingNameFetchIds.clear();
    cancelNameFetches();
    
    if (items.isEmpty()) return;
    
    int idx = 0;
    for (const QJsonValue& val : items) {
        QJsonObject item = val.toObject();
        QString name = item["name"].toString("Unknown");
        QString appid = item.contains("id")
            ? (item["id"].isString() ? item["id"].toString() : QString::number(item["id"].toInt()))
            : "0";
        
        bool supported = false;
        bool hasFix = false;
        if (item.contains("supported_local")) {
            supported = true;
            for (const auto& g : m_supportedGames) {
                if (g.id == appid) { hasFix = g.hasFix; break; }
            }
        } else {
            for (const auto& g : m_supportedGames) {
                if (g.id == appid) { supported = true; hasFix = g.hasFix; break; }
            }
        }
        
        QMap<QString, QString> cd;
        cd["name"] = name;
        cd["appid"] = appid;
        cd["supported"] = supported ? "true" : "false";
        cd["hasFix"] = hasFix ? "true" : "false";
        
        GameCard* card = new GameCard(m_gridContainer);
        card->setGameData(cd);
        connect(card, &GameCard::clicked, this, &MainWindow::onCardClicked);
        
        m_gridLayout->addWidget(card, idx / 3, idx % 3);
        m_gameCards.append(card);
        
        if (m_thumbnailCache.contains(appid)) {
            card->setThumbnail(m_thumbnailCache[appid]);
        }
        
        if (name.startsWith("Unknown Game") || name == "Unknown") {
            m_pendingNameFetchIds.append(appid);
        }
        idx++;
    }
    
    m_statusLabel->setText(QString("Found %1 results").arg(items.size()));
    QTimer::singleShot(50, this, &MainWindow::loadVisibleThumbnails);
    
    if (!m_pendingNameFetchIds.isEmpty()) startBatchNameFetch();
}

// ---- Card clicked (replaces onGameSelected) ----
void MainWindow::onCardClicked(GameCard* card) {
    if (m_selectedCard) m_selectedCard->setSelected(false);
    
    if (!card) {
        m_selectedCard = nullptr;
        m_selectedGame.clear();
        m_btnAddToLibrary->setEnabled(false);
        m_btnApplyFix->hide();
        m_statusLabel->setText("Ready");
        return;
    }
    
    m_selectedCard = card;
    card->setSelected(true);
    
    QMap<QString, QString> data = card->gameData();
    m_selectedGame = data;
    bool hasFix = (data["hasFix"] == "true");
    bool isSupported = (data["supported"] == "true");
    
    if (m_currentMode == AppMode::LuaPatcher) {
        m_btnApplyFix->hide();
        m_btnAddToLibrary->show();
        m_btnAddToLibrary->setEnabled(true);
        m_btnAddToLibrary->setText("Add to Library");
        
        if (isSupported) {
            m_btnAddToLibrary->setDescription(QString("Install patch for %1").arg(data["name"]));
            m_btnAddToLibrary->setColor(Colors::ACCENT_GREEN);
        } else {
            m_btnAddToLibrary->setDescription(QString("Generate patch for %1").arg(data["name"]));
            m_btnAddToLibrary->setColor(Colors::ACCENT_BLUE);
        }
        m_statusLabel->setText(QString("Selected: %1").arg(data["name"]));
    } else if (m_currentMode == AppMode::FixManager) {
        m_btnAddToLibrary->hide();
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

// ---- Patch / Generate / Restart / Fix ----
void MainWindow::doAddGame() {
    if (m_selectedGame.isEmpty()) return;
    bool isSupported = (m_selectedGame["supported"] == "true");
    if (isSupported) runPatchLogic(); else runGenerateLogic();
}

void MainWindow::runPatchLogic() {
    if (m_selectedGame.isEmpty()) return;
    m_btnAddToLibrary->setEnabled(false);
    m_progress->setValue(0);
    m_terminalDialog->clear();
    m_terminalDialog->appendLog(QString("Initializing patch for: %1").arg(m_selectedGame["name"]), "INFO");
    m_terminalDialog->show();
    
    m_dlWorker = new LuaDownloadWorker(m_selectedGame["appid"], this);
    connect(m_dlWorker, &LuaDownloadWorker::finished, this, &MainWindow::onPatchDone);
    connect(m_dlWorker, &LuaDownloadWorker::progress, [this](qint64 dl, qint64 total) {
        if (total > 0) m_progress->setValue(static_cast<int>(dl * 100 / total));
    });
    connect(m_dlWorker, &LuaDownloadWorker::status, [this](QString msg) { m_statusLabel->setText(msg); });
    connect(m_dlWorker, &LuaDownloadWorker::log, m_terminalDialog, &TerminalDialog::appendLog);
    connect(m_dlWorker, &LuaDownloadWorker::error, this, &MainWindow::onPatchError);
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
        bool ok = false;
        QString lastErr;
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
            if (QFile::exists(dest)) { m_terminalDialog->appendLog("Removing existing patch file...", "INFO"); QFile::remove(dest); }
            m_terminalDialog->appendLog(QString("Copying patch to %1").arg(dest), "INFO");
            if (QFile::copy(path, dest)) { m_terminalDialog->appendLog("Copy successful", "SUCCESS"); ok = true; }
            else { lastErr = "Failed to copy patch file to " + pluginDir; m_terminalDialog->appendLog(lastErr, "ERROR"); }
        }
        if (!ok) throw std::runtime_error(lastErr.toStdString());
        QFile::remove(path);
        m_progress->hide();
        m_btnAddToLibrary->setEnabled(true);
        m_statusLabel->setText("Patch Installed!");
        m_terminalDialog->appendLog("All operations completed successfully.", "SUCCESS");
        m_terminalDialog->setFinished(true);
    } catch (const std::exception& e) {
        onPatchError(QString::fromStdString(e.what()));
    }
}

void MainWindow::onPatchError(QString error) {
    m_progress->hide();
    m_btnAddToLibrary->setEnabled(true);
    m_statusLabel->setText("Error");
    m_terminalDialog->appendLog(QString("Process failed: %1").arg(error), "ERROR");
    m_terminalDialog->setFinished(false);
}

void MainWindow::runGenerateLogic() {
    if (m_selectedGame.isEmpty()) return;
    m_btnAddToLibrary->setEnabled(false);
    m_progress->setValue(0);
    m_terminalDialog->clear();
    m_terminalDialog->appendLog(QString("Initializing generation for: %1 (%2)").arg(m_selectedGame["name"]).arg(m_selectedGame["appid"]), "INFO");
    m_terminalDialog->show();
    
    m_genWorker = new GeneratorWorker(m_selectedGame["appid"], this);
    connect(m_genWorker, &GeneratorWorker::finished, this, [this](QString) {
        m_progress->hide();
        m_btnAddToLibrary->setEnabled(true);
        m_statusLabel->setText("Patch Generated & Installed!");
        m_terminalDialog->setFinished(true);
        QString appId = m_selectedGame["appid"];
        for (GameCard* card : m_gameCards) {
            if (card->appId() == appId) {
                QMap<QString, QString> d = card->gameData();
                d["supported"] = "true";
                card->setGameData(d);
                break;
            }
        }
        m_btnAddToLibrary->setDescription(QString("Re-patch %1").arg(m_selectedGame["name"]));
        m_btnAddToLibrary->setColor(Colors::ACCENT_GREEN);
    });
    connect(m_genWorker, &GeneratorWorker::progress, [this](qint64 dl, qint64 total) {
        if (total > 0) m_progress->setValue(static_cast<int>(dl * 100 / total));
    });
    connect(m_genWorker, &GeneratorWorker::status, [this](QString msg) { m_statusLabel->setText(msg); });
    connect(m_genWorker, &GeneratorWorker::log, m_terminalDialog, &TerminalDialog::appendLog);
    connect(m_genWorker, &GeneratorWorker::error, this, &MainWindow::onPatchError);
    m_genWorker->start();
}

void MainWindow::doRestart() {
    if (QMessageBox::question(this, "Restart Steam?", "Close Steam and all games?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    m_restartWorker = new RestartWorker(this);
    connect(m_restartWorker, &RestartWorker::finished, m_statusLabel, &QLabel::setText);
    m_restartWorker->start();
}

void MainWindow::doApplyFix() {
    if (m_selectedGame.isEmpty()) return;
    QString gamePath = QFileDialog::getExistingDirectory(this,
        QString("Select Game Folder for %1").arg(m_selectedGame["name"]),
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (gamePath.isEmpty()) { m_statusLabel->setText("Fix cancelled - no folder selected"); return; }
    
    m_btnApplyFix->setEnabled(false);
    m_progress->setValue(0);
    m_terminalDialog->clear();
    m_terminalDialog->appendLog(QString("Initializing fix for: %1").arg(m_selectedGame["name"]), "INFO");
    m_terminalDialog->appendLog(QString("Target folder: %1").arg(gamePath), "INFO");
    m_terminalDialog->show();
    
    m_fixWorker = new FixDownloadWorker(m_selectedGame["appid"], gamePath, this);
    connect(m_fixWorker, &FixDownloadWorker::finished, this, [this](QString) {
        m_progress->hide(); m_btnApplyFix->setEnabled(true);
        m_statusLabel->setText("Fix Applied Successfully!");
        m_terminalDialog->setFinished(true);
    });
    connect(m_fixWorker, &FixDownloadWorker::progress, [this](qint64 dl, qint64 total) {
        if (total > 0) m_progress->setValue(static_cast<int>(dl * 100 / total));
    });
    connect(m_fixWorker, &FixDownloadWorker::status, [this](QString msg) { m_statusLabel->setText(msg); });
    connect(m_fixWorker, &FixDownloadWorker::log, m_terminalDialog, &TerminalDialog::appendLog);
    connect(m_fixWorker, &FixDownloadWorker::error, this, &MainWindow::onPatchError);
    m_fixWorker->start();
}

// ---- Mode switching ----
void MainWindow::cancelNameFetches() {
    m_fetchingNames = false;
    for (QNetworkReply* r : m_activeNameFetches) { if (r) { r->abort(); r->deleteLater(); } }
    m_activeNameFetches.clear();
    m_pendingNameFetchIds.clear();
}

void MainWindow::switchMode(AppMode mode) {
    if (m_currentMode == mode) return;
    m_currentMode = mode;
    updateModeUI();
    onCardClicked(nullptr);
    clearGameCards();
    if (m_currentMode == AppMode::FixManager) {
        populateFixList();
    } else {
        if (m_searchInput->text().trimmed().isEmpty()) {
            displayRandomGames();
        } else {
            doSearch();
        }
    }
}

void MainWindow::populateFixList() {
    m_statusLabel->setText("Listing available fixes...");
    cancelNameFetches();
    m_pendingNameFetchIds.clear();
    
    QJsonArray fixGames;
    for (const auto& game : m_supportedGames) {
        if (game.hasFix) {
            QJsonObject item;
            item["id"] = game.id;
            item["name"] = (game.name.isEmpty() || game.name == game.id || game.name == "Unknown Game")
                ? "Loading..." : game.name;
            if (game.name.isEmpty() || game.name == game.id || game.name == "Unknown Game")
                m_pendingNameFetchIds.append(game.id);
            item["supported_local"] = true;
            fixGames.append(item);
        }
    }
    displayResults(fixGames);
    if (!m_pendingNameFetchIds.isEmpty()) startBatchNameFetch();
    m_statusLabel->setText(m_gameCards.isEmpty()
        ? "No fixes available in current index."
        : QString("Found %1 available fixes").arg(m_gameCards.count()));
    m_stack->setCurrentIndex(1);
    m_spinner->stop();
}

void MainWindow::updateModeUI() {
    m_tabLua->setActive(m_currentMode == AppMode::LuaPatcher);
    m_tabFix->setActive(m_currentMode == AppMode::FixManager);
    m_stack->setCurrentIndex(1);
}

// ---- Batch name fetch ----
void MainWindow::startBatchNameFetch() {
    if (m_pendingNameFetchIds.isEmpty()) { m_fetchingNames = false; m_spinner->stop(); return; }
    m_fetchingNames = true;
    m_nameFetchSearchId = m_currentSearchId;
    m_spinner->start();
    m_statusLabel->setText(QString("Found %1 results %2 Fetching game names...").arg(m_gameCards.count()).arg(QChar(0x2022)));
    for (int i = 0; i < 5 && !m_pendingNameFetchIds.isEmpty(); ++i) processNextNameFetch();
}

void MainWindow::processNextNameFetch() {
    if (m_pendingNameFetchIds.isEmpty() || !m_fetchingNames) {
        if (m_activeNameFetches.isEmpty() && m_fetchingNames) {
            m_fetchingNames = false; m_spinner->stop();
            m_statusLabel->setText(QString("Found %1 results").arg(m_gameCards.count()));
        }
        return;
    }
    QString appId = m_pendingNameFetchIds.takeFirst();
    QUrl url(QString("https://store.steampowered.com/api/appdetails?appids=%1").arg(appId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("fetch_appid", appId);
    reply->setProperty("fetch_type", "steam_store");
    reply->setProperty("fetch_sid", m_nameFetchSearchId);
    m_activeNameFetches.append(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onGameNameFetched(reply); });
}

void MainWindow::onGameNameFetched(QNetworkReply* reply) {
    reply->deleteLater();
    m_activeNameFetches.removeOne(reply);
    int fetchSid = reply->property("fetch_sid").toInt();
    if (fetchSid != m_nameFetchSearchId || !m_fetchingNames) { processNextNameFetch(); return; }
    
    QString appId = reply->property("fetch_appid").toString();
    QString fetchType = reply->property("fetch_type").toString();
    QString gameName;
    
    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (fetchType == "steam_store") {
            if (obj.contains(appId)) {
                QJsonObject root = obj[appId].toObject();
                if (root["success"].toBool() && root.contains("data"))
                    gameName = root["data"].toObject()["name"].toString();
            }
        } else if (fetchType == "steamspy") {
            if (obj.contains("name") && !obj["name"].toString().isEmpty())
                gameName = obj["name"].toString();
        }
    }
    
    if (gameName.isEmpty() && fetchType == "steam_store") {
        QUrl spyUrl(QString("https://steamspy.com/api.php?request=appdetails&appid=%1").arg(appId));
        QNetworkRequest req(spyUrl);
        req.setHeader(QNetworkRequest::UserAgentHeader, "SteamLuaPatcher/2.0");
        QNetworkReply* spyReply = m_networkManager->get(req);
        spyReply->setProperty("fetch_appid", appId);
        spyReply->setProperty("fetch_type", "steamspy");
        spyReply->setProperty("fetch_sid", m_nameFetchSearchId);
        m_activeNameFetches.append(spyReply);
        connect(spyReply, &QNetworkReply::finished, this, [this, spyReply]() { onGameNameFetched(spyReply); });
        return;
    }
    
    if (!gameName.isEmpty()) {
        for (GameCard* card : m_gameCards) {
            if (card->appId() == appId) {
                QMap<QString, QString> d = card->gameData();
                d["name"] = gameName;
                card->setGameData(d);
                break;
            }
        }
    }
    processNextNameFetch();
}

// ---- Thumbnail lazy loading ----
void MainWindow::loadVisibleThumbnails() {
    if (!m_scrollArea || !m_networkManager) return;
    QRect visibleRect = m_scrollArea->viewport()->rect();
    
    for (GameCard* card : m_gameCards) {
        QPoint pos = m_gridContainer->mapTo(m_scrollArea->viewport(), card->geometry().topLeft());
        QRect cardInView(pos, card->size());
        if (!visibleRect.intersects(cardInView)) continue;
        
        QString appId = card->appId();
        if (appId.isEmpty() || card->hasThumbnail()) continue;
        if (m_thumbnailCache.contains(appId)) { card->setThumbnail(m_thumbnailCache[appId]); continue; }
        if (m_activeThumbnailDownloads.contains(appId)) continue;
        
        m_activeThumbnailDownloads.insert(appId);
        QString thumbUrl = QString("https://cdn.akamai.steamstatic.com/steam/apps/%1/header.jpg").arg(appId);
        QNetworkReply* tr = m_networkManager->get(QNetworkRequest{QUrl(thumbUrl)});
        tr->setProperty("appid", appId);
        connect(tr, &QNetworkReply::finished, this, [this, tr]() { onThumbnailDownloaded(tr); });
    }
}

void MainWindow::onThumbnailDownloaded(QNetworkReply* reply) {
    reply->deleteLater();
    QString appId = reply->property("appid").toString();
    m_activeThumbnailDownloads.remove(appId);
    if (reply->error() != QNetworkReply::NoError || appId.isEmpty()) return;
    
    QPixmap pixmap;
    if (pixmap.loadFromData(reply->readAll())) {
        m_thumbnailCache[appId] = pixmap;
        for (GameCard* card : m_gameCards) {
            if (card->appId() == appId) { card->setThumbnail(pixmap); break; }
        }
    }
}
