#include "gamecard.h"
#include "utils/colors.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFont>

GameCard::GameCard(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(160, 200);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(220);
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

    QRect cardRect = rect().adjusted(5, 5, -5, -5);
    int radius = 14;
    bool supported = (m_data.value("supported") == "true");

    // Clip to rounded rect
    QPainterPath clipPath;
    clipPath.addRoundedRect(cardRect, radius, radius);
    painter.setClipPath(clipPath);

    if (m_hasThumbnail) {
        // Stretch thumbnail to fill the entire card
        painter.drawPixmap(cardRect, m_thumbnail);

        // Subtle dark vignette overlay for depth
        QRadialGradient vignette(cardRect.center(), cardRect.width() * 0.7);
        vignette.setColorAt(0, QColor(0, 0, 0, 0));
        vignette.setColorAt(1, QColor(0, 0, 0, 80));
        painter.fillRect(cardRect, vignette);
    } else {
        // Rich glass background gradient
        QLinearGradient bg(cardRect.topLeft(), cardRect.bottomRight());
        bg.setColorAt(0, QColor(30, 41, 59, 220));
        bg.setColorAt(0.5, QColor(20, 30, 48, 230));
        bg.setColorAt(1, QColor(10, 18, 32, 240));
        painter.fillRect(cardRect, bg);

        // Subtle pattern: inner glow
        QRadialGradient glow(cardRect.center(), cardRect.height() * 0.5);
        glow.setColorAt(0, QColor(59, 130, 246, 15));
        glow.setColorAt(1, QColor(0, 0, 0, 0));
        painter.fillRect(cardRect, glow);

        // Large default gamepad icon
        QFont iconFont("Segoe UI Emoji", 48);
        painter.setFont(iconFont);
        painter.setPen(QColor(148, 163, 184, 80));

        QRect iconArea = cardRect.adjusted(0, -15, 0, -55);
        painter.drawText(iconArea, Qt::AlignCenter, QString::fromUtf8("\xF0\x9F\x8E\xAE"));
    }

    // ---- Bottom info gradient overlay ----
    int infoHeight = 65;
    QRect infoRect(cardRect.left(), cardRect.bottom() - infoHeight, cardRect.width(), infoHeight);

    QLinearGradient infoGrad(infoRect.topLeft(), infoRect.bottomLeft());
    infoGrad.setColorAt(0, QColor(0, 0, 0, 0));
    infoGrad.setColorAt(0.25, QColor(0, 0, 0, 140));
    infoGrad.setColorAt(1, QColor(0, 0, 0, 230));
    painter.fillRect(infoRect, infoGrad);

    // Game name (larger, bolder)
    QString name = m_data.value("name", "Unknown");
    QFont nameFont("Segoe UI", 10, QFont::Bold);
    painter.setFont(nameFont);
    painter.setPen(Qt::white);

    QRect nameRect(infoRect.left() + 12, infoRect.top() + 10, infoRect.width() - 24, 22);
    QFontMetrics fm(nameFont);
    QString elidedName = fm.elidedText(name, Qt::ElideRight, nameRect.width());
    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // App ID line
    QFont idFont("Segoe UI", 8);
    painter.setFont(idFont);
    painter.setPen(QColor(180, 190, 210, 180));
    QRect idRect(infoRect.left() + 12, infoRect.top() + 36, infoRect.width() - 24, 20);
    painter.drawText(idRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QString("ID: %1").arg(m_data.value("appid", "?")));

    // Reset clip for border drawing
    painter.setClipRect(rect());

    // ---- Border & glow effects ----
    if (m_selected) {
        // Outer glow
        for (int i = 3; i >= 1; --i) {
            QPen glowPen(QColor(59, 130, 246, 30 * i), i * 1.5);
            painter.setPen(glowPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(cardRect.adjusted(-i, -i, i, i), radius + i, radius + i);
        }
        // Main selection border
        QPen selPen(Colors::toQColor(Colors::ACCENT_BLUE), 2);
        painter.setPen(selPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);
    } else if (m_hovered) {
        // Hover glow
        QPen hovPen(QColor(255, 255, 255, 50), 1.5);
        painter.setPen(hovPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);

        // Subtle inner highlight at top
        painter.setClipPath(clipPath);
        QLinearGradient topShine(cardRect.topLeft(), QPoint(cardRect.left(), cardRect.top() + 40));
        topShine.setColorAt(0, QColor(255, 255, 255, 20));
        topShine.setColorAt(1, QColor(255, 255, 255, 0));
        painter.fillRect(QRect(cardRect.left(), cardRect.top(), cardRect.width(), 40), topShine);
        painter.setClipRect(rect());
    } else {
        // Resting border
        QPen borderPen(QColor(255, 255, 255, 15), 1);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);
    }

    // Supported badge (top-right corner)
    if (supported) {
        painter.setClipPath(clipPath);
        // Green gradient badge
        QRect badge(cardRect.right() - 28, cardRect.top() + 6, 22, 22);
        QRadialGradient badgeGrad(badge.center(), 11);
        badgeGrad.setColorAt(0, Colors::toQColor(Colors::ACCENT_GREEN));
        badgeGrad.setColorAt(1, QColor(16, 185, 129, 180));
        painter.setPen(Qt::NoPen);
        painter.setBrush(badgeGrad);
        painter.drawEllipse(badge);

        // Checkmark
        painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath check;
        check.moveTo(badge.left() + 5, badge.center().y());
        check.lineTo(badge.center().x() - 1, badge.bottom() - 6);
        check.lineTo(badge.right() - 5, badge.top() + 6);
        painter.drawPath(check);
        painter.setClipRect(rect());
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
