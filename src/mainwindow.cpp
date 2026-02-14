#include "mainwindow.h"
#include "glassbutton.h"
#include "gamecard.h"
#include "loadingspinner.h"
#include "materialicons.h"
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

// ── Inline helper: a QWidget that paints a single Material icon ──
class MaterialIconWidget : public QWidget {
public:
    MaterialIconWidget(MaterialIcons::Icon icon, const QColor& color, int size = 24, QWidget* parent = nullptr)
        : QWidget(parent), m_icon(icon), m_color(color) {
        setFixedSize(size, size);
        setAttribute(Qt::WA_TranslucentBackground);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(4, 4, width() - 8, height() - 8);
        MaterialIcons::draw(p, r, m_color, m_icon);
    }
private:
    MaterialIcons::Icon m_icon;
    QColor m_color;
};

// ── Inline helper: a QPushButton that paints a Material icon ──
class MaterialIconButton : public QPushButton {
public:
    MaterialIconButton(MaterialIcons::Icon icon, const QColor& color, int size = 40, QWidget* parent = nullptr)
        : QPushButton(parent), m_icon(icon), m_color(color) {
        setFixedSize(size, size);
        setCursor(Qt::PointingHandCursor);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // Background
        QColor bg = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGH);
        if (underMouse()) bg = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGHEST);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()), width() / 2.0, height() / 2.0);
        p.fillPath(path, bg);
        // Icon
        int pad = 10;
        QRectF iconRect(pad, pad, width() - 2 * pad, height() - 2 * pad);
        MaterialIcons::draw(p, iconRect, m_color, m_icon);
    }
private:
    MaterialIcons::Icon m_icon;
    QColor m_color;
};

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
    // Material Design 3: Solid surface color (no gradient)
    painter.fillRect(rect(), Colors::toQColor(Colors::SURFACE));
}

void MainWindow::initUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    
    QHBoxLayout* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    
    // ──── Material Navigation Rail (Sidebar) ────
    QWidget* sidebarWidget = new QWidget();
    sidebarWidget->setFixedWidth(230);
    sidebarWidget->setStyleSheet(QString(
        "background-color: %1; border-right: 1px solid %2;"
    ).arg(Colors::SURFACE_CONTAINER).arg(Colors::OUTLINE_VARIANT));
    
    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(16, 24, 16, 16);
    sidebarLayout->setSpacing(8);
    
    // ── App header with actual logo ──
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);
    
    QLabel* appIconLabel = new QLabel();
    appIconLabel->setFixedSize(36, 36);
    QString iconPath = Paths::getResourcePath("logo.ico");
    if (QFile::exists(iconPath)) {
        QPixmap logoPixmap(iconPath);
        appIconLabel->setPixmap(logoPixmap.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // Fallback: draw a Material Flash icon
        appIconLabel->setStyleSheet(QString(
            "background: %1; border-radius: 10px; border: none;"
        ).arg(Colors::PRIMARY_CONTAINER));
    }
    appIconLabel->setStyleSheet(appIconLabel->styleSheet() + " border: none; background: transparent;");
    headerLayout->addWidget(appIconLabel);
    
    QLabel* title = new QLabel("Lua Patcher");
    title->setStyleSheet(QString(
        "font-size: 17px; font-weight: 700; color: %1; background: transparent; border: none; font-family: 'Roboto', 'Segoe UI';"
    ).arg(Colors::ON_SURFACE));
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    sidebarLayout->addLayout(headerLayout);
    sidebarLayout->addSpacing(20);
    
    // ── Section label ──
    QLabel* navLabel = new QLabel("NAVIGATION");
    navLabel->setStyleSheet(QString(
        "font-size: 10px; font-weight: 600; color: %1; letter-spacing: 1px;"
        " background: transparent; border: none; padding-left: 4px; font-family: 'Roboto', 'Segoe UI';"
    ).arg(Colors::OUTLINE));
    sidebarLayout->addWidget(navLabel);
    sidebarLayout->addSpacing(4);
    
    // Navigation tabs
    m_tabLua = new GlassButton(MaterialIcons::Download, " Lua Patcher", "", Colors::PRIMARY);
    m_tabLua->setFixedHeight(44);
    connect(m_tabLua, &QPushButton::clicked, this, [this](){ switchMode(AppMode::LuaPatcher); });
    sidebarLayout->addWidget(m_tabLua);
    
    m_tabFix = new GlassButton(MaterialIcons::Build, " Fix Manager", "", Colors::SECONDARY);
    m_tabFix->setFixedHeight(44);
    connect(m_tabFix, &QPushButton::clicked, this, [this](){ switchMode(AppMode::FixManager); });
    sidebarLayout->addWidget(m_tabFix);

    m_tabLibrary = new GlassButton(MaterialIcons::Library, " Library", "", Colors::ACCENT_GREEN);
    m_tabLibrary->setFixedHeight(44);
    connect(m_tabLibrary, &QPushButton::clicked, this, [this](){ switchMode(AppMode::Library); });
    sidebarLayout->addWidget(m_tabLibrary);
    
    sidebarLayout->addSpacing(8);
    
    // Material divider
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFixedHeight(1);
    line->setStyleSheet(QString("background: %1; border: none;").arg(Colors::OUTLINE_VARIANT));
    sidebarLayout->addWidget(line);
    sidebarLayout->addSpacing(4);
    
    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px; font-family: 'Roboto', 'Segoe UI'; background: transparent; border: none;"
    ).arg(Colors::ON_SURFACE_VARIANT));
    m_statusLabel->setWordWrap(true);
    sidebarLayout->addWidget(m_statusLabel);
    sidebarLayout->addStretch();
    
    // ── Section label for actions ──
    QLabel* actionsLabel = new QLabel("ACTIONS");
    actionsLabel->setStyleSheet(QString(
        "font-size: 10px; font-weight: 600; color: %1; letter-spacing: 1px;"
        " background: transparent; border: none; padding-left: 4px; font-family: 'Roboto', 'Segoe UI';"
    ).arg(Colors::OUTLINE));
    sidebarLayout->addWidget(actionsLabel);
    sidebarLayout->addSpacing(4);
    
    // Action buttons
    m_btnAddToLibrary = new GlassButton(MaterialIcons::Add, "Add to Library", "Install / Generate Patch", Colors::ACCENT_GREEN);
    m_btnAddToLibrary->setFixedHeight(52);
    m_btnAddToLibrary->setEnabled(false);
    connect(m_btnAddToLibrary, &QPushButton::clicked, this, &MainWindow::doAddGame);
    sidebarLayout->addWidget(m_btnAddToLibrary);

    m_btnApplyFix = new GlassButton(MaterialIcons::Build, "Apply Fix", "Apply Fix Files", Colors::SECONDARY);
    m_btnApplyFix->setFixedHeight(52);
    m_btnApplyFix->setEnabled(false);
    m_btnApplyFix->hide();
    connect(m_btnApplyFix, &QPushButton::clicked, this, &MainWindow::doApplyFix);
    sidebarLayout->addWidget(m_btnApplyFix);

    m_btnRemove = new GlassButton(MaterialIcons::Delete, "Remove", "Remove from Library", Colors::ACCENT_RED);
    m_btnRemove->setFixedHeight(52);
    m_btnRemove->setEnabled(false);
    m_btnRemove->hide();
    connect(m_btnRemove, &QPushButton::clicked, this, &MainWindow::doRemoveGame);
    sidebarLayout->addWidget(m_btnRemove);
    
    sidebarLayout->addSpacing(6);
    m_btnRestart = new GlassButton(MaterialIcons::RestartAlt, "Restart Steam", "Apply Changes", Colors::PRIMARY);
    m_btnRestart->setFixedHeight(52);
    connect(m_btnRestart, &QPushButton::clicked, this, &MainWindow::doRestart);
    sidebarLayout->addWidget(m_btnRestart);
    sidebarLayout->addSpacing(12);
    
    // Divider before version info
    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFixedHeight(1);
    line2->setStyleSheet(QString("background: %1; border: none;").arg(Colors::OUTLINE_VARIANT));
    sidebarLayout->addWidget(line2);
    sidebarLayout->addSpacing(8);
    
    QLabel* infoLabel = new QLabel(QString("v%1<br>by <a href=\"https://github.com/sayedalimollah2602-prog\" style=\"color: %2; text-decoration: none;\">leVI</a> & <a href=\"https://github.com/raxnmint\" style=\"color: %2; text-decoration: none;\">raxnmint</a>").arg(Config::APP_VERSION).arg(Colors::ON_SURFACE_VARIANT));
    infoLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold; font-family: 'Roboto', 'Segoe UI'; background: transparent; border: none;").arg(Colors::ON_SURFACE_VARIANT));
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    infoLabel->setOpenExternalLinks(true);
    sidebarLayout->addWidget(infoLabel);
    rootLayout->addWidget(sidebarWidget);

    // ──── Content Area ────
    QWidget* contentWidget = new QWidget();
    contentWidget->setStyleSheet(QString("background: %1;").arg(Colors::SURFACE_DIM));
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);
    
    // ── Search bar container (Material surface card) ──
    QWidget* searchContainer = new QWidget();
    searchContainer->setStyleSheet(QString(
        "background: %1; border-radius: 16px; border: 1px solid %2;"
    ).arg(Colors::SURFACE_CONTAINER).arg(Colors::OUTLINE_VARIANT));
    searchContainer->setFixedHeight(64);
    QHBoxLayout* searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(8, 8, 8, 8);
    searchLayout->setSpacing(8);
    
    // Search icon
    MaterialIconWidget* searchIconWidget = new MaterialIconWidget(
        MaterialIcons::Search, Colors::toQColor(Colors::ON_SURFACE_VARIANT), 40);
    searchLayout->addWidget(searchIconWidget);
    
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Search games...");
    m_searchInput->setMinimumHeight(40);
    m_searchInput->setStyleSheet(QString(
        "QLineEdit { background: transparent; border: none; border-radius: 0px;"
        " font-size: 15px; color: %1; padding: 0px 4px; }"
        "QLineEdit:focus { border: none; background: transparent; }"
    ).arg(Colors::ON_SURFACE));
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    searchLayout->addWidget(m_searchInput);
    
    // Refresh button with Material icon
    MaterialIconButton* refreshBtn = new MaterialIconButton(
        MaterialIcons::Refresh, Colors::toQColor(Colors::ON_SURFACE_VARIANT), 40);
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_searchInput->text().trimmed().isEmpty()) startSync(); else doSearch();
    });
    searchLayout->addWidget(refreshBtn);
    
    mainLayout->addWidget(searchContainer);
    
    // Stacked widget: page 0 = loading, page 1 = grid
    m_stack = new QStackedWidget();
    
    QWidget* pageLoading = new QWidget();
    QVBoxLayout* layLoading = new QVBoxLayout(pageLoading);
    layLoading->setAlignment(Qt::AlignCenter);
    m_spinner = new LoadingSpinner();
    layLoading->addWidget(m_spinner);
    m_stack->addWidget(pageLoading); // index 0
    
    // Grid page
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(QString(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: %1; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ).arg(Colors::SURFACE).arg(Colors::OUTLINE_VARIANT).arg(Colors::OUTLINE));
    
    m_gridContainer = new QWidget();
    m_gridContainer->setStyleSheet("background: transparent;");
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(4, 4, 4, 4);
    m_gridLayout->setSpacing(14);
    
    m_scrollArea->setWidget(m_gridContainer);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::loadVisibleThumbnails);
    
    m_stack->addWidget(m_scrollArea); // index 1
    mainLayout->addWidget(m_stack);
    
    // Progress bar - Material linear progress
    m_progress = new QProgressBar();
    m_progress->setFixedHeight(4);
    m_progress->setTextVisible(false);
    m_progress->setStyleSheet(QString(
        "QProgressBar { background: %1; border-radius: 2px; }"
        "QProgressBar::chunk { background: %2; border-radius: 2px; }"
    ).arg(Colors::SURFACE_VARIANT).arg(Colors::PRIMARY));
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
    m_stack->setCurrentIndex(1);
    m_spinner->stop();
}

// ---- Display installed patches (Library) ----
void MainWindow::displayLibrary() {
    clearGameCards();
    m_selectedGame.clear();
    m_btnAddToLibrary->setEnabled(false);
    cancelNameFetches();
    m_pendingNameFetchIds.clear();

    QStringList pluginDirs = Config::getAllSteamPluginDirs();
    QSet<QString> installedAppIds;
    
    for (const QString& dirPath : pluginDirs) {
        QDir dir(dirPath);
        QStringList luaFiles = dir.entryList({"*.lua"}, QDir::Files);
        for (const QString& file : luaFiles) {
            QString appId = QFileInfo(file).baseName();
            if (!appId.isEmpty()) installedAppIds.insert(appId);
        }
    }

    if (installedAppIds.isEmpty()) {
        m_statusLabel->setText("No patches installed found.");
        m_stack->setCurrentIndex(1);
        return;
    }

    int count = 0;
    for (const QString& appId : installedAppIds) {
        if (count >= 100) break;

        QString name = "Unknown Game";
        bool hasFix = false;
        
        for (const auto& g : m_supportedGames) {
            if (g.id == appId) {
                name = g.name;
                hasFix = g.hasFix;
                break;
            }
        }

        if (name == "Unknown Game") m_pendingNameFetchIds.append(appId);

        QMap<QString, QString> cd;
        cd["name"] = name;
        cd["appid"] = appId;
        cd["supported"] = "true";
        cd["hasFix"] = hasFix ? "true" : "false";

        GameCard* card = new GameCard(m_gridContainer);
        card->setGameData(cd);
        connect(card, &GameCard::clicked, this, &MainWindow::onCardClicked);

        m_gridLayout->addWidget(card, count / 3, count % 3);
        m_gameCards.append(card);

        if (m_thumbnailCache.contains(appId)) {
            card->setThumbnail(m_thumbnailCache[appId]);
        }
        count++;
    }

    QTimer::singleShot(50, this, &MainWindow::loadVisibleThumbnails);
    if (!m_pendingNameFetchIds.isEmpty()) startBatchNameFetch();

    m_statusLabel->setText(QString("Found %1 installed patches").arg(m_gameCards.count()));
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
    } else if (m_currentMode == AppMode::Library) {
        displayLibrary();
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
        } else if (m_currentMode == AppMode::Library) {
            displayLibrary();
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
    
    QJsonArray localResults;
    int count = 0;
    for (const auto& game : m_supportedGames) {
        if (count >= 100) break;
        if (m_currentMode == AppMode::FixManager && !game.hasFix) continue;
        if (m_currentMode == AppMode::Library) {
        }
        
        if (game.name.contains(query, Qt::CaseInsensitive) || game.id == query) {
            QJsonObject item;
            item["id"] = game.id;
            item["name"] = game.name;
            item["supported_local"] = true;
            localResults.append(item);
            count++;
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
    cancelNameFetches();
    m_pendingNameFetchIds.clear();

    if (items.isEmpty()) return;

    QJsonArray safeItems = items;
    if (safeItems.size() > 120) {
        while (safeItems.size() > 120) safeItems.removeAt(safeItems.size() - 1);
    } 

    int idx = 0;
    for (const QJsonValue& val : safeItems) {
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

// ---- Card clicked ----
void MainWindow::onCardClicked(GameCard* card) {
    if (m_selectedCard) m_selectedCard->setSelected(false);
    
    if (!card) {
        m_selectedCard = nullptr;
        m_selectedGame.clear();
        m_btnAddToLibrary->setEnabled(false);
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
        m_btnAddToLibrary->setEnabled(true);
        if (isSupported) {
            m_btnAddToLibrary->setDescription(QString("Install patch for %1").arg(data["name"]));
            m_btnAddToLibrary->setColor(Colors::ACCENT_GREEN);
        } else {
            m_btnAddToLibrary->setDescription(QString("Generate patch for %1").arg(data["name"]));
            m_btnAddToLibrary->setColor(Colors::PRIMARY);
        }
    } else if (m_currentMode == AppMode::FixManager) {
        if (hasFix) {
            m_btnApplyFix->setEnabled(true);
            m_btnApplyFix->setDescription(QString("Apply fix for %1").arg(data["name"]));
        } else {
            m_btnApplyFix->setEnabled(false);
        }
    } else if (m_currentMode == AppMode::Library) {
        m_btnRemove->setEnabled(true);
        m_btnRemove->setDescription(QString("Remove %1 from Library").arg(data["name"]));
    }
    m_statusLabel->setText(QString("Selected: %1").arg(data["name"]));
}

// ---- Patch / Generate / Restart / Fix / Remove ----
void MainWindow::doAddGame() {
    if (m_selectedGame.isEmpty()) return;
    bool isSupported = (m_selectedGame["supported"] == "true");
    if (isSupported) runPatchLogic(); else runGenerateLogic();
}

void MainWindow::doRemoveGame() {
    if (m_selectedGame.isEmpty()) return;
    QString appId = m_selectedGame["appid"];
    QString name = m_selectedGame["name"];
    
    if (QMessageBox::question(this, "Remove Patch", 
        QString("Are you sure you want to remove the patch for %1?\nThis will delete the lua file from your Steam plugin folder.").arg(name),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        
    QStringList pluginDirs = Config::getAllSteamPluginDirs();
    bool deleted = false;
    
    for (const QString& dirPath : pluginDirs) {
        QDir dir(dirPath);
        QString filePath = dir.filePath(appId + ".lua");
        if (QFile::exists(filePath)) {
            if (QFile::remove(filePath)) deleted = true;
        }
    }
    
    if (deleted) {
        m_statusLabel->setText(QString("Removed patch for %1").arg(name));
        displayLibrary();
    } else {
        QMessageBox::warning(this, "Error", "Failed to remove patch file. It may not exist or is in use.");
    }
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
    
    m_btnAddToLibrary->hide();
    m_btnApplyFix->hide();
    m_btnRemove->hide();
    
    if (m_currentMode == AppMode::LuaPatcher) {
        m_btnAddToLibrary->show();
    } else if (m_currentMode == AppMode::FixManager) {
        m_btnApplyFix->show();
    } else if (m_currentMode == AppMode::Library) {
        m_btnRemove->show();
    }
    
    onCardClicked(nullptr);
    clearGameCards();
    if (m_currentMode == AppMode::FixManager) {
        populateFixList();
    } else if (m_currentMode == AppMode::Library) {
        displayLibrary();
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
    int count = 0;
    for (const auto& game : m_supportedGames) {
        if (count >= 100) break;
        if (game.hasFix) {
            QJsonObject item;
            item["id"] = game.id;
            item["name"] = (game.name.isEmpty() || game.name == game.id || game.name == "Unknown Game")
                ? "Loading..." : game.name;
            if (game.name.isEmpty() || game.name == game.id || game.name == "Unknown Game")
                m_pendingNameFetchIds.append(game.id);
            item["supported_local"] = true;
            fixGames.append(item);
            count++;
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
    m_tabLibrary->setActive(m_currentMode == AppMode::Library);
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
