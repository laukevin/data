#include "network.h"
#include "enemy.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>

void Network::Init() {
    role = NetRole::NONE;
    netState = NetState::DISCONNECTED;
    serverSocket = -1;
    clientSocket = -1;
    listenSocket = -1;
    remoteConnected = false;
    lastRecvTime = 0;
    sendTimer = 0;
    ping = 0;
    pingTimer = 0;
    recvBufLen = 0;
    inQueue.clear();
    outQueue.clear();
    memset(&remotePlayer, 0, sizeof(remotePlayer));
    strncpy(ipInputBuffer, "127.0.0.1", sizeof(ipInputBuffer));
    ipInputLen = 9;
}

void Network::Shutdown() {
    Disconnect();
}

void Network::SetNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

bool Network::StartHost() {
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) return false;

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(listenSocket);
        listenSocket = -1;
        return false;
    }

    if (listen(listenSocket, 1) < 0) {
        close(listenSocket);
        listenSocket = -1;
        return false;
    }

    SetNonBlocking(listenSocket);
    role = NetRole::HOST;
    netState = NetState::LISTENING;
    return true;
}

void Network::AcceptConnections() {
    if (listenSocket < 0 || clientSocket >= 0) return;

    struct sockaddr_in clientAddr = {};
    socklen_t addrLen = sizeof(clientAddr);
    int newSock = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
    if (newSock >= 0) {
        clientSocket = newSock;
        SetNonBlocking(clientSocket);

        // Disable Nagle for low latency
        int opt = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        // Send accept
        NetPacket pkt = {};
        pkt.type = NetMsg::CONNECT_ACCEPT;
        pkt.size = 0;
        SendPacket(pkt);

        remoteConnected = true;
        netState = NetState::CONNECTED;
        lastRecvTime = GetTime();
    }
}

bool Network::ConnectToHost(const char* ip) {
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) return false;

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    // Try non-blocking connect
    SetNonBlocking(clientSocket);

    int ret = connect(clientSocket, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        close(clientSocket);
        clientSocket = -1;
        return false;
    }

    // Disable Nagle
    int opt = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    strncpy(hostIP, ip, sizeof(hostIP));
    role = NetRole::CLIENT;
    netState = NetState::CONNECTING;
    lastRecvTime = GetTime();
    return true;
}

void Network::Update(float dt) {
    if (netState == NetState::DISCONNECTED) return;

    // Host: accept new connections
    if (role == NetRole::HOST && netState == NetState::LISTENING) {
        AcceptConnections();
    }

    // Client: check if connection completed
    if (role == NetRole::CLIENT && netState == NetState::CONNECTING) {
        // Check if socket is writable (connected)
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(clientSocket, &writefds);
        struct timeval tv = {0, 0};
        int ret = select(clientSocket + 1, nullptr, &writefds, nullptr, &tv);
        if (ret > 0) {
            int err = 0;
            socklen_t len = sizeof(err);
            getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err == 0) {
                netState = NetState::CONNECTED;
                remoteConnected = true;
            } else {
                Disconnect();
                return;
            }
        }
        // Timeout after 5 seconds
        if (GetTime() - lastRecvTime > 5.0f) {
            Disconnect();
            return;
        }
    }

    // Process incoming data
    int sock = clientSocket;
    if (sock >= 0) {
        ProcessRecv(sock);
    }

    // Ping
    pingTimer += dt;
    if (pingTimer > 1.0f && IsConnected()) {
        pingTimer = 0;
        pingSentTime = GetTime();
        NetPacket pkt = {};
        pkt.type = NetMsg::PING;
        pkt.size = 0;
        SendPacket(pkt);
    }

    // Timeout check
    if (IsConnected() && GetTime() - lastRecvTime > 10.0f) {
        Disconnect();
    }

    // Send rate limiter
    sendTimer += dt;
}

void Network::ProcessRecv(int sock) {
    // Read available data
    uint8_t tmpBuf[4096];
    int bytesRead = recv(sock, tmpBuf, sizeof(tmpBuf), 0);

    if (bytesRead > 0) {
        lastRecvTime = GetTime();

        // Append to recv buffer
        int copyLen = bytesRead;
        if (recvBufLen + copyLen > (int)sizeof(recvBuf)) {
            copyLen = sizeof(recvBuf) - recvBufLen;
        }
        memcpy(recvBuf + recvBufLen, tmpBuf, copyLen);
        recvBufLen += copyLen;

        // Parse packets from buffer
        // Packet format: [type:1][size:2][data:size]
        while (recvBufLen >= 3) {
            uint16_t pktSize;
            memcpy(&pktSize, recvBuf + 1, 2);

            int totalSize = 3 + pktSize;
            if (recvBufLen < totalSize) break;  // Incomplete packet

            NetPacket pkt = {};
            pkt.type = (NetMsg)recvBuf[0];
            pkt.size = pktSize;
            if (pktSize > 0 && pktSize <= NetPacket::MAX_SIZE) {
                memcpy(pkt.data, recvBuf + 3, pktSize);
            }

            // Handle ping/pong immediately
            if (pkt.type == NetMsg::PING) {
                NetPacket pong = {};
                pong.type = NetMsg::PONG;
                pong.size = 0;
                SendPacket(pong);
            } else if (pkt.type == NetMsg::PONG) {
                ping = (GetTime() - pingSentTime) * 1000.0f;
            } else {
                inQueue.push_back(pkt);
            }

            // Remove processed bytes
            memmove(recvBuf, recvBuf + totalSize, recvBufLen - totalSize);
            recvBufLen -= totalSize;
        }
    } else if (bytesRead == 0) {
        // Connection closed
        Disconnect();
    }
    // bytesRead < 0 with EAGAIN/EWOULDBLOCK is normal for non-blocking
}

void Network::SendPacket(const NetPacket& pkt) {
    if (clientSocket < 0) return;

    // Wire format: [type:1][size:2][data:size]
    uint8_t header[3];
    header[0] = (uint8_t)pkt.type;
    memcpy(header + 1, &pkt.size, 2);

    SendRaw(clientSocket, header, 3);
    if (pkt.size > 0) {
        SendRaw(clientSocket, pkt.data, pkt.size);
    }
}

bool Network::SendRaw(int sock, const void* data, int len) {
    int sent = 0;
    while (sent < len) {
        int ret = send(sock, (const char*)data + sent, len - sent, MSG_NOSIGNAL);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return false;
        }
        sent += ret;
    }
    return true;
}

int Network::RecvRaw(int sock, void* data, int maxLen) {
    return recv(sock, data, maxLen, 0);
}

void Network::SendPlayerState(const NetPlayerState& state) {
    NetPacket pkt = {};
    pkt.type = NetMsg::PLAYER_STATE;
    pkt.size = sizeof(NetPlayerState);
    pkt.WritePlayerState(0, state);
    SendPacket(pkt);
}

void Network::SendShoot(Vector3 origin, Vector3 dir, int weaponIdx) {
    NetPacket pkt = {};
    pkt.type = NetMsg::PLAYER_SHOOT;
    pkt.WriteFloat(0, origin.x);
    pkt.WriteFloat(4, origin.y);
    pkt.WriteFloat(8, origin.z);
    pkt.WriteFloat(12, dir.x);
    pkt.WriteFloat(16, dir.y);
    pkt.WriteFloat(20, dir.z);
    pkt.WriteInt(24, weaponIdx);
    pkt.size = 28;
    SendPacket(pkt);
}

void Network::SendEnemyStates(const std::vector<Enemy>& enemies) {
    // Send in chunks to fit packet size
    int count = (int)enemies.size();
    int perPkt = NetPacket::MAX_SIZE / (int)sizeof(NetEnemyState);

    for (int start = 0; start < count; start += perPkt) {
        int end = start + perPkt;
        if (end > count) end = count;

        NetPacket pkt = {};
        pkt.type = NetMsg::ENEMY_STATES;
        int offset = 0;
        pkt.WriteShort(offset, (int16_t)(end - start)); offset += 2;
        pkt.WriteShort(offset, (int16_t)start); offset += 2;

        for (int i = start; i < end; i++) {
            NetEnemyState nes;
            nes.id = (uint16_t)i;
            nes.position = enemies[i].position;
            nes.yaw = enemies[i].yaw;
            nes.health = (int16_t)enemies[i].health;
            nes.state = (uint8_t)enemies[i].state;
            nes.type = (uint8_t)enemies[i].type;
            nes.active = enemies[i].active;
            memcpy(pkt.data + offset, &nes, sizeof(NetEnemyState));
            offset += sizeof(NetEnemyState);
        }
        pkt.size = (uint16_t)offset;
        SendPacket(pkt);
    }
}

void Network::SendEnemyDamage(int enemyIdx, int damage, Vector3 hitDir) {
    NetPacket pkt = {};
    pkt.type = NetMsg::ENEMY_DAMAGED;
    pkt.WriteInt(0, enemyIdx);
    pkt.WriteInt(4, damage);
    pkt.WriteFloat(8, hitDir.x);
    pkt.WriteFloat(12, hitDir.y);
    pkt.WriteFloat(16, hitDir.z);
    pkt.size = 20;
    SendPacket(pkt);
}

void Network::SendEnemyKill(int enemyIdx, int scorerPlayerId) {
    NetPacket pkt = {};
    pkt.type = NetMsg::ENEMY_KILLED;
    pkt.WriteInt(0, enemyIdx);
    pkt.WriteInt(4, scorerPlayerId);
    pkt.size = 8;
    SendPacket(pkt);
}

void Network::SendPickupCollected(int pickupIdx) {
    NetPacket pkt = {};
    pkt.type = NetMsg::PICKUP_UPDATE;
    pkt.WriteInt(0, pickupIdx);
    pkt.size = 4;
    SendPacket(pkt);
}

void Network::SendPlayerDamaged(int playerId, int amount) {
    NetPacket pkt = {};
    pkt.type = NetMsg::PLAYER_DAMAGED;
    pkt.WriteInt(0, playerId);
    pkt.WriteInt(4, amount);
    pkt.size = 8;
    SendPacket(pkt);
}

void Network::SendGameStart(int levelSeed, int levelIdx) {
    NetPacket pkt = {};
    pkt.type = NetMsg::GAME_START;
    pkt.WriteInt(0, levelSeed);
    pkt.WriteInt(4, levelIdx);
    pkt.size = 8;
    SendPacket(pkt);
}

void Network::SendWaveStart(int waveNum, int enemyCount) {
    NetPacket pkt = {};
    pkt.type = NetMsg::WAVE_START;
    pkt.WriteInt(0, waveNum);
    pkt.WriteInt(4, enemyCount);
    pkt.size = 8;
    SendPacket(pkt);
}

void Network::SendLevelComplete() {
    NetPacket pkt = {};
    pkt.type = NetMsg::LEVEL_COMPLETE;
    pkt.size = 0;
    SendPacket(pkt);
}

void Network::SendGameOver() {
    NetPacket pkt = {};
    pkt.type = NetMsg::GAME_OVER;
    pkt.size = 0;
    SendPacket(pkt);
}

void Network::SendGameWin() {
    NetPacket pkt = {};
    pkt.type = NetMsg::GAME_WIN;
    pkt.size = 0;
    SendPacket(pkt);
}

bool Network::HasPendingPackets() {
    return !inQueue.empty();
}

NetPacket Network::PopPacket() {
    NetPacket pkt = inQueue.front();
    inQueue.erase(inQueue.begin());
    return pkt;
}

void Network::Disconnect() {
    if (clientSocket >= 0) {
        close(clientSocket);
        clientSocket = -1;
    }
    if (listenSocket >= 0) {
        close(listenSocket);
        listenSocket = -1;
    }
    remoteConnected = false;
    netState = NetState::DISCONNECTED;
    recvBufLen = 0;
    inQueue.clear();
}

bool Network::IsConnected() const {
    return (netState == NetState::CONNECTED || netState == NetState::IN_GAME) && remoteConnected;
}
