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

void GameCard::setSkeleton(bool skeleton) {
    if (m_isSkeleton == skeleton) return;
    m_isSkeleton = skeleton;
    if (m_isSkeleton) {
        if (!m_skeletonTimer) {
            m_skeletonTimer = new QTimer(this);
            connect(m_skeletonTimer, &QTimer::timeout, this, &GameCard::updateSkeletonPulse);
        }
        m_skeletonPulse = 0.0;
        m_pulseIncreasing = true;
        m_skeletonTimer->start(30); // ~33 fps
    } else {
        if (m_skeletonTimer) m_skeletonTimer->stop();
        m_skeletonPulse = 0.0;
    }
    update();
}

bool GameCard::isSkeleton() const {
    return m_isSkeleton;
}

void GameCard::updateSkeletonPulse() {
    if (m_pulseIncreasing) {
        m_skeletonPulse += 0.05;
        if (m_skeletonPulse >= 1.0) {
            m_skeletonPulse = 1.0;
            m_pulseIncreasing = false;
        }
    } else {
        m_skeletonPulse -= 0.05;
        if (m_skeletonPulse <= 0.0) {
            m_skeletonPulse = 0.0;
            m_pulseIncreasing = true;
        }
    }
    update();
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

    if (m_isSkeleton) {
        // Base skeleton background
        QColor baseColor = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGH);
        // Blend dynamically with a slightly lighter color for the pulse
        QColor pulseColor = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGHEST);
        
        int r = baseColor.red() + (pulseColor.red() - baseColor.red()) * m_skeletonPulse;
        int g = baseColor.green() + (pulseColor.green() - baseColor.green()) * m_skeletonPulse;
        int b = baseColor.blue() + (pulseColor.blue() - baseColor.blue()) * m_skeletonPulse;
        QColor activeColor(r, g, b);

        painter.fillRect(cardRect.toRect(), activeColor);

        // Draw shimmer overlay
        QLinearGradient shimmer(cardRect.topLeft(), cardRect.bottomRight());
        shimmer.setColorAt(0, QColor(255, 255, 255, 0));
        shimmer.setColorAt(0.5, QColor(255, 255, 255, 10 + 15 * m_skeletonPulse));
        shimmer.setColorAt(1, QColor(255, 255, 255, 0));
        painter.fillRect(cardRect.toRect(), shimmer);

        // Draw skeleton placeholder for thumbnail
        QRectF thumbPlaceholder(cardRect.left(), cardRect.top(), cardRect.width(), cardRect.height() - 62);
        QColor thumbColor = Colors::toQColor(Colors::SURFACE_VARIANT);
        thumbColor.setAlphaF(0.4 + 0.3 * m_skeletonPulse);
        painter.fillRect(thumbPlaceholder.toRect(), thumbColor);

        // Draw skeleton placeholders for text in bottom area
        int infoHeight = 62;
        QRectF infoRect(cardRect.left(), cardRect.bottom() - infoHeight, cardRect.width(), infoHeight);
        QColor infoColor = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGHEST);
        infoColor.setAlphaF(0.5 + 0.3 * m_skeletonPulse);
        
        // Name placeholder
        QRectF namePlaceholder(infoRect.left() + 12, infoRect.top() + 14, infoRect.width() * 0.7, 14);
        QPainterPath namePath;
        namePath.addRoundedRect(namePlaceholder, 6, 6);
        painter.fillPath(namePath, infoColor);

        // ID placeholder
        QRectF idPlaceholder(infoRect.left() + 12, infoRect.top() + 36, infoRect.width() * 0.4, 10);
        QPainterPath idPath;
        idPath.addRoundedRect(idPlaceholder, 5, 5);
        painter.fillPath(idPath, infoColor);

        // Reset clip for border drawing
        painter.setClipRect(rect());

        // Resting outline variant
        QPen borderPen(Colors::toQColor(Colors::OUTLINE_VARIANT), 1);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(cardRect, radius, radius);

        return; // Skip drawing real content
    }

    if (m_hasThumbnail) {
        // Stretch thumbnail to fill card
        painter.drawPixmap(cardRect.toRect(), m_thumbnail);
    } else {
        // Material surface container background
        QColor surfaceColor = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGH);
        painter.fillRect(cardRect.toRect(), surfaceColor);

        // Subtle tonal overlay
        QRadialGradient glow(cardRect.center(), cardRect.height() * 0.6);
        glow.setColorAt(0, QColor(255, 255, 255, 15)); // Soft bright tint for glass
        glow.setColorAt(1, QColor(255, 255, 255, 0));
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
        infoGrad.setColorAt(0.3, QColor(0, 0, 0, 180));
        infoGrad.setColorAt(1, QColor(0, 0, 0, 240));
        painter.fillRect(infoRect.toRect(), infoGrad);
    } else {
        // Flat frosted bottom
        QColor frostedBottom = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGHEST);
        frostedBottom.setAlpha(180);
        painter.fillRect(infoRect.toRect(), frostedBottom);
        // Subtle top border relative to the bottom section
        painter.setPen(QPen(QColor(255,255,255,20), 1));
        painter.drawLine(infoRect.topLeft(), infoRect.topRight());
    }

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
    if (m_isSkeleton) return;
    emit clicked(this);
}

void GameCard::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    if (m_isSkeleton) return;
    m_hovered = true;
    update();
}

void GameCard::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (m_isSkeleton) return;
    m_hovered = false;
    update();
}
