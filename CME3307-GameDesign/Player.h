// Player.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"
#include <cmath>
#include "Missile.h"
#include <map> // Harita veri yapısı için (silah istatistikleri)

// Silah tipleri
enum class WeaponType {
    PISTOL,
    SHOTGUN,
    SMG
};

// Her silahın kendine özgü özelliklerini tutacak veri yapısı
struct WeaponStats {
    int clipSize;           // Şarjör kapasitesi
    int currentAmmoInClip;  // Şarjördeki mevcut mermi sayısı
    int totalAmmo;          // Toplam taşınan mermi (yedek). -1 sonsuz demektir.
    int fireCooldown;       // Ateş etme hızı (frame cinsinden)
    int reloadTime;         // Yeniden doldurma süresi (frame cinsinden)
};

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);
    virtual SPRITEACTION Update();
    void Fire(int targetX, int targetY);

    // --- Public Metotlar ---
    // UI'da (kullanıcı arayüzü) göstermek için public getter'lar
    const WeaponStats& GetCurrentWeaponStats() const;
    WeaponType GetCurrentWeaponType() const;
    bool IsReloading() const;

    // Oyuncu istatistikleri için metotlar
    void AddKey(int amount = 1);
    int  GetKeys() const;
    void ResetKeys();

    void AddHealth(int amount);
    int  GetHealth() const;

    void AddArmor(int amount);
    int  GetArmor() const;

    void AddScore(int amount);
    int  GetScore() const;

    // Not: Bu iki metot artık doğrudan kullanılmıyor, m_weaponStats içinde yönetiliyor.
    // İhtiyaç halinde bırakılabilir veya kaldırılabilir.
    void GiveSecondWeapon();
    bool HasSecondWeapon() const;
    void AddSecondaryAmmo(int amount);
    int  GetSecondaryAmmo() const;

    // Oyunu yeniden başlatmak için tüm değerleri sıfırlar
    void Reset();

    // Hasar alma mekanizması
    void TakeDamage(int amount);
    bool IsDead() const { return m_iHealth <= 0; }

private:
    // --- Private Metotlar ---
    void HandleInput(float fDeltaTime);
    void SwitchWeapon(WeaponType newWeapon);
    void StartReload(); // Yeniden doldurma işlemini başlatır

    // --- Üye Değişkenleri ---
    MazeGenerator* m_pMaze;

    // Hareket
    float m_fSpeed;
    // const int'i const float veya constexpr float yapmak daha doğru olur.
    static constexpr float SPRINT_SPEED_MULTIPLIER = 1.5f;

    // Silah ve Mermi Sistemi
    WeaponType m_currentWeapon;
    std::map<WeaponType, WeaponStats> m_weaponStats; // Her silahın istatistiklerini tutar
    int m_iFireCooldown;  // Ateş etme bekleme sayacı
    int m_iReloadTimer;   // Yeniden doldurma sayacı

    // Mermi hızı (tüm silahlar için ortak)
    static const int MISSILE_SPEED_SPS = 2000;

    // Oyuncu İstatistikleri
    int m_iKeys;
    int m_iHealth;
    int m_iArmor;
    int m_iScore;

    // Bu değişkenler artık WeaponStats içinde yönetildiği için kaldırılabilir
    // veya özel bir mantık için tutulabilir. Şimdilik bırakıyorum.
    int m_iSecondaryAmmo;
    bool m_bHasSecondWeapon;
};