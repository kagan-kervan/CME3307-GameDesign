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
    m_iHealth = 500;
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0;
    m_bHasSecondWeapon = false;
    m_iSecondaryAmmo = 0;
    m_currentWeapon = WeaponType::PISTOL;
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

// Player.cpp

// ... (diğer fonksiyonlar aynı)

void Player::HandleInput(float fDeltaTime)
{
    // Silah Değiştirme
    if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
    if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::SHOTGUN);
    if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);

    // Hareket Hızı
    float currentSpeed = m_fSpeed;
    // SPRINT_SPEED_MULTIPLIER'ın Player.h'de tanımlı olduğundan emin olun. Örnek: const float SPRINT_SPEED_MULTIPLIER = 1.5f;
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        currentSpeed *= SPRINT_SPEED_MULTIPLIER;
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
        // Yönü normalize et (çapraz hız sorununu çözmek için)
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) {
            dirX /= length;
            dirY /= length;
        }

        // Bu frame'deki hareket miktarını hesapla
        float moveAmountX = dirX * currentSpeed * fDeltaTime;
        float moveAmountY = dirY * currentSpeed * fDeltaTime;

        // Mevcut pozisyonu float olarak al
        float currentPosX_f = static_cast<float>(m_rcPosition.left);
        float currentPosY_f = static_cast<float>(m_rcPosition.top);

        // --- YENİ ÇARPIŞMA MANTIĞI: X ve Y Eksenlerini Ayrı Kontrol Et ---

        // 1. ADIM: Sadece X eksenindeki hareketi dene
        float nextPosX_f = currentPosX_f + moveAmountX;
        RECT nextRectX = {
            static_cast<int>(nextPosX_f),
            m_rcPosition.top, // Y pozisyonu şimdilik aynı
            static_cast<int>(nextPosX_f + GetWidth()),
            m_rcPosition.bottom
        };

        // X ekseninde çarpışma var mı?
        bool collisionX = false;
        if (m_pMaze && TILE_SIZE > 0 && moveAmountX != 0) { // Sadece hareket varsa kontrol et
            if (m_pMaze->IsWall(nextRectX.left / TILE_SIZE, nextRectX.top / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectX.right - 1) / TILE_SIZE, nextRectX.top / TILE_SIZE) ||
                m_pMaze->IsWall(nextRectX.left / TILE_SIZE, (nextRectX.bottom - 1) / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectX.right - 1) / TILE_SIZE, (nextRectX.bottom - 1) / TILE_SIZE))
            {
                collisionX = true;
            }
        }

        // Eğer X ekseninde çarpışma yoksa, yeni X pozisyonunu uygula
        if (!collisionX)
        {
            currentPosX_f = nextPosX_f;
        }

        // 2. ADIM: Sadece Y eksenindeki hareketi dene (belki de güncellenmiş X pozisyonu ile)
        float nextPosY_f = currentPosY_f + moveAmountY;
        RECT nextRectY = {
            static_cast<int>(currentPosX_f), // X pozisyonu güncellenmiş olabilir
            static_cast<int>(nextPosY_f),
            static_cast<int>(currentPosX_f + GetWidth()),
            static_cast<int>(nextPosY_f + GetHeight())
        };

        // Y ekseninde çarpışma var mı?
        bool collisionY = false;
        if (m_pMaze && TILE_SIZE > 0 && moveAmountY != 0) { // Sadece hareket varsa kontrol et
            if (m_pMaze->IsWall(nextRectY.left / TILE_SIZE, nextRectY.top / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectY.right - 1) / TILE_SIZE, nextRectY.top / TILE_SIZE) ||
                m_pMaze->IsWall(nextRectY.left / TILE_SIZE, (nextRectY.bottom - 1) / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectY.right - 1) / TILE_SIZE, (nextRectY.bottom - 1) / TILE_SIZE))
            {
                collisionY = true;
            }
        }

        // Eğer Y ekseninde çarpışma yoksa, yeni Y pozisyonunu uygula
        if (!collisionY)
        {
            currentPosY_f = nextPosY_f;
        }

        // Son olarak, geçerli olan son pozisyonu ayarla
        SetPosition(static_cast<int>(currentPosX_f), static_cast<int>(currentPosY_f));
    }
}

void Player::Fire(int targetX, int targetY)
{
    if (m_iFireCooldown > 0 || !_pPlayerMissileBitmap || !game_engine) return;

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

void Player::AddHealth(int amount) { m_iHealth = std::min(500, m_iHealth + amount); }
int  Player::GetHealth() const { return m_iHealth; }

void Player::AddArmor(int amount) { m_iArmor = std::min(100, m_iArmor + amount); }
int  Player::GetArmor() const { return m_iArmor; }

void Player::AddScore(int amount) { m_iScore += amount; }
int  Player::GetScore() const { return m_iScore; }

void Player::GiveSecondWeapon() { m_bHasSecondWeapon = true; }
bool Player::HasSecondWeapon() const { return m_bHasSecondWeapon; }

void Player::AddSecondaryAmmo(int amount) { if (m_bHasSecondWeapon) m_iSecondaryAmmo += amount; }
int  Player::GetSecondaryAmmo() const { return m_iSecondaryAmmo; }

void Player::Reset() {

    m_iFireCooldown = 0;
    m_iHealth = 500;
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0;
    m_bHasSecondWeapon = false;
    m_iSecondaryAmmo = 0;
    m_currentWeapon = WeaponType::PISTOL;
}