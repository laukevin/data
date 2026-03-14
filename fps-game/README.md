# DARK ARENA - 3D First-Person Shooter

A complete 3D first-person shooter written in C++ using the **raylib** open-source game engine.

## Features

- **Procedurally generated levels** - Each of the 5 levels features randomly generated rooms, corridors, and pillars with unique visual themes (Stone Dungeon, Tech Base, Hell, Dark Fortress)
- **3 Weapons** - Pistol, Shotgun, and Assault Rifle with unique fire rates, spread, and damage
- **4 Enemy types** - Grunt, Soldier, Demon (fast melee), and Heavy with AI (patrol, chase, attack states)
- **Full FPS controls** - WASD movement, mouse look, sprint, crouch, jump
- **Procedural textures** - Brick walls, tiled floors, panel ceilings, all generated at runtime
- **Particle system** - Blood, sparks, muzzle flashes, explosions, pickup effects
- **Procedural sound effects** - All audio synthesized at runtime (no asset files needed)
- **HUD** - Health/armor bars, ammo display, weapon slots, crosshair, hit markers, minimap
- **Pickups** - Health, armor, ammo, and weapon pickups scattered throughout levels
- **Screen effects** - Damage vignette, low health warning, head bob, weapon sway

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Left Click | Shoot |
| Space | Jump |
| Shift | Sprint |
| C / Ctrl | Toggle crouch |
| 1, 2, 3 | Switch weapons |
| Scroll Wheel | Cycle weapons |
| ESC | Pause |

## Building

### Prerequisites

- CMake 3.14+
- C++17 compiler (GCC, Clang, MSVC)
- raylib 5.0+ (installed system-wide or via CMake)

### Linux

```bash
# Install raylib (if not already installed)
git clone https://github.com/raysan5/raylib.git
cd raylib && mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=OFF .. && make -j$(nproc) && sudo make install
cd ../..

# Build the game
cd fps-game
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./DarkArena
```

### macOS

```bash
brew install raylib
cd fps-game && mkdir build && cd build
cmake .. && make -j$(sysctl -n hw.ncpu)
./DarkArena
```

### Windows (MSVC)

```cmd
cd fps-game
mkdir build && cd build
cmake ..
cmake --build . --config Release
Release\DarkArena.exe
```

## Architecture

```
fps-game/
├── include/          # Header files
│   ├── game.h        # Main game state and loop
│   ├── player.h      # FPS player controller
│   ├── level.h       # Procedural level generation
│   ├── enemy.h       # Enemy types and AI
│   ├── weapon.h      # Weapon system and raycasting
│   ├── particles.h   # Particle effects
│   ├── hud.h         # HUD rendering
│   ├── audio_manager.h  # Procedural audio
│   └── renderer.h    # Rendering utilities
├── src/              # Implementation files
├── CMakeLists.txt    # Build configuration
└── README.md
```

## Engine: raylib

This game uses [raylib](https://www.raylib.com/) - a simple and easy-to-use C library for game programming. raylib is open source (zlib/libpng license) and supports Windows, Linux, macOS, and more.

## License

MIT
