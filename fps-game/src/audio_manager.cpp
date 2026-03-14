#include "audio_manager.h"
#include <cmath>
#include <cstdlib>

Wave AudioManager::GenSineWave(float freq, float duration, float volume) {
    int sampleRate = 44100;
    int sampleCount = (int)(sampleRate * duration);

    Wave wave = {0};
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = malloc(sampleCount * sizeof(short));

    short* samples = (short*)wave.data;
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float envelope = 1.0f - (t / duration);  // Linear decay
        envelope = envelope * envelope;  // Exponential decay
        float value = sinf(2.0f * PI * freq * t) * volume * envelope;
        samples[i] = (short)(value * 32000.0f);
    }
    return wave;
}

Wave AudioManager::GenNoiseWave(float duration, float volume) {
    int sampleRate = 44100;
    int sampleCount = (int)(sampleRate * duration);

    Wave wave = {0};
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = malloc(sampleCount * sizeof(short));

    short* samples = (short*)wave.data;
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float envelope = 1.0f - (t / duration);
        envelope = envelope * envelope;
        float value = ((float)(rand() % 2000 - 1000) / 1000.0f) * volume * envelope;
        samples[i] = (short)(value * 32000.0f);
    }
    return wave;
}

Wave AudioManager::GenSweepWave(float startFreq, float endFreq, float duration, float volume) {
    int sampleRate = 44100;
    int sampleCount = (int)(sampleRate * duration);

    Wave wave = {0};
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = malloc(sampleCount * sizeof(short));

    short* samples = (short*)wave.data;
    float phase = 0;
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float frac = t / duration;
        float freq = startFreq + (endFreq - startFreq) * frac;
        float envelope = 1.0f - frac;
        envelope = envelope * envelope;
        phase += 2.0f * PI * freq / sampleRate;
        float value = sinf(phase) * volume * envelope;
        samples[i] = (short)(value * 32000.0f);
    }
    return wave;
}

void AudioManager::Init() {
    // Generate procedural sound effects
    Wave wShoot = GenSweepWave(800, 200, 0.15f, 0.5f);
    sndShoot = LoadSoundFromWave(wShoot);
    UnloadWave(wShoot);

    Wave wShotgun = GenNoiseWave(0.2f, 0.7f);
    sndShotgun = LoadSoundFromWave(wShotgun);
    UnloadWave(wShotgun);

    Wave wRifle = GenSweepWave(1200, 400, 0.08f, 0.4f);
    sndRifle = LoadSoundFromWave(wRifle);
    UnloadWave(wRifle);

    Wave wHit = GenSineWave(300, 0.1f, 0.4f);
    sndHit = LoadSoundFromWave(wHit);
    UnloadWave(wHit);

    Wave wPickup = GenSweepWave(400, 800, 0.2f, 0.3f);
    sndPickup = LoadSoundFromWave(wPickup);
    UnloadWave(wPickup);

    Wave wDamage = GenSweepWave(500, 100, 0.15f, 0.5f);
    sndDamage = LoadSoundFromWave(wDamage);
    UnloadWave(wDamage);

    Wave wExplosion = GenNoiseWave(0.5f, 0.8f);
    sndExplosion = LoadSoundFromWave(wExplosion);
    UnloadWave(wExplosion);

    Wave wDoor = GenSweepWave(100, 200, 0.3f, 0.3f);
    sndDoor = LoadSoundFromWave(wDoor);
    UnloadWave(wDoor);

    Wave wFootstep = GenNoiseWave(0.05f, 0.15f);
    sndFootstep = LoadSoundFromWave(wFootstep);
    UnloadWave(wFootstep);
}

void AudioManager::Update() {
    // Could update ambient music here
}

void AudioManager::PlayShoot(int weaponType) {
    switch (weaponType) {
        case 0: PlaySound(sndShoot); break;
        case 1: PlaySound(sndShotgun); break;
        case 2: PlaySound(sndRifle); break;
        default: PlaySound(sndShoot); break;
    }
}

void AudioManager::PlayHit() { PlaySound(sndHit); }
void AudioManager::PlayPickup() { PlaySound(sndPickup); }
void AudioManager::PlayDamage() { PlaySound(sndDamage); }
void AudioManager::PlayExplosion() { PlaySound(sndExplosion); }
void AudioManager::PlayDoor() { PlaySound(sndDoor); }
void AudioManager::PlayFootstep() {
    if (!IsSoundPlaying(sndFootstep)) PlaySound(sndFootstep);
}

void AudioManager::Shutdown() {
    UnloadSound(sndShoot);
    UnloadSound(sndShotgun);
    UnloadSound(sndRifle);
    UnloadSound(sndHit);
    UnloadSound(sndPickup);
    UnloadSound(sndDamage);
    UnloadSound(sndExplosion);
    UnloadSound(sndDoor);
    UnloadSound(sndFootstep);
}
