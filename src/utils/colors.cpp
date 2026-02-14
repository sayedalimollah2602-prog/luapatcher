#include "colors.h"

// Material Design 3 Dark Theme - Surface
const QString Colors::SURFACE              = "#1C1B1F";
const QString Colors::SURFACE_DIM          = "#141218";
const QString Colors::SURFACE_BRIGHT       = "#3B383E";
const QString Colors::SURFACE_CONTAINER    = "#211F26";
const QString Colors::SURFACE_CONTAINER_HIGH    = "#2B2930";
const QString Colors::SURFACE_CONTAINER_HIGHEST = "#36343B";
const QString Colors::SURFACE_VARIANT      = "#49454F";
const QString Colors::ON_SURFACE           = "#E6E1E5";
const QString Colors::ON_SURFACE_VARIANT   = "#CAC4D0";
const QString Colors::OUTLINE              = "#938F99";
const QString Colors::OUTLINE_VARIANT      = "#49454F";

// Primary (Purple)
const QString Colors::PRIMARY              = "#D0BCFF";
const QString Colors::ON_PRIMARY           = "#381E72";
const QString Colors::PRIMARY_CONTAINER    = "#4F378B";
const QString Colors::ON_PRIMARY_CONTAINER = "#EADDFF";

// Secondary
const QString Colors::SECONDARY            = "#CCC2DC";
const QString Colors::ON_SECONDARY         = "#332D41";
const QString Colors::SECONDARY_CONTAINER  = "#4A4458";

// Tertiary
const QString Colors::TERTIARY             = "#EFB8C8";
const QString Colors::ON_TERTIARY          = "#492532";
const QString Colors::TERTIARY_CONTAINER   = "#633B48";

// Error
const QString Colors::ERROR                = "#F2B8B5";
const QString Colors::ON_ERROR             = "#601410";
const QString Colors::ERROR_CONTAINER      = "#8C1D18";

// Accent aliases (mapped to Material tokens)
const QString Colors::ACCENT_BLUE          = "#D0BCFF";  // Maps to PRIMARY
const QString Colors::ACCENT_PURPLE        = "#CCC2DC";  // Maps to SECONDARY
const QString Colors::ACCENT_GREEN         = "#A8DB8F";  // Material green tone
const QString Colors::ACCENT_RED           = "#F2B8B5";  // Maps to ERROR

// Legacy aliases (mapped to Material surface system)
const QString Colors::BG_GRADIENT_START    = "#1C1B1F";  // SURFACE
const QString Colors::BG_GRADIENT_END      = "#141218";  // SURFACE_DIM
const QString Colors::GLASS_BG             = "rgba(33, 31, 38, 200)";  // SURFACE_CONTAINER
const QString Colors::GLASS_HOVER          = "rgba(43, 41, 48, 220)";  // SURFACE_CONTAINER_HIGH
const QString Colors::GLASS_BORDER         = "rgba(147, 143, 153, 80)"; // OUTLINE
const QString Colors::TEXT_PRIMARY         = "#E6E1E5";  // ON_SURFACE
const QString Colors::TEXT_SECONDARY       = "#CAC4D0";  // ON_SURFACE_VARIANT

QColor Colors::toQColor(const QString& colorStr) {
    return QColor(colorStr);
}
