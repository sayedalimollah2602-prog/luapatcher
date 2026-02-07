#include "glassbutton.h"
#include "utils/colors.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPen>
#include <QFont>

GlassButton::GlassButton(const QString& iconChar, const QString& title,
                         const QString& description, const QString& accentColor,
                         QWidget* parent)
    : QPushButton(parent)
    , m_iconChar(iconChar)
    , m_titleText(title)
    , m_descText(description)
    , m_accentColor(accentColor)
    , m_isActive(false)
{
    // Removed fixed height to allow dynamic sizing
    setMinimumHeight(40); 
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setCursor(Qt::PointingHandCursor);
    
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(1.0);
}

void GlassButton::setDescription(const QString& desc) {
    m_descText = desc;
    update();
}

void GlassButton::setEnabled(bool enabled) {
    QPushButton::setEnabled(enabled);
    m_opacityEffect->setOpacity(enabled ? 1.0 : 0.5);
    update();
}

void GlassButton::setActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;
    update();
}

void GlassButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Determine state
    bool isHover = underMouse() && isEnabled();
    bool isPressed = isDown() && isEnabled();
    
    QRect rect = this->rect();
    int h = rect.height();
    bool isCompact = (h < 60); // Compact mode for sidebar items
    
    QRect bgRect = rect.adjusted(1, 1, -1, -1);
    
    // 1. Background (Glass)
    QColor bgColor = Colors::toQColor(Colors::GLASS_BG);
    
    if (m_isActive) {
        // Active state background (Accent tint)
        bgColor = QColor(m_accentColor);
        bgColor.setAlpha(30); 
    } else if (isHover) {
        bgColor = Colors::toQColor(Colors::GLASS_HOVER);
    } else if (isPressed) {
        bgColor = QColor(m_accentColor);
        bgColor.setAlpha(40);
    }
    
    QPainterPath path;
    path.addRoundedRect(bgRect, 8, 8); // Reduced radius for cleaner look
    painter.fillPath(path, bgColor);
    
    // 2. Active Indicator (Left Border)
    if (m_isActive) {
        painter.fillRect(0, 4, 3, h - 8, QColor(m_accentColor));
    }
    
    // 3. Border (Optional)
    if (!m_isActive) {
        QColor borderColor = Colors::toQColor(Colors::GLASS_BORDER);
        if (isHover || isPressed) {
            borderColor = QColor(m_accentColor);
            borderColor.setAlpha(80);
        }
        QPen pen(borderColor, 1);
        painter.setPen(pen);
        painter.drawPath(path);
    }
    
    // 4. Icon Box (Left)
    int iconSize = isCompact ? 28 : 40; // Reduced from 48
    int padding = isCompact ? 8 : 16;
    
    QRect iconRect(padding, (h - iconSize) / 2, iconSize, iconSize);
    
    // Icon Background Gradient
    QLinearGradient grad(iconRect.topLeft(), iconRect.bottomRight());
    QColor c1(m_accentColor);
    QColor c2 = c1.darker(150);
    grad.setColorAt(0, c1);
    grad.setColorAt(1, c2);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(grad));
    painter.drawRoundedRect(iconRect, isCompact ? 8 : 10, isCompact ? 8 : 10);
    
    // Icon Text
    painter.setPen(QColor("#FFFFFF"));
    QFont iconFont("Segoe UI Symbol", isCompact ? 12 : 16); // Reduced from 18
    painter.setFont(iconFont);
    painter.drawText(iconRect, Qt::AlignCenter, m_iconChar);
    
    // 5. Text Content
    int textX = padding + iconSize + (isCompact ? 10 : 14);
    int textW = rect.width() - textX - 5;
    
    painter.setPen(m_isActive ? QColor(m_accentColor) : Colors::toQColor(Colors::TEXT_PRIMARY));
    
    if (isCompact || m_descText.isEmpty()) {
        // Center Title Only
        QRect titleRect(textX, 0, textW, h);
        QFont titleFont("Segoe UI", isCompact ? 9 : 10, QFont::Bold); // Reduced from 11
        painter.setFont(titleFont);
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText.trimmed());
    } else {
        // Title & Description
        int titleY = (h / 2) - 10;
        QRect titleRect(textX, titleY - 2, textW, 20);
        QFont titleFont("Segoe UI", 10, QFont::Bold);
        painter.setFont(titleFont);
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText);
        
        QRect descRect(textX, titleY + 18, textW, 16);
        painter.setPen(Colors::toQColor(Colors::TEXT_SECONDARY));
        QFont descFont("Segoe UI", 8); // Reduced from 9
        painter.setFont(descFont);
        painter.drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter, m_descText);
    }
}