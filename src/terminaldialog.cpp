#include "terminaldialog.h"
#include "utils/colors.h"
#include <QGraphicsDropShadowEffect>
#include <QDateTime>

TerminalDialog::TerminalDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Patch Terminal");
    setFixedSize(600, 400);
    setModal(true);
    
    // Main layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    // Terminal-like log view
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Consolas, Monaco, monospace", 10));
    m_logView->setStyleSheet(QString(
        "QTextEdit {"
        "    background-color: #0D1117;"
        "    color: #C9D1D9;"
        "    border: 1px solid %1;"
        "    border-radius: 8px;"
        "    padding: 12px;"
        "    selection-background-color: #388BFD;"
        "}"
        "QScrollBar:vertical {"
        "    background: #161B22;"
        "    width: 10px;"
        "    border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #30363D;"
        "    border-radius: 5px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #484F58;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
    ).arg(Colors::GLASS_BORDER));
    
    // Add shadow effect
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(m_logView);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 4);
    m_logView->setGraphicsEffect(shadow);
    
    layout->addWidget(m_logView);
    
    // Close button (hidden until operation completes)
    m_closeBtn = new QPushButton("Close", this);
    m_closeBtn->setFixedHeight(40);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    color: %3;"
        "}"
        "QPushButton:hover {"
        "    background: %4;"
        "    border-color: %5;"
        "}"
        "QPushButton:pressed {"
        "    background: %6;"
        "}")
        .arg(Colors::GLASS_BG)
        .arg(Colors::GLASS_BORDER)
        .arg(Colors::TEXT_PRIMARY)
        .arg(Colors::GLASS_HOVER)
        .arg(Colors::ACCENT_BLUE)
        .arg(Colors::GLASS_BG));
    m_closeBtn->hide();
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(m_closeBtn);
    
    // Dialog styling
    setStyleSheet(QString(
        "QDialog {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %1, stop:1 %2);"
        "    border-radius: 12px;"
        "}")
        .arg(Colors::BG_GRADIENT_START)
        .arg(Colors::BG_GRADIENT_END));
}

void TerminalDialog::appendLog(const QString& message, const QString& level) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color;
    
    if (level == "INFO") {
        color = "#58A6FF";  // Blue
    } else if (level == "SUCCESS") {
        color = "#3FB950";  // Green
    } else if (level == "ERROR") {
        color = "#F85149";  // Red
    } else if (level == "WARN") {
        color = "#D29922";  // Yellow/Orange
    } else {
        color = "#8B949E";  // Gray
    }
    
    QString html = QString(
        "<span style='color: #484F58;'>[%1]</span> "
        "<span style='color: %2; font-weight: bold;'>[%3]</span> "
        "<span style='color: #C9D1D9;'>%4</span><br>"
    ).arg(timestamp).arg(color).arg(level).arg(message.toHtmlEscaped());
    
    m_logView->insertHtml(html);
    m_logView->ensureCursorVisible();
}

void TerminalDialog::clear() {
    m_logView->clear();
    m_closeBtn->hide();
}

void TerminalDialog::setFinished(bool success) {
    if (success) {
        m_closeBtn->setText("✓ Close");
        m_closeBtn->setStyleSheet(QString(
            "QPushButton {"
            "    background: %1;"
            "    border: 1px solid %1;"
            "    border-radius: 8px;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    color: white;"
            "}"
            "QPushButton:hover {"
            "    background: #2EA043;"
            "}"
            "QPushButton:pressed {"
            "    background: #238636;"
            "}").arg(Colors::ACCENT_GREEN));
    } else {
        m_closeBtn->setText("✕ Close");
        m_closeBtn->setStyleSheet(QString(
            "QPushButton {"
            "    background: %1;"
            "    border: 1px solid %1;"
            "    border-radius: 8px;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    color: white;"
            "}"
            "QPushButton:hover {"
            "    background: #DA3633;"
            "}"
            "QPushButton:pressed {"
            "    background: #B62324;"
            "}").arg(Colors::ACCENT_RED));
    }
    m_closeBtn->show();
}
