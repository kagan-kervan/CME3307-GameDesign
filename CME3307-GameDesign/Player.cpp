#include "Player.h"
#include "GameEngine.h"
#include "Game.h"
#include "resource.h"
#include <windows.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <random>
#include "Camera.h" // YENİ: Camera nesnesine erişim için
#undef max
#undef min

#define PI 3.14159265358979323846

extern GameEngine* game_engine;
extern Bitmap* _pPlayerMissileBitmap;
extern RECT globalBounds;
extern int TILE_SIZE;
// YENİ: Game.cpp'deki global değişkenlere erişim için

extern Camera* camera; // Camera.h'ta tanımlı Camera sınıfının global örneği
// YENİ: FOVBackground.cpp'den kopyalanan yardımcı fonksiyon


Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap, SPRITE_TYPE_PLAYER), m_pMaze(pMaze)
{
    m_ptMouse = { 0, 0 }; // YENİ: Mouse pozisyonunu başlat
    Reset();
}

void Player::Reset()
{
    m_fSpeed = 1200.0f;
    m_iFireCooldown = 0;
    m_iReloadTimer = 0;
    m_iHealth = 500;
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0; 
    m_bHasMelter = true; // YENİ
    m_iSecondaryAmmo = 0;
    m_currentWeapon = WeaponType::PISTOL;

    m_fMaxStamina = 100.0f;
    m_fStamina = m_fMaxStamina;
    m_bIsSprinting = false;
    m_fStaminaRegenTimer = 0.0f;

    m_weaponStats.clear();
    // PISTOL: 7 mermi, sonsuz yedek, hızlı ateş, hızlı reload
    m_weaponStats[WeaponType::PISTOL] = { 7, 7, 63, 0, 10 };
    m_weaponStats[WeaponType::MELTER] = { 1, 1000000, 0, 0, 40 }; // Başlangıçta mermisi yok
}

// YENİ: Player.cpp'ye bu metodu ekleyin
void Player::UpdateMousePosition(int x, int y)
{
    m_ptMouse.x = x;
    m_ptMouse.y = y;
}

SPRITEACTION Player::Update()
{
    float fDeltaTime = 1.0f / 60.0f;
    if (game_engine && game_engine->GetFrameDelay() > 0) {
        fDeltaTime = static_cast<float>(game_engine->GetFrameDelay()) / 1000.0f;
    }

    if (m_iFireCooldown > 0) {
        m_iFireCooldown--;
    }

    if (m_iReloadTimer > 0) {
        m_iReloadTimer--;
        if (m_iReloadTimer == 0) {
            WeaponStats& stats = m_weaponStats.at(m_currentWeapon);
            if (stats.totalAmmo > 0) {
                stats.currentAmmoInClip = stats.clipSize;
                stats.totalAmmo -= stats.clipSize;
            }
        }
    }

    if (!m_bIsSprinting) {
        if (m_fStaminaRegenTimer > 0.0f) {
            m_fStaminaRegenTimer -= fDeltaTime;
        }
        else if (m_fStamina < m_fMaxStamina) {
            m_fStamina += STAMINA_REGEN_RATE * fDeltaTime;
            if (m_fStamina > m_fMaxStamina) {
                m_fStamina = m_fMaxStamina;
            }
        }
    }

    if (camera)
    {
        RECT rcPos = GetPosition();
        int playerCenterX = rcPos.left + GetWidth() / 2;
        int playerCenterY = rcPos.top + GetHeight() / 2;

        int mouseWorldX = m_ptMouse.x + camera->x;
        int mouseWorldY = m_ptMouse.y + camera->y;

        // Hedef açıyı hesapla.
        double targetAngle = atan2(-(double)(mouseWorldY - playerCenterY), (double)mouseWorldX - playerCenterX) + (PI / 2.0) + PI;

        // Mevcut açıyı yumuşak bir geçişle hedef açıya yaklaştır
        double currentAngle = GetRotation();
        SetRotation(LerpAngle(currentAngle, targetAngle, 0.25f));

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

    // YENİ: Silaha sahip değilse geçiş yapmayı engelle
    if (newWeapon == WeaponType::MELTER && !m_bHasMelter)
    {
        return;
    }

    m_currentWeapon = newWeapon;
    m_iFireCooldown = 0;
    m_iReloadTimer = 0;
}
void Player::HandleInput(float fDeltaTime)
{
    if (m_iReloadTimer <= 0) {
        if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
        // DEĞİŞTİRİLDİ: '2' tuşu artık MELTER silahına geçer (eğer varsa)
        if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::MELTER);
        // '3' tuşu kaldırıldı.
        // if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);

        if (GetAsyncKeyState('R') & 0x8000) {
            StartReload();
        }
    }

    float currentSpeed = m_fSpeed;
    m_bIsSprinting = false;

    if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) && m_fStamina > 0) {
        currentSpeed *= SPRINT_SPEED_MULTIPLIER;
        m_bIsSprinting = true;
        m_fStamina -= STAMINA_DEPLETE_RATE * fDeltaTime;
        if (m_fStamina < 0) m_fStamina = 0;
        m_fStaminaRegenTimer = STAMINA_REGEN_DELAY;
    }

    float dirX = 0.0f, dirY = 0.0f;
    if (GetAsyncKeyState('W') & 0x8000) dirY = -1.0f;
    if (GetAsyncKeyState('S') & 0x8000) dirY = 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) dirX = -1.0f;
    if (GetAsyncKeyState('D') & 0x8000) dirX = 1.0f;

    if (dirX != 0.0f || dirY != 0.0f) {
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) {
            dirX /= length;
            dirY /= length;
        }

        float moveAmountX = dirX * currentSpeed * fDeltaTime;
        float moveAmountY = dirY * currentSpeed * fDeltaTime;

        float currentPosX_f = static_cast<float>(m_rcPosition.left);
        float currentPosY_f = static_cast<float>(m_rcPosition.top);

        float nextPosX_f = currentPosX_f + moveAmountX;
        RECT nextRectX = { static_cast<int>(nextPosX_f), m_rcPosition.top, static_cast<int>(nextPosX_f + GetWidth()), m_rcPosition.bottom };
        bool collisionX = false;
        if (m_pMaze && TILE_SIZE > 0 && moveAmountX != 0.0f) {
            if (m_pMaze->IsWall(nextRectX.left / TILE_SIZE, nextRectX.top / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectX.right - 1) / TILE_SIZE, nextRectX.top / TILE_SIZE) ||
                m_pMaze->IsWall(nextRectX.left / TILE_SIZE, (nextRectX.bottom - 1) / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectX.right - 1) / TILE_SIZE, (nextRectX.bottom - 1) / TILE_SIZE))
            {
                collisionX = true;
            }
        }
        if (!collisionX) {
            currentPosX_f = nextPosX_f;
        }

        float nextPosY_f = currentPosY_f + moveAmountY;
        RECT nextRectY = { static_cast<int>(currentPosX_f), static_cast<int>(nextPosY_f), static_cast<int>(currentPosX_f + GetWidth()), static_cast<int>(nextPosY_f + GetHeight()) };
        bool collisionY = false;
        if (m_pMaze && TILE_SIZE > 0 && moveAmountY != 0.0f) {
            if (m_pMaze->IsWall(nextRectY.left / TILE_SIZE, nextRectY.top / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectY.right - 1) / TILE_SIZE, nextRectY.top / TILE_SIZE) ||
                m_pMaze->IsWall(nextRectY.left / TILE_SIZE, (nextRectY.bottom - 1) / TILE_SIZE) ||
                m_pMaze->IsWall((nextRectY.right - 1) / TILE_SIZE, (nextRectY.bottom - 1) / TILE_SIZE))
            {
                collisionY = true;
            }
        }
        if (!collisionY) {
            currentPosY_f = nextPosY_f;
        }

        SetPosition(static_cast<int>(currentPosX_f), static_cast<int>(currentPosY_f));
    }
}

void Player::StartReload()
{
    WeaponStats& stats = m_weaponStats.at(m_currentWeapon);
    if (m_iReloadTimer > 0 || stats.currentAmmoInClip == stats.clipSize || stats.totalAmmo <= 0) {
        return;
    }
    m_iReloadTimer = stats.reloadTime;
    PlaySound(MAKEINTRESOURCE(IDW_RELOAD), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
}


void Player::Fire(int targetX, int targetY)
{
    // Melter mermisi için global bitmap'e ihtiyacımız var (Game.cpp'de tanımlanacak)
    extern Bitmap* _pMelterMissileBitmap;

    if (m_iFireCooldown > 0 || m_iReloadTimer > 0 || !game_engine) return;

    WeaponStats& stats = m_weaponStats.at(m_currentWeapon);

    if (stats.currentAmmoInClip <= 0) {
        // Mermi bittiyse ve yedek mermi varsa doldur
        if (stats.totalAmmo > 0) {
            StartReload();
        }
        return;
    }

    stats.currentAmmoInClip--;
    m_iFireCooldown = stats.fireCooldown;

    // Mermi sprite'ını oluşturma ve ateşleme (ortak kısım)
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
        if (!_pPlayerMissileBitmap) return;
        float velocityX = normBaseX * missile_speed_factor;
        float velocityY = normBaseY * missile_speed_factor;
        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        // YENİ: Mermi tipini ayarlıyoruz
        game_engine->AddSprite(pMissile);
        PlaySound(MAKEINTRESOURCE(IDW_SHOOT), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
        break;
    }
    case WeaponType::MELTER: {
        if (!_pMelterMissileBitmap) return;
        float velocityX = normBaseX * missile_speed_factor * 0.7f; // Biraz daha yavaş
        float velocityY = normBaseY * missile_speed_factor * 0.7f;
        Missile* pMissile = new Missile(_pMelterMissileBitmap, globalBounds, startPos, velocityX, velocityY, SpriteType::SPRITE_TYPE_MELTER_MISSILE);
        // YENİ: Mermi tipini özel olarak ayarlıyoruz
        game_engine->AddSprite(pMissile);
        // Farklı bir ses efekti çalınabilir
        PlaySound(MAKEINTRESOURCE(IDW_SHOOT), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
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

void Player::AddKey(int amount) { m_iKeys += amount; }
int  Player::GetKeys() const { return m_iKeys; }
void Player::ResetKeys() { m_iKeys = 0; }

void Player::AddHealth(int amount) { m_iHealth = std::min(500, m_iHealth + amount); }
int  Player::GetHealth() const { return m_iHealth; }

void Player::AddArmor(int amount) { m_iArmor = std::min(100, m_iArmor + amount); }
int  Player::GetArmor() const { return m_iArmor; }

void Player::AddScore(int amount) { m_iScore += amount; }
int  Player::GetScore() const { return m_iScore; }

void Player::GiveMelter() {
    m_bHasMelter = true;
}
bool Player::HasMelter() const {
    return m_bHasMelter;
}

void Player::AddMelterAmmo(int amount) {
    if (m_bHasMelter) {
        m_weaponStats.at(WeaponType::MELTER).totalAmmo += amount;
    }
    m_weaponStats.at(WeaponType::PISTOL).totalAmmo += 10*amount;
    
}
int Player::GetMelterAmmo() const {
    if (m_bHasMelter) {
        return m_weaponStats.at(WeaponType::MELTER).totalAmmo + m_weaponStats.at(WeaponType::MELTER).currentAmmoInClip;
    }
    return 0;
}

const WeaponStats& Player::GetCurrentWeaponStats() const {
    return m_weaponStats.at(m_currentWeapon);
}
WeaponType Player::GetCurrentWeaponType() const {
    return m_currentWeapon;
}
bool Player::IsReloading() const {
    return m_iReloadTimer > 0;
}
float Player::GetStamina() const {
    return m_fStamina;
}
float Player::GetMaxStamina() const {
    return m_fMaxStamina;
}