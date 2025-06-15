// Player.cpp
#include "Player.h"
#include "GameEngine.h"
#include "resource.h"
#include <windows.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <random>

#undef max
#undef min

// Dışarıdan gelen global değişkenler
extern GameEngine* game_engine;
extern Bitmap* _pPlayerMissileBitmap;
extern RECT globalBounds;
extern int TILE_SIZE;

// Kurucu Metot: Oyuncunun tüm başlangıç durumunu ayarlar
Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap, SPRITE_TYPE_PLAYER), m_pMaze(pMaze)
{
    // Oyuncunun tüm değerlerini başlangıç durumuna getirmek için Reset'i çağır
    Reset();
}

// Oyuncunun durumunu sıfırlayan metot (oyun başlangıcı ve yeniden başlatma için)
void Player::Reset()
{
    m_fSpeed = 250.0f;
    m_iFireCooldown = 0;
    m_iReloadTimer = 0;
    m_iHealth = 500; // Canı 100'e sıfırla
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0;
    m_bHasSecondWeapon = false; // Bu değişkeni kullanıyorsanız sıfırlayın
    m_iSecondaryAmmo = 0;     // Bu değişkeni kullanıyorsanız sıfırlayın
    m_currentWeapon = WeaponType::PISTOL;

    // --- Silah İstatistiklerini Tanımla ve Sıfırla ---
    m_weaponStats.clear();
    // PISTOL: 7 mermi, sonsuz yedek, hızlı ateş, hızlı reload
    m_weaponStats[WeaponType::PISTOL] = { 7, 7, -1, 15, 60 };
    // SHOTGUN: 2 mermi, sonsuz yedek, yavaş ateş, yavaş reload
    m_weaponStats[WeaponType::SHOTGUN] = { 2, 2, -1, 40, 100 };
    // SMG: 15 mermi, sonsuz yedek, çok hızlı ateş, orta hızda reload
    m_weaponStats[WeaponType::SMG] = { 15, 15, -1, 5, 80 };
}


SPRITEACTION Player::Update()
{
    float fDeltaTime = 1.0f / 30.0f; // Varsayılan 30 FPS
    if (game_engine && game_engine->GetFrameDelay() > 0) {
        fDeltaTime = static_cast<float>(game_engine->GetFrameDelay()) / 1000.0f;
    }

    if (m_iFireCooldown > 0) {
        m_iFireCooldown--;
    }

    // Yeniden doldurma sayacını işle
    if (m_iReloadTimer > 0) {
        m_iReloadTimer--;
        if (m_iReloadTimer == 0) { // Yeniden doldurma tamamlandı
            WeaponStats& stats = m_weaponStats.at(m_currentWeapon);

            // Sonsuz mermi ise direkt şarjörü doldur
            if (stats.totalAmmo == -1) {
                stats.currentAmmoInClip = stats.clipSize;
            }
            // (Gelecekte eklenebilir: Sınırlı mermi varsa, yedekten çekme mantığı)
        }
    }

    HandleInput(fDeltaTime);
    UpdateFrame();

    if (m_bDying || IsDead())
        return SA_KILL;

    return SA_NONE;
}

void Player::SwitchWeapon(WeaponType newWeapon)
{
    if (m_currentWeapon == newWeapon) return;

    m_currentWeapon = newWeapon;
    m_iFireCooldown = 0;
    m_iReloadTimer = 0; // Silah değiştirince mevcut reload iptal olur
}

void Player::HandleInput(float fDeltaTime)
{
    // Reload yapmıyorsa normal girdileri kontrol et
    if (m_iReloadTimer <= 0)
    {
        // Silah Değiştirme
        if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
        if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::SHOTGUN);
        if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);

        // Manuel Reload
        if (GetAsyncKeyState('R') & 0x8000) {
            StartReload();
        }
    }

    // --- Hareket Mantığı (her durumda çalışır) ---
    float currentSpeed = m_fSpeed;
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        currentSpeed *= SPRINT_SPEED_MULTIPLIER;
    }

    float dirX = 0.0f, dirY = 0.0f;
    if (GetAsyncKeyState('W') & 0x8000) dirY = -1.0f;
    if (GetAsyncKeyState('S') & 0x8000) dirY = 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) dirX = -1.0f;
    if (GetAsyncKeyState('D') & 0x8000) dirX = 1.0f;

    if (dirX != 0.0f || dirY != 0.0f)
    {
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) { dirX /= length; dirY /= length; }

        float moveAmountX = dirX * currentSpeed * fDeltaTime;
        float moveAmountY = dirY * currentSpeed * fDeltaTime;

        float currentPosX_f = static_cast<float>(m_rcPosition.left);
        float currentPosY_f = static_cast<float>(m_rcPosition.top);

        // X ekseni hareketi
        float nextPosX_f = currentPosX_f + moveAmountX;
        RECT nextRectX = { static_cast<int>(nextPosX_f), m_rcPosition.top, static_cast<int>(nextPosX_f + GetWidth()), m_rcPosition.bottom };
        bool collisionX = false;
        if (m_pMaze && TILE_SIZE > 0 && moveAmountX != 0) {
            if (m_pMaze->IsWall(nextRectX.left / TILE_SIZE, nextRectX.top / TILE_SIZE) || m_pMaze->IsWall((nextRectX.right - 1) / TILE_SIZE, nextRectX.top / TILE_SIZE) || m_pMaze->IsWall(nextRectX.left / TILE_SIZE, (nextRectX.bottom - 1) / TILE_SIZE) || m_pMaze->IsWall((nextRectX.right - 1) / TILE_SIZE, (nextRectX.bottom - 1) / TILE_SIZE))
            {
                collisionX = true;
            }
        }
        if (!collisionX) { currentPosX_f = nextPosX_f; }

        // Y ekseni hareketi
        float nextPosY_f = currentPosY_f + moveAmountY;
        RECT nextRectY = { static_cast<int>(currentPosX_f), static_cast<int>(nextPosY_f), static_cast<int>(currentPosX_f + GetWidth()), static_cast<int>(nextPosY_f + GetHeight()) };
        bool collisionY = false;
        if (m_pMaze && TILE_SIZE > 0 && moveAmountY != 0) {
            if (m_pMaze->IsWall(nextRectY.left / TILE_SIZE, nextRectY.top / TILE_SIZE) || m_pMaze->IsWall((nextRectY.right - 1) / TILE_SIZE, nextRectY.top / TILE_SIZE) || m_pMaze->IsWall(nextRectY.left / TILE_SIZE, (nextRectY.bottom - 1) / TILE_SIZE) || m_pMaze->IsWall((nextRectY.right - 1) / TILE_SIZE, (nextRectY.bottom - 1) / TILE_SIZE))
            {
                collisionY = true;
            }
        }
        if (!collisionY) { currentPosY_f = nextPosY_f; }

        SetPosition(static_cast<int>(currentPosX_f), static_cast<int>(currentPosY_f));
    }
}

void Player::StartReload()
{
    WeaponStats& stats = m_weaponStats.at(m_currentWeapon);
    if (m_iReloadTimer > 0 || stats.currentAmmoInClip == stats.clipSize || stats.totalAmmo == 0) {
        return;
    }
    m_iReloadTimer = stats.reloadTime;
}


void Player::Fire(int targetX, int targetY)
{
    if (m_iFireCooldown > 0 || m_iReloadTimer > 0 || !_pPlayerMissileBitmap || !game_engine) return;

    WeaponStats& stats = m_weaponStats.at(m_currentWeapon);

    if (stats.currentAmmoInClip <= 0) {
        StartReload();
        return;
    }

    stats.currentAmmoInClip--;
    m_iFireCooldown = stats.fireCooldown;

    POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
    float baseDirX = static_cast<float>(targetX - startPos.x);
    float baseDirY = static_cast<float>(targetY - startPos.y);
    float baseDistance = std::sqrt(baseDirX * baseDirX + baseDirY * baseDirY);
    if (baseDistance == 0) return;
    float normBaseX = baseDirX / baseDistance;
    float normBaseY = baseDirY / baseDistance;
    float missile_speed_factor = MISSILE_SPEED_SPS;

    switch (m_currentWeapon)
    {
    case WeaponType::PISTOL: {
        float velocityX = normBaseX * missile_speed_factor;
        float velocityY = normBaseY * missile_speed_factor;
        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        game_engine->AddSprite(pMissile);
        break;
    }
    case WeaponType::SHOTGUN: {
        const int pelletCount = 5;
        const float spreadAngleDeg = 15.0f;
        for (int i = 0; i < pelletCount; ++i) {
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
    case WeaponType::SMG: {
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
    if (m_iArmor > 0) {
        int damageAbsorbedByArmor = std::min(m_iArmor, amount / 2);
        m_iArmor -= damageAbsorbedByArmor;
        damageToHealth -= damageAbsorbedByArmor;
    }
    m_iHealth -= damageToHealth;
    m_iHealth = std::max(0, m_iHealth);
}

// --- Getter ve Diğer Fonksiyonlar ---
void Player::AddKey(int amount) { m_iKeys += amount; }
int  Player::GetKeys() const { return m_iKeys; }
void Player::ResetKeys() { m_iKeys = 0; }

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

const WeaponStats& Player::GetCurrentWeaponStats() const {
    return m_weaponStats.at(m_currentWeapon);
}
WeaponType Player::GetCurrentWeaponType() const {
    return m_currentWeapon;
}
bool Player::IsReloading() const {
    return m_iReloadTimer > 0;
}