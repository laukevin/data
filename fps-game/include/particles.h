#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float lifetime;
    float maxLifetime;
    float size;
    float sizeDecay;
    bool active = false;
};

struct ParticleSystem {
    static const int MAX_PARTICLES = 2000;
    Particle particles[MAX_PARTICLES];
    int nextParticle = 0;

    void Init();
    void Update(float dt);
    void Draw(Camera3D camera);

    // Emitters
    void EmitBlood(Vector3 pos, Vector3 dir, int count);
    void EmitSpark(Vector3 pos, Vector3 normal, int count);
    void EmitMuzzleFlash(Vector3 pos, Vector3 dir);
    void EmitExplosion(Vector3 pos, int count);
    void EmitSmoke(Vector3 pos, int count);
    void EmitPickup(Vector3 pos, Color color);
    void EmitTrail(Vector3 pos, Color color);

private:
    Particle& GetNext();
};
