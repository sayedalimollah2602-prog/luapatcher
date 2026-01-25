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
{
    setFixedHeight(80);
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

void GlassButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Determine state
    bool isHover = underMouse() && isEnabled();
    bool isPressed = isDown() && isEnabled();
    
    QRect rect = this->rect();
    QRect bgRect = rect.adjusted(1, 1, -1, -1);
    
    // 1. Background (Glass)
    QColor bgColor = Colors::toQColor(Colors::GLASS_BG);
    if (isHover) {
        bgColor = Colors::toQColor(Colors::GLASS_HOVER);
    }
    if (isPressed) {
        bgColor = QColor(m_accentColor);
        bgColor.setAlpha(40);
    }
    
    QPainterPath path;
    path.addRoundedRect(bgRect, 16, 16);
    painter.fillPath(path, bgColor);
    
    // 2. Border
    QColor borderColor = Colors::toQColor(Colors::GLASS_BORDER);
    if (isHover || isPressed) {
        borderColor = QColor(m_accentColor);
        borderColor.setAlpha(100);
    }
    
    QPen pen(borderColor, 1);
    painter.setPen(pen);
    painter.drawPath(path);
    
    // 3. Icon Box (Left)
    int iconSize = 48;
    QRect iconRect(20, (rect.height() - iconSize) / 2, iconSize, iconSize);
    
    // Icon Background Gradient
    QLinearGradient grad(iconRect.topLeft(), iconRect.bottomRight());
    QColor c1(m_accentColor);
    QColor c2 = c1.darker(150);
    grad.setColorAt(0, c1);
    grad.setColorAt(1, c2);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(grad));
    painter.drawRoundedRect(iconRect, 12, 12);
    
    // Icon Text
    painter.setPen(QColor("#FFFFFF"));
    QFont iconFont("Segoe UI Symbol", 18);
    painter.setFont(iconFont);
    painter.drawText(iconRect, Qt::AlignCenter, m_iconChar);
    
    // 4. Text Content
    int textX = 20 + iconSize + 16;
    int textW = rect.width() - textX - 10;
    
    // Title
    QRect titleRect(textX, 18, textW, 24);
    painter.setPen(Colors::toQColor(Colors::TEXT_PRIMARY));
    QFont titleFont("Segoe UI", 11, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText);
    
    // Description
    QRect descRect(textX, 42, textW, 20);
    painter.setPen(Colors::toQColor(Colors::TEXT_SECONDARY));
    QFont descFont("Segoe UI", 9);
    painter.setFont(descFont);
    painter.drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter, m_descText);
}