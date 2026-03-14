# DARK ARENA - Co-Op Zombie Survival FPS

A 3D first-person shooter with **online 2-player co-op** written in C++ using the **raylib** open-source game engine. Team up with a friend and survive 10 waves of zombies!

## Game Modes

- **Solo Survival** - Fight through 10 waves of zombies alone
- **Co-Op (Host)** - Host a game on your LAN/IP for a friend to join
- **Co-Op (Join)** - Connect to a friend's game by IP address

## Features

- **Online 2-player co-op** - TCP networking with player sync, shared enemies, and cooperative gameplay
- **Wave-based zombie survival** - 10 increasingly difficult waves with scaling enemy counts
- **4 Zombie types**:
  - **Walker** - Slow shamblers, low damage
  - **Runner** - Fast and aggressive, attacks in packs
  - **Brute** - Massive HP tank, hits hard
  - **Spitter** - Ranged acid attacks from distance
- **3 Weapons** - Pistol, Shotgun, and Assault Rifle
- **Procedurally generated levels** - Random rooms, corridors, and pillars with 4 visual themes
- **Full FPS controls** - WASD movement, mouse look, sprint, crouch, jump
- **Co-op HUD** - Partner health bar, kill counters, ping display, partner on minimap
- **Particle system** - Blood, sparks, muzzle flashes, pickup effects
- **Procedural textures and audio** - Zero external assets needed

## How Co-Op Works

1. **Player 1**: Select "HOST GAME" from the menu
2. **Player 2**: Select "JOIN GAME" and enter Player 1's IP address
3. Once connected, Player 1 presses ENTER to start
4. Both players spawn in the same procedurally generated level
5. Zombies target the closest player - work together to survive!
6. Game ends when both players die, or you survive all 10 waves

The host is authoritative for enemy AI and game state. Both players can shoot enemies independently with hit detection synced over the network.

## Controls

| Key | Action |
|-----|--------|
| W/S or UP/DOWN | Menu navigation |
| ENTER | Confirm / Start |
| WASD | Move |
| Mouse | Look |
| Left Click | Shoot |
| Space | Jump |
| Shift | Sprint |
| C / Ctrl | Toggle crouch |
| 1, 2, 3 | Switch weapons |
| Scroll Wheel | Cycle weapons |
| ESC | Pause / Back |

## Building

### Prerequisites

- CMake 3.14+
- C++17 compiler (GCC, Clang, MSVC)
- raylib 5.0+ (installed system-wide or via CMake)
- POSIX sockets (Linux/macOS - networking uses TCP sockets)

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

## Architecture

```
fps-game/
├── include/
│   ├── game.h          # Game state, co-op mode, wave system
│   ├── player.h        # FPS player controller
│   ├── level.h         # Procedural level generation
│   ├── enemy.h         # Zombie types and AI (targets closest player)
│   ├── weapon.h        # Weapon system and raycasting
│   ├── particles.h     # Particle effects
│   ├── hud.h           # HUD, lobby screen, co-op display
│   ├── network.h       # TCP networking, packet protocol
│   ├── audio_manager.h # Procedural audio
│   └── renderer.h      # Rendering utilities
├── src/                # Implementation files
├── CMakeLists.txt
└── README.md
```

## Networking

- **Protocol**: TCP with custom binary packet format
- **Architecture**: Host-authoritative (host runs enemy AI, validates hits)
- **Sync rate**: 30 Hz player state updates
- **Packet format**: `[type:1][size:2][data:N]`
- **Default port**: 7777

## Engine: raylib

This game uses [raylib](https://www.raylib.com/) - a simple and easy-to-use C library for game programming. raylib is open source (zlib/libpng license) and supports Windows, Linux, macOS, and more.

## License

MIT
