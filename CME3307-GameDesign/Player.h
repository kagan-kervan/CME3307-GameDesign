#pragma once
// Player.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"
#include <cmath>

// YEN�: Silah tiplerini tan�ml�yoruz
enum class WeaponType {
    PISTOL,
    SHOTGUN,
    SMG // Submachine Gun (H�zl� Taramal�)
};

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);
    int TILE_SIZE = 50;
    virtual SPRITEACTION Update();
    void Fire(int targetX, int targetY);

private:
    void HandleInput(float fDeltaTime);
    void SwitchWeapon(WeaponType newWeapon); // Silah de�i�tiren yeni private fonksiyon

    MazeGenerator* m_pMaze;
    float m_fSpeed;
    static const int SPRINT_SPEED_MULTIPLIER = 2;

    // --- S�LAH S�STEM� DE���KENLER� ---
    WeaponType m_currentWeapon; // Oyuncunun mevcut silah�
    int m_iFireCooldown;        // Ate� etme bekleme s�resi sayac�

    static const int PISTOL_COOLDOWN = 6;  // 0.2 saniyede bir ate� eder
    static const int SHOTGUN_COOLDOWN = 12; // ~0.6 saniyede bir ate� eder
    static const int SMG_COOLDOWN = 1;      // ~0.06 saniyede bir (�ok �ok h�zl�)

    // Mermi h�z� (sabit kalabilir veya silaha g�re de�i�ebilir)
    static const int MISSILE_SPEED_SPS = 2000; // Saniyede Piksel
};