#include "glassbutton.h"
#include "utils/colors.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPen>
#include <QFont>

// Map legacy emoji strings to Material icons
static MaterialIcons::Icon mapEmojiToIcon(const QString& emoji) {
    QString trimmed = emoji.trimmed();
    if (trimmed.contains("🔧") || trimmed.contains("wrench"))
        return MaterialIcons::Build;
    if (trimmed.contains("📚") || trimmed.contains("library") || trimmed.contains("book"))
        return MaterialIcons::Library;
    if (trimmed.contains("↻") || trimmed.contains("restart") || trimmed.contains("refresh"))
        return MaterialIcons::RestartAlt;
    if (trimmed.contains("🗑") || trimmed.contains("delete") || trimmed.contains("trash"))
        return MaterialIcons::Delete;
    if (trimmed.contains("⬇") || trimmed.contains("download"))
        return MaterialIcons::Download;
    if (trimmed.contains("⚡") || trimmed.contains("flash") || trimmed.contains("bolt"))
        return MaterialIcons::Flash;
    if (trimmed.contains("+") || trimmed.contains("add"))
        return MaterialIcons::Add;
    return MaterialIcons::Flash;
}

GlassButton::GlassButton(MaterialIcons::Icon icon, const QString& title,
                         const QString& description, const QString& accentColor,
                         QWidget* parent)
    : QPushButton(parent)
    , m_icon(icon)
    , m_titleText(title)
    , m_descText(description)
    , m_accentColor(accentColor)
    , m_isActive(false)
{
    setMinimumHeight(40);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setCursor(Qt::PointingHandCursor);
    
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(1.0);
}

GlassButton::GlassButton(const QString& iconChar, const QString& title,
                         const QString& description, const QString& accentColor,
                         QWidget* parent)
    : QPushButton(parent)
    , m_icon(mapEmojiToIcon(iconChar))
    , m_titleText(title)
    , m_descText(description)
    , m_accentColor(accentColor)
    , m_isActive(false)
{
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
    m_opacityEffect->setOpacity(enabled ? 1.0 : 0.45);
    update();
}

void GlassButton::setColor(const QString& color) {
    m_accentColor = color;
    update();
}

void GlassButton::setActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;
    update();
}

void GlassButton::setMaterialIcon(MaterialIcons::Icon icon) {
    m_icon = icon;
    update();
}

void GlassButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    bool isHover = underMouse() && isEnabled();
    bool isPressed = isDown() && isEnabled();
    
    QRect r = rect();
    int h = r.height();
    bool isCompact = (h < 60);
    
    QRectF bgRect = QRectF(r).adjusted(1, 1, -1, -1);
    int radius = 16; // Material M3 standard corner radius
    
    // ── Background ──
    QColor bgColor;
    if (m_isActive) {
        // Active: secondary container tint
        bgColor = Colors::toQColor(Colors::SECONDARY_CONTAINER);
        bgColor.setAlpha(180);
    } else if (isPressed) {
        QColor accent(m_accentColor);
        accent.setAlpha(40);
        bgColor = accent;
    } else if (isHover) {
        bgColor = Colors::toQColor(Colors::SURFACE_CONTAINER_HIGH);
    } else {
        bgColor = Colors::toQColor(Colors::SURFACE_CONTAINER);
    }
    
    QPainterPath bgPath;
    bgPath.addRoundedRect(bgRect, radius, radius);
    painter.fillPath(bgPath, bgColor);
    
    // ── Active indicator pill (left) ──
    if (m_isActive) {
        QPainterPath pill;
        pill.addRoundedRect(QRectF(2, h * 0.25, 4, h * 0.5), 2, 2);
        painter.fillPath(pill, Colors::toQColor(Colors::PRIMARY));
    }
    
    // ── Border ──
    if (!m_isActive) {
        QColor borderColor;
        if (isHover || isPressed) {
            borderColor = Colors::toQColor(Colors::PRIMARY);
            borderColor.setAlpha(100);
        } else {
            borderColor = Colors::toQColor(Colors::OUTLINE_VARIANT);
        }
        QPen pen(borderColor, 1);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(bgRect, radius, radius);
    }
    
    // ── Icon ──
    int iconSize = isCompact ? 20 : 24;
    int padding = isCompact ? 10 : 14;
    
    QRectF iconBgRect(padding, (h - (isCompact ? 30 : 36)) / 2.0,
                      isCompact ? 30 : 36, isCompact ? 30 : 36);
    
    // Icon background: rounded container with accent color
    QColor iconBgColor(m_accentColor);
    if (m_isActive) {
        iconBgColor = Colors::toQColor(Colors::PRIMARY);
    }
    
    QPainterPath iconBgPath;
    iconBgPath.addRoundedRect(iconBgRect, isCompact ? 8 : 10, isCompact ? 8 : 10);
    painter.fillPath(iconBgPath, iconBgColor);
    
    // Draw material icon centered in the icon bg
    QRectF iconDrawRect(
        iconBgRect.center().x() - iconSize / 2.0,
        iconBgRect.center().y() - iconSize / 2.0,
        iconSize, iconSize
    );
    
    QColor iconColor = m_isActive
        ? Colors::toQColor(Colors::ON_PRIMARY)
        : QColor("#FFFFFF");
    MaterialIcons::draw(painter, iconDrawRect, iconColor, m_icon);
    
    // ── Text ──
    int textX = padding + iconBgRect.width() + (isCompact ? 10 : 14);
    int textW = r.width() - textX - 8;
    
    QColor textColor = m_isActive
        ? Colors::toQColor(Colors::PRIMARY)
        : Colors::toQColor(Colors::ON_SURFACE);
    painter.setPen(textColor);
    
    if (isCompact || m_descText.isEmpty()) {
        QRectF titleRect(textX, 0, textW, h);
        QFont titleFont("Roboto", isCompact ? 9 : 10, QFont::DemiBold);
        titleFont.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(titleFont);
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText.trimmed());
    } else {
        int titleY = (h / 2) - 10;
        QRectF titleRect(textX, titleY - 2, textW, 20);
        QFont titleFont("Roboto", 10, QFont::DemiBold);
        titleFont.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(titleFont);
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText);
        
        QRectF descRect(textX, titleY + 18, textW, 16);
        painter.setPen(Colors::toQColor(Colors::ON_SURFACE_VARIANT));
        QFont descFont("Roboto", 8);
        descFont.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(descFont);
        painter.drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter, m_descText);
    }
}