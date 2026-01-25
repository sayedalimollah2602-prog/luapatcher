#ifndef COLORS_H
#define COLORS_H

#include <QString>
#include <QColor>

class Colors {
public:
    // Deep Blue to Black Gradient
    static const QString BG_GRADIENT_START;
    static const QString BG_GRADIENT_END;
    
    // Glass Components
    static const QString GLASS_BG;
    static const QString GLASS_HOVER;
    static const QString GLASS_BORDER;
    
    // Text
    static const QString TEXT_PRIMARY;
    static const QString TEXT_SECONDARY;
    
    // Accents
    static const QString ACCENT_BLUE;
    static const QString ACCENT_PURPLE;
    static const QString ACCENT_GREEN;
    static const QString ACCENT_RED;
    
    // Helper to convert string to QColor
    static QColor toQColor(const QString& colorStr);
};

#endif // COLORS_H
