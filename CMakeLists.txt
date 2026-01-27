cmake_minimum_required(VERSION 3.16)
project(SteamLuaPatcher VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Option for static build
option(BUILD_STATIC "Build with static Qt libraries" OFF)

# Find Qt6 - Core components only
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# Application icon resource (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
else()
    set(APP_ICON_RESOURCE_WINDOWS "")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/glassbutton.cpp
    src/loadingspinner.cpp
    src/workers/indexdownloadworker.cpp
    src/workers/luadownloadworker.cpp
    src/workers/restartworker.cpp
    src/utils/paths.cpp
    src/utils/colors.cpp
)

set(HEADERS
    src/mainwindow.h
    src/glassbutton.h
    src/loadingspinner.h
    src/workers/indexdownloadworker.h
    src/workers/luadownloadworker.h
    src/workers/restartworker.h
    src/utils/paths.h
    src/utils/colors.h
    src/config.h
)

# Add static plugin initialization for static builds
if(BUILD_STATIC)
    list(APPEND SOURCES src/static_plugins.cpp)
endif()

# Create executable
if(WIN32 AND APP_ICON_RESOURCE_WINDOWS)
    add_executable(SteamLuaPatcher WIN32 ${SOURCES} ${HEADERS} ${APP_ICON_RESOURCE_WINDOWS})
else()
    add_executable(SteamLuaPatcher ${SOURCES} ${HEADERS})
endif()

# Link Qt libraries
target_link_libraries(SteamLuaPatcher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Static build configuration
if(BUILD_STATIC)
    # Link static Qt plugins (only core plugins available in qtbase)
    target_link_libraries(SteamLuaPatcher PRIVATE
        Qt6::QWindowsIntegrationPlugin
        Qt6::QGifPlugin
        Qt6::QJpegPlugin
        Qt6::QICOPlugin
        Qt6::QWindowsVistaStylePlugin
    )
    
    # Windows static linking flags for MSVC
    if(MSVC)
        set_property(TARGET SteamLuaPatcher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
    
    # Windows static linking flags for MinGW
    if(MINGW)
        target_link_options(SteamLuaPatcher PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    
    # Define static build macro
    target_compile_definitions(SteamLuaPatcher PRIVATE QT_STATIC_BUILD)
endif()

# Windows-specific settings
if(WIN32)
    set_target_properties(SteamLuaPatcher PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install
install(TARGETS SteamLuaPatcher
    RUNTIME DESTINATION bin
)
