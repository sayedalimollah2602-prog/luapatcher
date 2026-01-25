#include "colors.h"

const QString Colors::BG_GRADIENT_START = "#0f1c30";
const QString Colors::BG_GRADIENT_END = "#02040a";

const QString Colors::GLASS_BG = "rgba(30, 41, 59, 60)";
const QString Colors::GLASS_HOVER = "rgba(51, 65, 85, 80)";
const QString Colors::GLASS_BORDER = "rgba(255, 255, 255, 25)";

const QString Colors::TEXT_PRIMARY = "#FFFFFF";
const QString Colors::TEXT_SECONDARY = "#94A3B8";

const QString Colors::ACCENT_BLUE = "#3B82F6";
const QString Colors::ACCENT_PURPLE = "#8B5CF6";
const QString Colors::ACCENT_GREEN = "#10B981";
const QString Colors::ACCENT_RED = "#EF4444";

QColor Colors::toQColor(const QString& colorStr) {
    return QColor(colorStr);
}
