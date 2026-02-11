#include "gamecard.h"
#include "utils/colors.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QFont>

GameCard::GameCard(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(140, 120);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(180);
}

void GameCard::setGameData(const QMap<QString, QString>& data) {
    m_data = data;
    update();
}

QMap<QString, QString> GameCard::gameData() const {
    return m_data;
}

void GameCard::setThumbnail(const QPixmap& pixmap) {
    m_thumbnail = pixmap;
    m_hasThumbnail = !pixmap.isNull();
    update();
}

bool GameCard::hasThumbnail() const {
    return m_hasThumbnail;
}

void GameCard::setSelected(bool selected) {
    m_selected = selected;
    update();
}

bool GameCard::isSelected() const {
    return m_selected;
}

QString GameCard::appId() const {
    return m_data.value("appid");
}

void GameCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QRect cardRect = rect().adjusted(4, 4, -4, -4);
    int radius = 10;

    // Clip to rounded rect
    QPainterPath clipPath;
    clipPath.addRoundedRect(cardRect, radius, radius);
    painter.setClipPath(clipPath);

    if (m_hasThumbnail) {
        // Stretch thumbnail to fill the entire card
        painter.drawPixmap(cardRect, m_thumbnail);
    } else {
        // Dark glass background
        QLinearGradient bg(cardRect.topLeft(), cardRect.bottomLeft());
        bg.setColorAt(0, QColor(30, 41, 59, 200));
        bg.setColorAt(1, QColor(15, 23, 42, 220));
        painter.fillRect(cardRect, bg);

        // Large default gamepad icon
        QFont iconFont("Segoe UI Emoji", 40);
        painter.setFont(iconFont);
        painter.setPen(QColor(148, 163, 184, 120));

        QRect iconArea = cardRect.adjusted(0, 0, 0, -45);
        painter.drawText(iconArea, Qt::AlignCenter, QString::fromUtf8("🎮"));
    }

    // Bottom info gradient overlay
    int infoHeight = 50;
    QRect infoRect(cardRect.left(), cardRect.bottom() - infoHeight, cardRect.width(), infoHeight);

    QLinearGradient infoGrad(infoRect.topLeft(), infoRect.bottomLeft());
    infoGrad.setColorAt(0, QColor(0, 0, 0, 0));
    infoGrad.setColorAt(0.3, QColor(0, 0, 0, 160));
    infoGrad.setColorAt(1, QColor(0, 0, 0, 220));
    painter.fillRect(infoRect, infoGrad);

    // Game name
    QString name = m_data.value("name", "Unknown");
    QFont nameFont("Segoe UI", 9, QFont::Bold);
    painter.setFont(nameFont);
    painter.setPen(Qt::white);

    QRect nameRect = infoRect.adjusted(8, 6, -8, -16);
    QFontMetrics fm(nameFont);
    QString elidedName = fm.elidedText(name, Qt::ElideRight, nameRect.width());
    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // Status line
    bool supported = (m_data.value("supported") == "true");
    QString statusText = supported ? "Supported" : "Not Indexed";
    QString idText = QString("ID: %1").arg(m_data.value("appid", "?"));

    QFont statusFont("Segoe UI", 7);
    painter.setFont(statusFont);
    painter.setPen(supported ? Colors::toQColor(Colors::ACCENT_GREEN)
                             : QColor(148, 163, 184));

    QRect statusRect = infoRect.adjusted(8, 20, -8, -2);
    painter.drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QString("%1 • %2").arg(statusText).arg(idText));

    // Reset clip for border drawing
    painter.setClipRect(rect());

    // Selection / hover border
    if (m_selected) {
        QPen selPen(Colors::toQColor(Colors::ACCENT_BLUE), 2.5);
        painter.setPen(selPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect.adjusted(1, 1, -1, -1), radius, radius);
    } else if (m_hovered) {
        QPen hovPen(QColor(255, 255, 255, 60), 1.5);
        painter.setPen(hovPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect.adjusted(1, 1, -1, -1), radius, radius);
    } else {
        // Subtle border
        QPen borderPen(QColor(255, 255, 255, 20), 1);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect.adjusted(1, 1, -1, -1), radius, radius);
    }

    // Supported accent dot (top-right)
    if (supported) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Colors::toQColor(Colors::ACCENT_GREEN));
        painter.drawEllipse(cardRect.right() - 16, cardRect.top() + 8, 8, 8);
    }
}

void GameCard::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    emit clicked(this);
}

void GameCard::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void GameCard::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_hovered = false;
    update();
}
