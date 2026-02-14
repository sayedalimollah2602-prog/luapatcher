#ifndef COLORS_H
#define COLORS_H

#include <QString>
#include <QColor>

class Colors {
public:
    // Material Design 3 Dark Theme - Surface colors
    static const QString SURFACE;
    static const QString SURFACE_DIM;
    static const QString SURFACE_BRIGHT;
    static const QString SURFACE_CONTAINER;
    static const QString SURFACE_CONTAINER_HIGH;
    static const QString SURFACE_CONTAINER_HIGHEST;
    static const QString SURFACE_VARIANT;
    static const QString ON_SURFACE;
    static const QString ON_SURFACE_VARIANT;
    static const QString OUTLINE;
    static const QString OUTLINE_VARIANT;

    // Primary
    static const QString PRIMARY;
    static const QString ON_PRIMARY;
    static const QString PRIMARY_CONTAINER;
    static const QString ON_PRIMARY_CONTAINER;

    // Secondary
    static const QString SECONDARY;
    static const QString ON_SECONDARY;
    static const QString SECONDARY_CONTAINER;

    // Tertiary
    static const QString TERTIARY;
    static const QString ON_TERTIARY;
    static const QString TERTIARY_CONTAINER;

    // Error
    static const QString ERROR;
    static const QString ON_ERROR;
    static const QString ERROR_CONTAINER;

    // Accent aliases (for backward compatibility during transition)
    static const QString ACCENT_BLUE;
    static const QString ACCENT_PURPLE;
    static const QString ACCENT_GREEN;
    static const QString ACCENT_RED;

    // Legacy aliases
    static const QString BG_GRADIENT_START;
    static const QString BG_GRADIENT_END;
    static const QString GLASS_BG;
    static const QString GLASS_HOVER;
    static const QString GLASS_BORDER;
    static const QString TEXT_PRIMARY;
    static const QString TEXT_SECONDARY;

    // Helper to convert string to QColor
    static QColor toQColor(const QString& colorStr);
};

#endif // COLORS_H
