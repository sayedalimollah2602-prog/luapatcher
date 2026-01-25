/**
 * @file static_plugins.cpp
 * @brief Static Qt plugin imports for standalone executable builds
 * 
 * This file is only compiled when building with static Qt.
 * It imports the necessary Qt plugins at compile time so they
 * are embedded in the executable.
 */

#include <QtPlugin>

// Platform plugin (required for Windows)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

// Image format plugins
Q_IMPORT_PLUGIN(QGifPlugin)
Q_IMPORT_PLUGIN(QJpegPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)

// Windows style
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
