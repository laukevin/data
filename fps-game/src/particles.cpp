#include "particles.h"
#include "rlgl.h"
#include <cmath>

void ParticleSystem::Init() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
    nextParticle = 0;
}

Particle& ParticleSystem::GetNext() {
    Particle& p = particles[nextParticle];
    nextParticle = (nextParticle + 1) % MAX_PARTICLES;
    return p;
}

void ParticleSystem::Update(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = particles[i];
        if (!p.active) continue;

        p.lifetime -= dt;
        if (p.lifetime <= 0) {
            p.active = false;
            continue;
        }

        // Physics
        p.velocity.y -= 8.0f * dt;  // gravity
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.size -= p.sizeDecay * dt;
        if (p.size < 0) p.size = 0;

        // Floor bounce
        if (p.position.y < 0.05f) {
            p.position.y = 0.05f;
            p.velocity.y *= -0.3f;
            p.velocity.x *= 0.8f;
            p.velocity.z *= 0.8f;
        }

        // Fade
        float life = p.lifetime / p.maxLifetime;
        p.color.a = (unsigned char)(life * 255);
    }
}

void ParticleSystem::Draw(Camera3D camera) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = particles[i];
        if (!p.active || p.size <= 0) continue;

        // Billboard particle
        DrawSphere(p.position, p.size, p.color);
    }
}

void ParticleSystem::EmitBlood(Vector3 pos, Vector3 dir, int count) {
    for (int i = 0; i < count; i++) {
        Particle& p = GetNext();
        p.active = true;
        p.position = pos;
        p.velocity = {
            dir.x * 3.0f + (GetRandomValue(-100, 100) / 100.0f) * 2.0f,
            dir.y * 2.0f + (GetRandomValue(0, 100) / 100.0f) * 3.0f,
            dir.z * 3.0f + (GetRandomValue(-100, 100) / 100.0f) * 2.0f
        };
        int shade = GetRandomValue(100, 200);
        p.color = {(unsigned char)shade, 0, 0, 255};
        p.size = 0.03f + GetRandomValue(0, 30) / 1000.0f;
        p.sizeDecay = 0.02f;
        p.lifetime = p.maxLifetime = 0.5f + GetRandomValue(0, 100) / 100.0f;
    }
}

void ParticleSystem::EmitSpark(Vector3 pos, Vector3 normal, int count) {
    for (int i = 0; i < count; i++) {
        Particle& p = GetNext();
        p.active = true;
        p.position = pos;
        p.velocity = {
            normal.x * 2.0f + (GetRandomValue(-100, 100) / 100.0f) * 3.0f,
            normal.y * 2.0f + (GetRandomValue(50, 200) / 100.0f) * 2.0f,
            normal.z * 2.0f + (GetRandomValue(-100, 100) / 100.0f) * 3.0f
        };
        p.color = {255, (unsigned char)GetRandomValue(150, 255), 0, 255};
        p.size = 0.02f + GetRandomValue(0, 20) / 1000.0f;
        p.sizeDecay = 0.03f;
        p.lifetime = p.maxLifetime = 0.3f + GetRandomValue(0, 50) / 100.0f;
    }
}

void ParticleSystem::EmitMuzzleFlash(Vector3 pos, Vector3 dir) {
    for (int i = 0; i < 3; i++) {
        Particle& p = GetNext();
        p.active = true;
        p.position = pos;
        p.velocity = {
            dir.x * 5.0f + (GetRandomValue(-50, 50) / 100.0f),
            dir.y * 5.0f + (GetRandomValue(-50, 50) / 100.0f),
            dir.z * 5.0f + (GetRandomValue(-50, 50) / 100.0f)
        };
        p.color = {255, 255, 200, 255};
        p.size = 0.05f;
        p.sizeDecay = 0.15f;
        p.lifetime = p.maxLifetime = 0.1f;
    }
}

void ParticleSystem::EmitExplosion(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle& p = GetNext();
        p.active = true;
        p.position = pos;
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float upVel = GetRandomValue(0, 100) / 100.0f * 8.0f;
        float hVel = GetRandomValue(50, 300) / 100.0f * 4.0f;
        p.velocity = {cosf(angle) * hVel, upVel, sinf(angle) * hVel};

        int type = GetRandomValue(0, 2);
        if (type == 0) p.color = {255, (unsigned char)GetRandomValue(100, 200), 0, 255};
        else if (type == 1) p.color = {255, (unsigned char)GetRandomValue(50, 100), 0, 255};
        else p.color = {100, 100, 100, 200};

        p.size = 0.05f + GetRandomValue(0, 50) / 1000.0f;
        p.sizeDecay = 0.02f;
        p.lifetime = p.maxLifetime = 0.5f + GetRandomValue(0, 100) / 100.0f;
    }
}

void ParticleSystem::EmitSmoke(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle& p = GetNext();
        p.active = true;
        p.position = pos;
        p.velocity = {
            (GetRandomValue(-50, 50) / 100.0f),
            GetRandomValue(50, 200) / 100.0f,
            (GetRandomValue(-50, 50) / 100.0f)
        };
        int gray = GetRandomValue(80, 150);
        p.color = {(unsigned char)gray, (unsigned char)gray, (unsigned char)gray, 150};
        p.size = 0.08f;
        p.sizeDecay = 0.01f;
        p.lifetime = p.maxLifetime = 1.0f + GetRandomValue(0, 100) / 100.0f;
    }
}

void ParticleSystem::EmitPickup(Vector3 pos, Color color) {
    for (int i = 0; i < 15; i++) {
        Particle& p = GetNext();
        p.active = true;
        p.position = pos;
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(100, 300) / 100.0f;
        p.velocity = {cosf(angle) * speed, GetRandomValue(200, 500) / 100.0f, sinf(angle) * speed};
        p.color = color;
        p.size = 0.04f;
        p.sizeDecay = 0.02f;
        p.lifetime = p.maxLifetime = 0.5f;
    }
}

void ParticleSystem::EmitTrail(Vector3 pos, Color color) {
    Particle& p = GetNext();
    p.active = true;
    p.position = pos;
    p.velocity = {(GetRandomValue(-10, 10) / 100.0f), 0.1f, (GetRandomValue(-10, 10) / 100.0f)};
    p.color = color;
    p.color.a = 150;
    p.size = 0.02f;
    p.sizeDecay = 0.03f;
    p.lifetime = p.maxLifetime = 0.2f;
}
