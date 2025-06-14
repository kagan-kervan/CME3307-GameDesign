// Player.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"
#include <cmath> // std::sqrt için
#include "Missile.h" // Missile sınıfı için

// Silah tipleri
enum class WeaponType {
    PISTOL,
    SHOTGUN,
    SMG
};

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);
    // TILE_SIZE Player sınıfı içinde tanımlı olmamalı, global veya Game.h'den gelmeli.
    // Şimdilik buradan kaldırıyorum, Game.cpp'deki global TILE_SIZE kullanılacak.
    // int TILE_SIZE = 50;

    virtual SPRITEACTION Update();
    void Fire(int targetX, int targetY);
    void HandleInput(float fDeltaTime);
    void SwitchWeapon(WeaponType newWeapon);

    void AddKey(int amount = 1);
    int  GetKeys() const;

    void AddHealth(int amount);
    int  GetHealth() const;

    void AddArmor(int amount);
    int  GetArmor() const;

    void AddScore(int amount);
    int  GetScore() const;

    void GiveSecondWeapon();
    bool HasSecondWeapon() const;

    void AddSecondaryAmmo(int amount);
    int  GetSecondaryAmmo() const;

    // YENİ: Hasar alma fonksiyonu
    void TakeDamage(int amount);
    bool IsDead() const { return m_iHealth <= 0; } // Oyuncunun ölüp ölmediğini kontrol et

    int GetPistolAmmo() const;
    int GetShotgunAmmo() const;
    int GetSMGAmmo() const;

    void AddPistolAmmo(int amount);
    void AddShotgunAmmo(int amount);
    void AddSMGAmmo(int amount);

private:
    MazeGenerator* m_pMaze;
    float m_fSpeed;
    static const int SPRINT_SPEED_MULTIPLIER = 2; // const float olmalı

    WeaponType m_currentWeapon;
    int m_iFireCooldown;

    static const int PISTOL_COOLDOWN = 6;
    static const int SHOTGUN_COOLDOWN = 12;
    static const int SMG_COOLDOWN = 1;

    static const int MISSILE_SPEED_SPS = 2000;

    int m_iKeys;
    int m_iHealth;
    int m_iArmor;
    int m_iScore;
    int m_iSecondaryAmmo;
    bool m_bHasSecondWeapon;
    int m_iPistolAmmo;
    int m_iShotgunAmmo;
    int m_iSMGAmmo;
};