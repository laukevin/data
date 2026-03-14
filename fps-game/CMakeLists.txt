cmake_minimum_required(VERSION 3.14)
project(DarkArena VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Find raylib
find_package(raylib REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/game.cpp
    src/player.cpp
    src/level.cpp
    src/enemy.cpp
    src/weapon.cpp
    src/particles.cpp
    src/hud.cpp
    src/audio_manager.cpp
    src/renderer.cpp
    src/network.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib m)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE GL pthread dl rt X11)
endif()
