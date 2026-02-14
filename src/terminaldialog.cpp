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
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);
    
    // Terminal log view - Material surface styling
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Consolas, Monaco, monospace", 10));
    m_logView->setStyleSheet(QString(
        "QTextEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 16px;"
        "    padding: 14px;"
        "    selection-background-color: %4;"
        "}"
        "QScrollBar:vertical {"
        "    background: %5;"
        "    width: 8px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: %3;"
        "    border-radius: 4px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: %6;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
    ).arg(Colors::SURFACE_CONTAINER)
     .arg(Colors::ON_SURFACE)
     .arg(Colors::OUTLINE_VARIANT)
     .arg(Colors::PRIMARY)
     .arg(Colors::SURFACE)
     .arg(Colors::OUTLINE));
    
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(m_logView);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 2);
    m_logView->setGraphicsEffect(shadow);
    
    layout->addWidget(m_logView);
    
    // Close button - Material filled button
    m_closeBtn = new QPushButton("Close", this);
    m_closeBtn->setFixedHeight(44);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 22px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    color: %3;"
        "    font-family: 'Roboto', 'Segoe UI';"
        "}"
        "QPushButton:hover {"
        "    background: %4;"
        "    border-color: %5;"
        "}"
        "QPushButton:pressed {"
        "    background: %1;"
        "}")
        .arg(Colors::SURFACE_CONTAINER_HIGH)
        .arg(Colors::OUTLINE_VARIANT)
        .arg(Colors::ON_SURFACE)
        .arg(Colors::SURFACE_CONTAINER_HIGHEST)
        .arg(Colors::PRIMARY));
    m_closeBtn->hide();
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(m_closeBtn);
    
    // Material surface dialog background
    setStyleSheet(QString(
        "QDialog {"
        "    background: %1;"
        "    border-radius: 28px;"
        "}")
        .arg(Colors::SURFACE_CONTAINER_HIGH));
}

void TerminalDialog::appendLog(const QString& message, const QString& level) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color;
    
    if (level == "INFO") {
        color = Colors::PRIMARY;        // Material primary
    } else if (level == "SUCCESS") {
        color = Colors::ACCENT_GREEN;   // Green
    } else if (level == "ERROR") {
        color = Colors::ERROR;          // Material error
    } else if (level == "WARN") {
        color = Colors::TERTIARY;       // Material tertiary (pink/warm)
    } else {
        color = Colors::OUTLINE;        // Neutral
    }
    
    QString html = QString(
        "<span style='color: %1;'>[%2]</span> "
        "<span style='color: %3; font-weight: bold;'>[%4]</span> "
        "<span style='color: %5;'>%6</span><br>"
    ).arg(Colors::OUTLINE).arg(timestamp).arg(color).arg(level)
     .arg(Colors::ON_SURFACE).arg(message.toHtmlEscaped());
    
    m_logView->insertHtml(html);
    m_logView->ensureCursorVisible();
}

void TerminalDialog::clear() {
    m_logView->clear();
    m_closeBtn->hide();
}

void TerminalDialog::setFinished(bool success) {
    if (success) {
        m_closeBtn->setText("Done");
        m_closeBtn->setStyleSheet(QString(
            "QPushButton {"
            "    background: %1;"
            "    border: none;"
            "    border-radius: 22px;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    color: #FFFFFF;"
            "    font-family: 'Roboto', 'Segoe UI';"
            "}"
            "QPushButton:hover {"
            "    background: %2;"
            "}"
            "QPushButton:pressed {"
            "    background: %3;"
            "}").arg(Colors::ACCENT_GREEN)
                .arg("#7BC06B") // lighter green
                .arg("#5A9E4D")); // darker green
    } else {
        m_closeBtn->setText("Close");
        m_closeBtn->setStyleSheet(QString(
            "QPushButton {"
            "    background: %1;"
            "    border: none;"
            "    border-radius: 22px;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    color: #FFFFFF;"
            "    font-family: 'Roboto', 'Segoe UI';"
            "}"
            "QPushButton:hover {"
            "    background: %2;"
            "}"
            "QPushButton:pressed {"
            "    background: %3;"
            "}").arg(Colors::ERROR_CONTAINER)
                .arg("#A12424")
                .arg("#7A1A1A"));
    }
    m_closeBtn->show();
}
