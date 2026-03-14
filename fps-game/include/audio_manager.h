#pragma once
#include "raylib.h"

struct AudioManager {
    // We generate simple synth sounds since we don't have audio files
    Sound sndShoot;
    Sound sndShotgun;
    Sound sndRifle;
    Sound sndHit;
    Sound sndPickup;
    Sound sndDamage;
    Sound sndExplosion;
    Sound sndDoor;
    Sound sndFootstep;
    Music ambience;

    bool musicPlaying = false;

    void Init();
    void Update();
    void PlayShoot(int weaponType);
    void PlayHit();
    void PlayPickup();
    void PlayDamage();
    void PlayExplosion();
    void PlayDoor();
    void PlayFootstep();
    void Shutdown();

private:
    Wave GenSineWave(float freq, float duration, float volume);
    Wave GenNoiseWave(float duration, float volume);
    Wave GenSweepWave(float startFreq, float endFreq, float duration, float volume);
};
