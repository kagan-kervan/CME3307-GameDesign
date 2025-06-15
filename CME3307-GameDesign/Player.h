#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"
#include <cmath>
#include "Missile.h"
#include <map>

enum class WeaponType {
    PISTOL,
    SHOTGUN,
    SMG
};

struct WeaponStats {
    int clipSize;
    int currentAmmoInClip;
    int totalAmmo;
    int fireCooldown;
    int reloadTime;
};

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);
    virtual SPRITEACTION Update();
    void Fire(int targetX, int targetY);

    const WeaponStats& GetCurrentWeaponStats() const;
    WeaponType GetCurrentWeaponType() const;
    bool IsReloading() const;

    void AddKey(int amount = 1);
    int  GetKeys() const;
    void ResetKeys();

    void AddHealth(int amount);
    int  GetHealth() const;

    void AddArmor(int amount);
    int  GetArmor() const;

    void AddScore(int amount);
    int  GetScore() const;

    float GetStamina() const;
    float GetMaxStamina() const;

    void GiveSecondWeapon();
    bool HasSecondWeapon() const;
    void AddSecondaryAmmo(int amount);
    int  GetSecondaryAmmo() const;

    void Reset();

    void TakeDamage(int amount);
    bool IsDead() const { return m_iHealth <= 0; }

private:
    void HandleInput(float fDeltaTime);
    void SwitchWeapon(WeaponType newWeapon);
    void StartReload();

    MazeGenerator* m_pMaze;

    float m_fSpeed;
    static constexpr float SPRINT_SPEED_MULTIPLIER = 1.5f;

    float m_fStamina;
    float m_fMaxStamina;
    bool  m_bIsSprinting;
    // DEĞİŞİKLİK: 'const' yerine 'constexpr' kullanıyoruz.
    static constexpr float STAMINA_DEPLETE_RATE = 30.0f;
    static constexpr float STAMINA_REGEN_RATE = 20.0f;
    static constexpr float STAMINA_REGEN_DELAY = 1.5f;

    float m_fStaminaRegenTimer;


    WeaponType m_currentWeapon;
    std::map<WeaponType, WeaponStats> m_weaponStats;
    int m_iFireCooldown;
    int m_iReloadTimer;
    static const int MISSILE_SPEED_SPS = 2000;

    int m_iKeys;
    int m_iHealth;
    int m_iArmor;
    int m_iScore;
    int m_iSecondaryAmmo;
    bool m_bHasSecondWeapon;
};