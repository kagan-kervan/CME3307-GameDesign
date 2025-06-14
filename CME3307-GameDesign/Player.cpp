// Player.cpp
#include "Player.h"
#include "GameEngine.h" // game_engine global değişkeni için
#include "resource.h"   // IDB_PLAYER_MISSILE gibi kaynak ID'leri için (varsayım)
#include <windows.h>    // GetAsyncKeyState için
#include <string>       // std::to_string (can gösterimi için)
#include <algorithm>    // std::min, std::max için
#include <cmath>        // std::sqrt, atan2, cos, sin için
#undef max           // Windows.h'daki max ile std::max çakışmasını önlemek için
#undef min
// Dışarıdan gelen global değişkenler
extern GameEngine* game_engine;
extern Bitmap* _pPlayerMissileBitmap; // Oyuncu mermi bitmap'i
extern RECT globalBounds; // Mermilerin hareket edeceği genel sınırlar
extern int TILE_SIZE;     // Global TILE_SIZE

// Kurucu Metot
Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap, SPRITE_TYPE_PLAYER),
    m_pMaze(pMaze)
{
    m_fSpeed = 250.0f; // Saniyede piksel cinsinden hız

    m_iFireCooldown = 0;
    m_iHealth = 100;
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0;
    m_bHasSecondWeapon = false;
    m_iSecondaryAmmo = 0;
    m_currentWeapon = WeaponType::PISTOL;

    m_iPistolAmmo = 100;
    m_iShotgunAmmo = 25;
    m_iSMGAmmo = 150;
}

SPRITEACTION Player::Update()
{
    float fDeltaTime = 0.0f;
    if (game_engine && game_engine->GetFrameDelay() > 0) {
        fDeltaTime = static_cast<float>(game_engine->GetFrameDelay()) / 1000.0f;
    }
    else {
        fDeltaTime = 1.0f / 30.0f; // Varsayılan 30 FPS
    }

    if (m_iFireCooldown > 0) {
        m_iFireCooldown--;
    }

    HandleInput(fDeltaTime);

    UpdateFrame();

    if (m_bDying || IsDead())
        return SA_KILL;

    return SA_NONE;
}

void Player::SwitchWeapon(WeaponType newWeapon)
{
    if (m_currentWeapon == newWeapon) {
        return;
    }
    // İkinci silah veya SMG için m_bHasSecondWeapon kontrolü eklenebilir
    // if (newWeapon == WeaponType::SHOTGUN && !m_bHasSecondWeapon) return;
    // if (newWeapon == WeaponType::SMG && !m_bHasSecondWeapon) return;

    m_iFireCooldown = 0;
    m_currentWeapon = newWeapon;
}

void Player::HandleInput(float fDeltaTime)
{
    // Silah Değiştirme
    if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
    if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::SHOTGUN);
    if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);

    // Hareket Hızı
    float currentSpeed = m_fSpeed;
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        currentSpeed *= SPRINT_SPEED_MULTIPLIER; // SPRINT_SPEED_MULTIPLIER Player.h'de tanımlı olmalı (örn: const float SPRINT_SPEED_MULTIPLIER = 1.5f;)
    }

    // Hareket Yönü
    float dirX = 0.0f;
    float dirY = 0.0f;
    if (GetAsyncKeyState('W') & 0x8000) dirY = -1.0f;
    if (GetAsyncKeyState('S') & 0x8000) dirY = 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) dirX = -1.0f;
    if (GetAsyncKeyState('D') & 0x8000) dirX = 1.0f;

    if (dirX != 0.0f || dirY != 0.0f)
    {
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) {
            dirX /= length;
            dirY /= length;
        }

        float moveAmountX = dirX * currentSpeed * fDeltaTime;
        float moveAmountY = dirY * currentSpeed * fDeltaTime;

        float newPosX_f = static_cast<float>(m_rcPosition.left) + moveAmountX;
        float newPosY_f = static_cast<float>(m_rcPosition.top) + moveAmountY;

        RECT rcNewPos = {
            static_cast<int>(newPosX_f),
            static_cast<int>(newPosY_f),
            static_cast<int>(newPosX_f + GetWidth()),
            static_cast<int>(newPosY_f + GetHeight())
        };

        bool collision = false;
        if (m_pMaze && TILE_SIZE > 0) {
            if (m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, rcNewPos.top / TILE_SIZE)) collision = true;
            if (!collision && m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, rcNewPos.top / TILE_SIZE)) collision = true;
            if (!collision && m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE)) collision = true;
            if (!collision && m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE)) collision = true;
        }

        if (!collision)
        {
            SetPosition(rcNewPos);
        }
    }
}

void Player::Fire(int targetX, int targetY)
{
    if (m_iFireCooldown > 0 || !_pPlayerMissileBitmap || !game_engine) return;

    switch (m_currentWeapon)
    {
    case WeaponType::PISTOL:
        if (m_iPistolAmmo <= 0) return;
        m_iPistolAmmo--;
        break;
    case WeaponType::SHOTGUN:
        if (m_iShotgunAmmo < 5) return; // 5 saçma atılıyor
        m_iShotgunAmmo -= 5;
        break;
    case WeaponType::SMG:
        if (m_iSMGAmmo <= 0) return;
        m_iSMGAmmo--;
        break;
    }


    POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
    float baseDirX = static_cast<float>(targetX - startPos.x);
    float baseDirY = static_cast<float>(targetY - startPos.y);
    float baseDistance = std::sqrt(baseDirX * baseDirX + baseDirY * baseDirY);

    if (baseDistance == 0) return;

    float normBaseX = baseDirX / baseDistance;
    float normBaseY = baseDirY / baseDistance;

    // DÜZELTME: MISSILE_SPEED_SPS doğrudan kullanılacak. Missile sınıfı fDeltaTime ile çarpacak.
    float missile_speed_factor = MISSILE_SPEED_SPS; // Saniyede piksel

    switch (m_currentWeapon)
    {
    case WeaponType::PISTOL:
    {
        m_iFireCooldown = PISTOL_COOLDOWN;
        float velocityX = normBaseX * missile_speed_factor;
        float velocityY = normBaseY * missile_speed_factor;

        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        game_engine->AddSprite(pMissile);
        break;
    }
    case WeaponType::SHOTGUN:
    {
        m_iFireCooldown = SHOTGUN_COOLDOWN;
        const int pelletCount = 5;
        const float spreadAngleDeg = 15.0f;

        for (int i = 0; i < pelletCount; ++i)
        {
            float randomAngleOffset = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngleDeg;
            float currentAngleRad = std::atan2(normBaseY, normBaseX) + randomAngleOffset * (3.14159265f / 180.0f);

            float dirX = std::cos(currentAngleRad);
            float dirY = std::sin(currentAngleRad);

            float velocityX = dirX * missile_speed_factor;
            float velocityY = dirY * missile_speed_factor;

            Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
            game_engine->AddSprite(pMissile);
        }
        break;
    }
    case WeaponType::SMG:
    {
        m_iFireCooldown = SMG_COOLDOWN;
        float randomAngleOffset = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 5.0f;
        float currentAngleRad = std::atan2(normBaseY, normBaseX) + randomAngleOffset * (3.14159265f / 180.0f);

        float dirX = std::cos(currentAngleRad);
        float dirY = std::sin(currentAngleRad);

        float velocityX = dirX * missile_speed_factor;
        float velocityY = dirY * missile_speed_factor;

        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        game_engine->AddSprite(pMissile);
        break;
    }
    }
}

void Player::TakeDamage(int amount)
{
    if (m_iHealth <= 0) return;

    int damageToHealth = amount;
    if (m_iArmor > 0)
    {
        int damageAbsorbedByArmor = std::min(m_iArmor, amount / 2);
        m_iArmor -= damageAbsorbedByArmor;
        damageToHealth -= damageAbsorbedByArmor;
    }

    m_iHealth -= damageToHealth;
    m_iHealth = std::max(0, m_iHealth);

    if (m_iHealth <= 0)
    {
        // OutputDebugString(L"PLAYER DIED!\n");
    }
}

void Player::AddKey(int amount) { m_iKeys += amount; }
int  Player::GetKeys() const { return m_iKeys; }

void Player::AddHealth(int amount) { m_iHealth = std::min(100, m_iHealth + amount); }
int  Player::GetHealth() const { return m_iHealth; }

void Player::AddArmor(int amount) { m_iArmor = std::min(100, m_iArmor + amount); }
int  Player::GetArmor() const { return m_iArmor; }

void Player::AddScore(int amount) { m_iScore += amount; }
int  Player::GetScore() const { return m_iScore; }

void Player::GiveSecondWeapon() { m_bHasSecondWeapon = true; }
bool Player::HasSecondWeapon() const { return m_bHasSecondWeapon; }

void Player::AddSecondaryAmmo(int amount) { if (m_bHasSecondWeapon) m_iSecondaryAmmo += amount; }
int  Player::GetSecondaryAmmo() const { return m_iSecondaryAmmo; }

int Player::GetPistolAmmo() const { return m_iPistolAmmo; }
int Player::GetShotgunAmmo() const { return m_iShotgunAmmo; }
int Player::GetSMGAmmo() const { return m_iSMGAmmo; }