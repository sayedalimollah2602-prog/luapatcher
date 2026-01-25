#include "luagenerationworker.h"
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

LuaGenerationWorker::LuaGenerationWorker(const QString& appId, QObject* parent)
    : QObject(parent)
    , m_appId(appId)
    , m_page(new QWebEnginePage(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_reply(nullptr)
{
    connect(m_page, &QWebEnginePage::loadFinished, this, &LuaGenerationWorker::onPageLoadFinished);
}

LuaGenerationWorker::~LuaGenerationWorker()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
}

void LuaGenerationWorker::start()
{
    emit status("Initializing browser engine...");
    // Load a dummy page to initialize the context
    // We use a steam URL to avoid CORS issues if the script fetches from steam
    m_page->setUrl(QUrl("https://store.steampowered.com/about/"));
}

void LuaGenerationWorker::onPageLoadFinished(bool success)
{
    if (!success) {
        emit error("Failed to load browser context");
        return;
    }

    runGeneration();
}

void LuaGenerationWorker::runGeneration()
{
    emit status("Injecting generation script...");

    QFile scriptFile(":/resources/generator.js");
    if (!scriptFile.open(QIODevice::ReadOnly)) {
        emit error("Failed to load generator script resource");
        return;
    }
    QString scriptSource = QString::fromUtf8(scriptFile.readAll());
    scriptFile.close();

    // Inject and run
    m_page->runJavaScript(scriptSource);
    
    // Call our wrapper
    emit status("Generating Lua link...");
    QString code = QString("window.generateLua('%1');").arg(m_appId);
    
    m_page->runJavaScript(code, [this](const QVariant& result) {
        onJsResult(result);
    });
}

void LuaGenerationWorker::onJsResult(const QVariant& result)
{
    QString url = result.toString();
    if (url.isEmpty() || url == "null") {
        emit error("Generation failed: No URL returned");
        return;
    }

    emit status("Downloading Lua file...");
    downloadFile(url);
}

void LuaGenerationWorker::downloadFile(const QString& url)
{
    QNetworkRequest request((QUrl(url)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    
    m_reply = m_networkManager->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &LuaGenerationWorker::onDownloadFinished);
}

void LuaGenerationWorker::onDownloadFinished()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        emit error("Download failed: " + m_reply->errorString());
        m_reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    QByteArray data = m_reply->readAll();
    m_reply->deleteLater();
    m_reply = nullptr;

    if (data.isEmpty()) {
        emit error("Downloaded file is empty");
        return;
    }

    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString filePath = QDir(tempPath).filePath("generated_" + m_appId + ".lua");
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Failed to save generated file");
        return;
    }
    file.write(data);
    file.close();

    emit finished(filePath);
}
