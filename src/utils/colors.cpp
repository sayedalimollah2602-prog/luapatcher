#include "colors.h"

// Pure Black Theme - Surface colors
const QString Colors::SURFACE              = "#000000";
const QString Colors::SURFACE_DIM          = "#000000";
const QString Colors::SURFACE_BRIGHT       = "#1A1A1A";
const QString Colors::SURFACE_CONTAINER    = "#0A0A0A";
const QString Colors::SURFACE_CONTAINER_HIGH    = "#141414";
const QString Colors::SURFACE_CONTAINER_HIGHEST = "#1E1E1E";
const QString Colors::SURFACE_VARIANT      = "#2A2A2A";
const QString Colors::ON_SURFACE           = "#E6E1E5";
const QString Colors::ON_SURFACE_VARIANT   = "#CAC4D0";
const QString Colors::OUTLINE              = "#6E6E6E";
const QString Colors::OUTLINE_VARIANT      = "#2A2A2A";

// Primary (Purple) - unchanged
const QString Colors::PRIMARY              = "#D0BCFF";
const QString Colors::ON_PRIMARY           = "#381E72";
const QString Colors::PRIMARY_CONTAINER    = "#4F378B";
const QString Colors::ON_PRIMARY_CONTAINER = "#EADDFF";

// Secondary - unchanged
const QString Colors::SECONDARY            = "#CCC2DC";
const QString Colors::ON_SECONDARY         = "#332D41";
const QString Colors::SECONDARY_CONTAINER  = "#4A4458";

// Tertiary - unchanged
const QString Colors::TERTIARY             = "#EFB8C8";
const QString Colors::ON_TERTIARY          = "#492532";
const QString Colors::TERTIARY_CONTAINER   = "#633B48";

// Error - unchanged
const QString Colors::ERROR                = "#F2B8B5";
const QString Colors::ON_ERROR             = "#601410";
const QString Colors::ERROR_CONTAINER      = "#8C1D18";

// Accent aliases - unchanged
const QString Colors::ACCENT_BLUE          = "#D0BCFF";  // Maps to PRIMARY
const QString Colors::ACCENT_PURPLE        = "#CCC2DC";  // Maps to SECONDARY
const QString Colors::ACCENT_GREEN         = "#A8DB8F";  // Material green tone
const QString Colors::ACCENT_RED           = "#F2B8B5";  // Maps to ERROR

// Legacy aliases (mapped to black surface system)
const QString Colors::BG_GRADIENT_START    = "#000000";  // SURFACE
const QString Colors::BG_GRADIENT_END      = "#000000";  // SURFACE_DIM
const QString Colors::GLASS_BG             = "rgba(10, 10, 10, 220)";   // SURFACE_CONTAINER
const QString Colors::GLASS_HOVER          = "rgba(20, 20, 20, 230)";   // SURFACE_CONTAINER_HIGH
const QString Colors::GLASS_BORDER         = "rgba(110, 110, 110, 80)"; // OUTLINE
const QString Colors::TEXT_PRIMARY         = "#E6E1E5";  // ON_SURFACE
const QString Colors::TEXT_SECONDARY       = "#CAC4D0";  // ON_SURFACE_VARIANT

QColor Colors::toQColor(const QString& colorStr) {
    return QColor(colorStr);
}
