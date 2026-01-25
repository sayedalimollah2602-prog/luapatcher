#include "luagenerationworker.h"
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QWebEngineSettings>

// Custom WebEnginePage to capture console messages
class LoggingWebEnginePage : public QWebEnginePage {
public:
    explicit LoggingWebEnginePage(QObject* parent = nullptr) : QWebEnginePage(parent) {}

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID) override {
        QString levelStr;
        switch(level) {
            case InfoMessageLevel: levelStr = "INFO"; break;
            case WarningMessageLevel: levelStr = "WARN"; break;
            case ErrorMessageLevel: levelStr = "ERROR"; break;
            default: levelStr = "LOG"; break;
        }
        qDebug() << "[JS" << levelStr << "]" << message << "(Line" << lineNumber << ")";
    }
};

LuaGenerationWorker::LuaGenerationWorker(const QString& appId, QObject* parent)
    : QObject(parent)
    , m_appId(appId)
    , m_page(new LoggingWebEnginePage(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_pollTimer(new QTimer(this))
    , m_pollAttempts(0)
{
    // Enable remote access and disable security for cross-origin requests
    m_page->settings()->setAttribute(QWebEngineSettings::WebAttribute::WebSecurityEnabled, false);
    m_page->settings()->setAttribute(QWebEngineSettings::WebAttribute::LocalContentCanAccessRemoteUrls, true);
    m_page->settings()->setAttribute(QWebEngineSettings::WebAttribute::LocalContentCanAccessFileUrls, true);

    connect(m_page, &QWebEnginePage::loadFinished, this, &LuaGenerationWorker::onPageLoadFinished);
}

LuaGenerationWorker::~LuaGenerationWorker()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
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

    // 1. Inject Mock DOM Elements
    // generator.js expects #gid (input), #go (button), #msg, #actions, #dl, #open to exist.
    // If they don't, it crashes immediately.
    QString mockDomScript = R"(
        (function() {
            console.log("Injecting Mock DOM for generator.js compatibility...");
            if (!document.getElementById('gid')) {
                var container = document.createElement('div');
                container.id = 'mock-container';
                container.style.display = 'none';
                container.innerHTML = `
                    <input id="gid" value="">
                    <button id="go"></button>
                    <div id="msg"></div>
                    <div id="actions">
                        <button id="dl"></button>
                        <button id="open"></button>
                    </div>
                `;
                document.body.appendChild(container);
            }
        })();
    )";
    
    // Run mock injection first
    m_page->runJavaScript(mockDomScript);

    // 2. Inject and run the actual generator
    m_page->runJavaScript(scriptSource);
    
    // 3. Call our wrapper with Promise handling
    // IMPORTANT: generateLua() is an async function that returns a Promise.
    // Qt's runJavaScript callback receives the Promise object, not the resolved value.
    // We need to handle the Promise in JS and store the result in a global variable,
    // then poll for it from C++.
    emit status("Generating Lua link...");
    
    QString promiseHandlerCode = QString(R"(
        (function() {
            window.__luaGenerationResult = null;
            window.__luaGenerationError = null;
            window.__luaGenerationDone = false;
            
            window.generateLua('%1')
                .then(function(url) {
                    console.log('Promise resolved with URL:', url);
                    window.__luaGenerationResult = url;
                    window.__luaGenerationDone = true;
                })
                .catch(function(err) {
                    console.error('Promise rejected:', err);
                    window.__luaGenerationError = err ? err.toString() : 'Unknown error';
                    window.__luaGenerationDone = true;
                });
        })();
    )").arg(m_appId);
    
    m_page->runJavaScript(promiseHandlerCode);
    
    // 4. Start polling for the result
    startPolling();
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

void LuaGenerationWorker::startPolling()
{
    m_pollAttempts = 0;
    connect(m_pollTimer, &QTimer::timeout, this, &LuaGenerationWorker::pollResult);
    m_pollTimer->start(100); // Poll every 100ms
}

void LuaGenerationWorker::pollResult()
{
    m_pollAttempts++;
    
    // Check if we've exceeded max attempts (30 seconds = 300 attempts @ 100ms)
    if (m_pollAttempts > 300) {
        m_pollTimer->stop();
        emit error("Generation timed out: Script did not return a URL in 30 seconds");
        return;
    }
    
    // Check if the Promise has completed
    m_page->runJavaScript("window.__luaGenerationDone", [this](const QVariant& doneResult) {
        bool done = doneResult.toBool();
        if (!done) {
            return; // Still processing, wait for next poll
        }
        
        // Stop polling
        m_pollTimer->stop();
        
        // Check for error first
        m_page->runJavaScript("window.__luaGenerationError", [this](const QVariant& errorResult) {
            QString errorMsg = errorResult.toString();
            if (!errorMsg.isEmpty() && errorMsg != "null") {
                emit error("Generation failed: " + errorMsg);
                return;
            }
            
            // Get the result
            m_page->runJavaScript("window.__luaGenerationResult", [this](const QVariant& urlResult) {
                QString url = urlResult.toString();
                if (url.isEmpty() || url == "null") {
                    emit error("Generation failed: No URL returned");
                    return;
                }
                
                onJsResult(url);
            });
        });
    });
}
