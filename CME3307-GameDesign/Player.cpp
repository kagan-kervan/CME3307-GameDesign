#include "Player.h"
#include "GameEngine.h"
#include "resource.h"

extern GameEngine* game_engine;
extern Bitmap* _pPlayerMissileBitmap;
extern RECT globalBounds;


Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap), m_pMaze(pMaze)
{
    m_fSpeed = 700.0f;
    m_iFireCooldown = 0;
    m_iHealth = 100;
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0;
    m_bHasSecondWeapon = false;
    m_iSecondaryAmmo = 0;
    // Oyuncu varsayýlan olarak tabanca ile baþlar
    m_currentWeapon = WeaponType::PISTOL;
}

SPRITEACTION Player::Update()
{
    float fDeltaTime = 1.0f / 60.0f;

    if (m_iFireCooldown > 0) {
        m_iFireCooldown--;
    }

    HandleInput(fDeltaTime);

    UpdateFrame();
    if (m_bDying)
        return SA_KILL;

    return SA_NONE;
}


// YENÝ: Silah deðiþtirme fonksiyonu
void Player::SwitchWeapon(WeaponType newWeapon)
{
    // Eðer zaten o silahtaysak bir þey yapma
    if (m_currentWeapon == newWeapon) {
        return;
    }
    m_iFireCooldown = 0;
    m_currentWeapon = newWeapon;
    // Ýsteðe baðlý: Silah deðiþtirme sesi çal veya bir UI güncellemesi yap
    // Örneðin: MessageBox(NULL, L"Silah Deðiþtirildi!", L"Bilgi", MB_OK);
}

void Player::HandleInput(float fDeltaTime)
{
    // --- Silah Deðiþtirme Kontrolü ---
    if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
    if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::SHOTGUN);
    if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);

    // --- HIZ BELÝRLEME (SPRINT KONTROLÜ) ---
    float currentSpeed = m_fSpeed; // Varsayýlan hýz normal yürüme hýzýdýr.

    // Eðer sol SHIFT tuþuna basýlýyorsa...
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        // Hýzý, sprint hýzýyla güncelle.
        currentSpeed *= SPRINT_SPEED_MULTIPLIER;
    }
    // -----------------------------------------


    // --- Hareket Kontrolü ---
    float dirX = 0.0f;
    float dirY = 0.0f;
    if (GetAsyncKeyState('W') & 0x8000) dirY = -1.0f;
    if (GetAsyncKeyState('S') & 0x8000) dirY = 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) dirX = -1.0f;
    if (GetAsyncKeyState('D') & 0x8000) dirX = 1.0f;

    if (dirX != 0.0f || dirY != 0.0f)
    {
        // Yön vektörünü normalize et
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) {
            dirX /= length;
            dirY /= length;
        }

        // HAREKET HESAPLAMASINDA GÜNCEL HIZI KULLAN
        float moveAmountX = currentSpeed * dirX * fDeltaTime;
        float moveAmountY = currentSpeed * dirY * fDeltaTime;

        float newX = m_rcPosition.left + moveAmountX;
        float newY = m_rcPosition.top + moveAmountY;
        RECT rcNewPos = { (int)newX, (int)newY, (int)newX + GetWidth(), (int)newY + GetHeight() };

        // Duvar çarpýþma kontrolü
        if (!m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, rcNewPos.top / TILE_SIZE) &&
            !m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, rcNewPos.top / TILE_SIZE) &&
            !m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE) &&
            !m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE))
        {
            SetPosition((int)newX, (int)newY);
        }
    }
}
// ATEÞ ETME MANTIÐI GÜNCELLENDÝ
void Player::Fire(int targetX, int targetY)
{
    if (m_iFireCooldown > 0) return;

    // Ateþ etme mantýðýný mevcut silaha göre ayarla
    switch (m_currentWeapon)
    {
    case WeaponType::PISTOL:
    {
        m_iFireCooldown = PISTOL_COOLDOWN; // Tabanca bekleme süresini ayarla

        // ---- Tek mermi ateþleme (standart kod) ----
        POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
        float dirX = static_cast<float>(targetX - startPos.x);
        float dirY = static_cast<float>(targetY - startPos.y);
        float distance = std::sqrt(dirX * dirX + dirY * dirY);
        if (distance == 0) return;
        float normX = dirX / distance;
        float normY = dirY / distance;
        float velocityX = normX * MISSILE_SPEED_SPS;
        float velocityY = normY * MISSILE_SPEED_SPS;
        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        pMissile->SetPosition(startPos.x - pMissile->GetWidth() / 2, startPos.y - pMissile->GetHeight() / 2);
        game_engine->AddSprite(pMissile);
        break;
    }

    case WeaponType::SHOTGUN:
    {
        m_iFireCooldown = SHOTGUN_COOLDOWN; // Shotgun bekleme süresini ayarla

        // ---- Çoklu mermi (saçma) ateþleme ----
        const int pelletCount = 3; // Kaç adet saçma ateþleneceði
        const float spreadAngle = 18.0f; // Saçýlma açýsý (derece)

        for (int i = 0; i < pelletCount; ++i)
        {
            POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
            float dirX = static_cast<float>(targetX - startPos.x);
            float dirY = static_cast<float>(targetY - startPos.y);

            // Rastgele bir saçýlma açýsý ekle
            float randomSpread = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngle * (3.14159f / 180.0f); // radyan cinsinden
            float newDirX = dirX * cos(randomSpread) - dirY * sin(randomSpread);
            float newDirY = dirX * sin(randomSpread) + dirY * cos(randomSpread);

            float distance = std::sqrt(newDirX * newDirX + newDirY * newDirY);
            if (distance == 0) continue;

            float normX = newDirX / distance;
            float normY = newDirY / distance;
            float velocityX = normX * MISSILE_SPEED_SPS;
            float velocityY = normY * MISSILE_SPEED_SPS;
            Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
            pMissile->SetPosition(startPos.x - pMissile->GetWidth() / 2, startPos.y - pMissile->GetHeight() / 2);
            game_engine->AddSprite(pMissile);
        }
        break;
    }

    case WeaponType::SMG:
    {
        m_iFireCooldown = SMG_COOLDOWN; // SMG bekleme süresini ayarla

        // ---- Hýzlý, hafifçe seken tek mermi ateþleme ----
        POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
        float dirX = static_cast<float>(targetX - startPos.x);
        float dirY = static_cast<float>(targetY - startPos.y);

        // Hafif bir sekme ekle (Shotgun'dan daha az)
        float randomSpread = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4.0f * (3.14159f / 180.0f);
        float newDirX = dirX * cos(randomSpread) - dirY * sin(randomSpread);
        float newDirY = dirX * sin(randomSpread) + dirY * cos(randomSpread);

        float distance = std::sqrt(newDirX * newDirX + newDirY * newDirY);
        if (distance == 0) return;

        float normX = newDirX / distance;
        float normY = newDirY / distance;
        float velocityX = normX * MISSILE_SPEED_SPS;
        float velocityY = normY * MISSILE_SPEED_SPS;
        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        pMissile->SetPosition(startPos.x - pMissile->GetWidth() / 2, startPos.y - pMissile->GetHeight() / 2);
        game_engine->AddSprite(pMissile);
        break;
    }
    }
}

void Player::AddKey(int amount) { m_iKeys += amount; }
int  Player::GetKeys() const { return m_iKeys; }

void Player::AddHealth(int amount) { m_iHealth = min(100, m_iHealth + amount); } // Can 100'ü geçemez
int  Player::GetHealth() const { return m_iHealth; }

void Player::AddArmor(int amount) { m_iArmor = min(100, m_iArmor + amount); } // Zırh 100'ü geçemez
int  Player::GetArmor() const { return m_iArmor; }

void Player::AddScore(int amount) { m_iScore += amount; }
int  Player::GetScore() const { return m_iScore; }

void Player::GiveSecondWeapon() { m_bHasSecondWeapon = true; }
bool Player::HasSecondWeapon() const { return m_bHasSecondWeapon; }

void Player::AddSecondaryAmmo(int amount) { if (m_bHasSecondWeapon) m_iSecondaryAmmo += amount; }
int  Player::GetSecondaryAmmo() const { return m_iSecondaryAmmo; }