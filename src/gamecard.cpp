#include "gamecard.h"
#include "utils/colors.h"
#include "materialicons.h"

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

    QRectF cardRect = QRectF(rect()).adjusted(4, 4, -4, -4);
    int radius = 16; // Material M3 standard
    bool supported = (m_data.value("supported") == "true");

    // ── Elevation shadow ──
    if (m_hovered || m_selected) {
        // Level 2 elevation
        for (int i = 4; i >= 1; --i) {
            QColor shadowColor(0, 0, 0, 12 * i);
            QPen shadowPen(shadowColor, 0.5);
            painter.setPen(shadowPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(cardRect.adjusted(-i, -i + 1, i, i + 1), radius + i, radius + i);
        }
    } else {
        // Level 1 elevation
        for (int i = 2; i >= 1; --i) {
            QColor shadowColor(0, 0, 0, 15 * i);
            QPen shadowPen(shadowColor, 0.5);
            painter.setPen(shadowPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(cardRect.adjusted(-i, i * 0.5, i, i + 0.5), radius + i, radius + i);
        }
    }

    // Clip to rounded rect
    QPainterPath clipPath;
    clipPath.addRoundedRect(cardRect, radius, radius);
    painter.setClipPath(clipPath);

    if (m_hasThumbnail) {
        // Stretch thumbnail to fill card
        painter.drawPixmap(cardRect.toRect(), m_thumbnail);
    } else {
        // Material surface container background
        QColor surfaceColor = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGH);
        painter.fillRect(cardRect.toRect(), surfaceColor);

        // Subtle tonal overlay
        QRadialGradient glow(cardRect.center(), cardRect.height() * 0.6);
        glow.setColorAt(0, QColor(208, 188, 255, 10)); // Primary tint
        glow.setColorAt(1, QColor(0, 0, 0, 0));
        painter.fillRect(cardRect.toRect(), glow);

        // Gamepad icon placeholder
        QRectF iconArea(
            cardRect.center().x() - 28,
            cardRect.center().y() - 40,
            56, 56
        );
        QColor iconColor = Colors::toQColor(Colors::ON_SURFACE_VARIANT);
        iconColor.setAlpha(60);
        MaterialIcons::draw(painter, iconArea, iconColor, MaterialIcons::Gamepad);
    }

    // ── Bottom info area ──
    int infoHeight = 62;
    QRectF infoRect(cardRect.left(), cardRect.bottom() - infoHeight,
                    cardRect.width(), infoHeight);

    // Material surface overlay for readability
    QLinearGradient infoGrad(infoRect.topLeft(), infoRect.bottomLeft());
    if (m_hasThumbnail) {
        infoGrad.setColorAt(0, QColor(0, 0, 0, 0));
        infoGrad.setColorAt(0.3, QColor(28, 27, 31, 180));
        infoGrad.setColorAt(1, QColor(28, 27, 31, 240));
    } else {
        infoGrad.setColorAt(0, QColor(0, 0, 0, 0));
        infoGrad.setColorAt(0.3, QColor(20, 18, 24, 120));
        infoGrad.setColorAt(1, QColor(20, 18, 24, 180));
    }
    painter.fillRect(infoRect.toRect(), infoGrad);

    // Game name
    QString name = m_data.value("name", "Unknown");
    QFont nameFont("Roboto", 10, QFont::DemiBold);
    nameFont.setStyleStrategy(QFont::PreferAntialias);
    painter.setFont(nameFont);
    painter.setPen(Colors::toQColor(Colors::ON_SURFACE));

    QRectF nameRect(infoRect.left() + 12, infoRect.top() + 10,
                    infoRect.width() - 24, 22);
    QFontMetrics fm(nameFont);
    QString elidedName = fm.elidedText(name, Qt::ElideRight, (int)nameRect.width());
    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // App ID
    QFont idFont("Roboto", 8);
    idFont.setStyleStrategy(QFont::PreferAntialias);
    painter.setFont(idFont);
    painter.setPen(Colors::toQColor(Colors::ON_SURFACE_VARIANT));
    QRectF idRect(infoRect.left() + 12, infoRect.top() + 34,
                  infoRect.width() - 24, 18);
    painter.drawText(idRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QString("ID: %1").arg(m_data.value("appid", "?")));

    // Reset clip for border drawing
    painter.setClipRect(rect());

    // ── Border & selection state ──
    if (m_selected) {
        // Material primary border
        QPen selPen(Colors::toQColor(Colors::PRIMARY), 2.5);
        painter.setPen(selPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);
    } else if (m_hovered) {
        // Subtle outline on hover
        QPen hovPen(Colors::toQColor(Colors::OUTLINE), 1.2);
        painter.setPen(hovPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);

        // Top highlight shimmer
        painter.setClipPath(clipPath);
        QLinearGradient topShine(cardRect.topLeft(),
                                QPointF(cardRect.left(), cardRect.top() + 30));
        topShine.setColorAt(0, QColor(255, 255, 255, 12));
        topShine.setColorAt(1, QColor(255, 255, 255, 0));
        painter.fillRect(QRectF(cardRect.left(), cardRect.top(),
                                cardRect.width(), 30), topShine);
        painter.setClipRect(rect());
    } else {
        // Resting outline variant
        QPen borderPen(Colors::toQColor(Colors::OUTLINE_VARIANT), 1);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);
    }

    // ── Supported badge ──
    if (supported) {
        painter.setClipPath(clipPath);

        QRectF badgeRect(cardRect.right() - 30, cardRect.top() + 6, 24, 24);

        // Material container background
        QColor badgeColor = Colors::toQColor(Colors::ACCENT_GREEN);
        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeRect, 12, 12);
        painter.fillPath(badgePath, badgeColor);

        // Check icon
        QRectF checkRect = badgeRect.adjusted(4, 4, -4, -4);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::NoBrush);

        QPen checkPen(QColor("#FFFFFF"), 2.2);
        checkPen.setCapStyle(Qt::RoundCap);
        checkPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(checkPen);

        QPainterPath check;
        check.moveTo(checkRect.left() + 1, checkRect.center().y());
        check.lineTo(checkRect.center().x() - 1, checkRect.bottom() - 2);
        check.lineTo(checkRect.right() - 1, checkRect.top() + 2);
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
