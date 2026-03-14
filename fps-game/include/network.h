#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

// Network message types
enum class NetMsg : uint8_t {
    // Connection
    CONNECT_REQUEST = 1,
    CONNECT_ACCEPT,
    CONNECT_REJECT,
    DISCONNECT,
    PING,
    PONG,

    // Game state (server -> client)
    GAME_START,
    LEVEL_SEED,
    ENEMY_STATES,
    PICKUP_UPDATE,
    PLAYER_DAMAGED,
    ENEMY_DAMAGED,
    ENEMY_KILLED,
    LEVEL_COMPLETE,
    GAME_OVER,
    GAME_WIN,
    WAVE_START,

    // Player state (bidirectional)
    PLAYER_STATE,
    PLAYER_SHOOT,
    PLAYER_WEAPON_SWITCH,

    // Chat
    CHAT_MESSAGE,
};

// Compact player state for network sync
struct NetPlayerState {
    Vector3 position;
    float yaw;
    float pitch;
    int health;
    int armor;
    int currentWeapon;
    bool isSprinting;
    bool isCrouching;
    bool isShooting;
};

// Compact enemy state for network sync
struct NetEnemyState {
    uint16_t id;
    Vector3 position;
    float yaw;
    int16_t health;
    uint8_t state;  // EnemyState
    uint8_t type;   // EnemyType
    bool active;
};

// Network packet header
struct NetPacket {
    static const int MAX_SIZE = 4096;
    NetMsg type;
    uint16_t size;
    uint8_t data[MAX_SIZE];

    void WriteFloat(int offset, float v) { memcpy(data + offset, &v, 4); }
    float ReadFloat(int offset) const { float v; memcpy(&v, data + offset, 4); return v; }
    void WriteInt(int offset, int32_t v) { memcpy(data + offset, &v, 4); }
    int32_t ReadInt(int offset) const { int32_t v; memcpy(&v, data + offset, 4); return v; }
    void WriteShort(int offset, int16_t v) { memcpy(data + offset, &v, 2); }
    int16_t ReadShort(int offset) const { int16_t v; memcpy(&v, data + offset, 2); return v; }
    void WriteByte(int offset, uint8_t v) { data[offset] = v; }
    uint8_t ReadByte(int offset) const { return data[offset]; }

    void WritePlayerState(int offset, const NetPlayerState& ps) {
        memcpy(data + offset, &ps, sizeof(NetPlayerState));
    }
    NetPlayerState ReadPlayerState(int offset) const {
        NetPlayerState ps;
        memcpy(&ps, data + offset, sizeof(NetPlayerState));
        return ps;
    }
};

enum class NetRole {
    NONE,
    HOST,
    CLIENT
};

enum class NetState {
    DISCONNECTED,
    LISTENING,      // Host waiting for client
    CONNECTING,     // Client trying to connect
    CONNECTED,      // Both players in lobby
    IN_GAME         // Playing
};

struct Network {
    NetRole role = NetRole::NONE;
    NetState netState = NetState::DISCONNECTED;

    int serverSocket = -1;
    int clientSocket = -1;  // On host: the accepted client. On client: connection to host
    int listenSocket = -1;

    uint16_t port = 7777;
    char hostIP[64] = "127.0.0.1";
    char ipInputBuffer[64] = "127.0.0.1";
    int ipInputLen = 9;

    // Remote player state
    NetPlayerState remotePlayer = {};
    bool remoteConnected = false;
    float lastRecvTime = 0.0f;
    float sendTimer = 0.0f;
    float sendRate = 1.0f / 30.0f;  // 30 Hz

    // Stats
    float ping = 0.0f;
    float pingTimer = 0.0f;
    float pingSentTime = 0.0f;

    // Packet queue
    std::vector<NetPacket> inQueue;
    std::vector<NetPacket> outQueue;

    void Init();
    void Shutdown();

    // Host
    bool StartHost();
    void UpdateHost();
    void AcceptConnections();

    // Client
    bool ConnectToHost(const char* ip);
    void UpdateClient();

    // Common
    void Update(float dt);
    void SendPacket(const NetPacket& pkt);
    void SendPlayerState(const NetPlayerState& state);
    void SendShoot(Vector3 origin, Vector3 dir, int weaponIdx);
    void SendEnemyStates(const struct std::vector<struct Enemy>& enemies);
    void SendEnemyDamage(int enemyIdx, int damage, Vector3 hitDir);
    void SendEnemyKill(int enemyIdx, int scorerPlayerId);
    void SendPickupCollected(int pickupIdx);
    void SendPlayerDamaged(int playerId, int amount);
    void SendGameStart(int levelSeed, int levelIdx);
    void SendWaveStart(int waveNum, int enemyCount);
    void SendLevelComplete();
    void SendGameOver();
    void SendGameWin();
    bool HasPendingPackets();
    NetPacket PopPacket();
    void Disconnect();
    bool IsConnected() const;
    bool IsHost() const { return role == NetRole::HOST; }

private:
    bool SendRaw(int sock, const void* data, int len);
    int RecvRaw(int sock, void* data, int maxLen);
    void ProcessRecv(int sock);
    void SetNonBlocking(int sock);

    // Recv buffer for handling partial reads
    uint8_t recvBuf[8192];
    int recvBufLen = 0;
};
