cmake_minimum_required(VERSION 3.16)
project(SteamLuaPatcher VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# Application icon resource (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
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
