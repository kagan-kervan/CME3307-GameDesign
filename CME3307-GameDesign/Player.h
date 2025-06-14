// Player.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"
#include <cmath>

// YENÝ: Silah tiplerini tanýmlýyoruz
enum class WeaponType {
    PISTOL,
    SHOTGUN,
    SMG // Submachine Gun (Hýzlý Taramalý)
};

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);
    virtual SPRITEACTION Update();
    void Fire(int targetX, int targetY);

    // Yeni eklenen public metodlar
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


private:
    void HandleInput(float fDeltaTime);
    void SwitchWeapon(WeaponType newWeapon); // Silah deðiþtiren yeni private fonksiyon

    MazeGenerator* m_pMaze;
    float m_fSpeed;
    static const int SPRINT_SPEED_MULTIPLIER = 2;

    // --- SÝLAH SÝSTEMÝ DEÐÝÞKENLERÝ ---
    WeaponType m_currentWeapon; // Oyuncunun mevcut silahý
    int m_iFireCooldown;        // Ateþ etme bekleme süresi sayacý

    static const int PISTOL_COOLDOWN = 6;  // 0.2 saniyede bir ateþ eder
    static const int SHOTGUN_COOLDOWN = 12; // ~0.6 saniyede bir ateþ eder
    static const int SMG_COOLDOWN = 1;      // ~0.06 saniyede bir (çok çok hýzlý)

    // Mermi hýzý (sabit kalabilir veya silaha göre deðiþebilir)
    static const int MISSILE_SPEED_SPS = 2000; // Saniyede Piksel

    MazeGenerator* m_pMaze; // Çarpýþma kontrolü için labirent referansý
    int m_iSpeed;

    // Yeni eklenen oyuncu özellikleri
    int m_iKeys;
    int m_iHealth;
    int m_iArmor;
    int m_iScore;
    int m_iSecondaryAmmo;
    bool m_bHasSecondWeapon;

};