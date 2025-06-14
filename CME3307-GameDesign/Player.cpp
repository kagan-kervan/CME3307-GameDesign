#include "Player.h"
#include "GameEngine.h"
#include <string>
#include <random> // Shotgun da��l�m� i�in

extern GameEngine* _pGame;
extern Bitmap* _pMissileBitmap;

Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap), m_pMaze(pMaze)
{
    m_fSpeed = 1500.0f;
    m_iFireCooldown = 0;

    // Oyuncu varsay�lan olarak tabanca ile ba�lar
    m_currentWeapon = WeaponType::PISTOL;
}

SPRITEACTION Player::Update()
{
    HandleInput();

    // Hareket i�in Sprite::Update'i �a��rm�yoruz, ��nk� pozisyonu HandleInput'ta kendimiz ayarl�yoruz.
    // Sadece animasyon gibi ba�ka �zellikler varsa �a��r�labilir.
    // return Sprite::Update(); // Bu sat�r yerine a�a��dakini kullan�n:

    UpdateFrame(); // Sadece animasyon karesini g�nceller
    if (m_bDying)
        return SA_KILL;

    return SA_NONE;
}

// YEN�: Silah de�i�tirme fonksiyonu
void Player::SwitchWeapon(WeaponType newWeapon)
{
    // E�er zaten o silahtaysak bir �ey yapma
    if (m_currentWeapon == newWeapon) {
        return;
    }
    m_iFireCooldown = 0;
    m_currentWeapon = newWeapon;
    // �ste�e ba�l�: Silah de�i�tirme sesi �al veya bir UI g�ncellemesi yap
    // �rne�in: MessageBox(NULL, L"Silah De�i�tirildi!", L"Bilgi", MB_OK);
}


// ATE� ETME MANTI�I G�NCELLEND�
void Player::Fire(int targetX, int targetY)
{
    if (m_iFireCooldown > 0) return;

    // Ate� etme mant���n� mevcut silaha g�re ayarla
    switch (m_currentWeapon)
    {
    case WeaponType::PISTOL:
    {
        m_iFireCooldown = PISTOL_COOLDOWN; // Tabanca bekleme s�resini ayarla

        // ---- Tek mermi ate�leme (standart kod) ----
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
        m_iFireCooldown = SHOTGUN_COOLDOWN; // Shotgun bekleme s�resini ayarla

        // ---- �oklu mermi (sa�ma) ate�leme ----
        const int pelletCount = 3; // Ka� adet sa�ma ate�lenece�i
        const float spreadAngle = 18.0f; // Sa��lma a��s� (derece)

        for (int i = 0; i < pelletCount; ++i)
        {
            POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
            float dirX = static_cast<float>(targetX - startPos.x);
            float dirY = static_cast<float>(targetY - startPos.y);

            // Rastgele bir sa��lma a��s� ekle
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
        m_iFireCooldown = SMG_COOLDOWN; // SMG bekleme s�resini ayarla

        // ---- H�zl�, hafif�e seken tek mermi ate�leme ----
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


void Player::HandleInput(float fDeltaTime)
{
    // --- Silah De�i�tirme Kontrol� ---
    if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
    if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::SHOTGUN);
    if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);

    // --- HIZ BEL�RLEME (SPRINT KONTROL�) ---
    float currentSpeed = m_fSpeed; // Varsay�lan h�z normal y�r�me h�z�d�r.

    // E�er sol SHIFT tu�una bas�l�yorsa...
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        // H�z�, sprint h�z�yla g�ncelle.
        currentSpeed *= SPRINT_SPEED_MULTIPLIER;
    }
    // -----------------------------------------


    // --- Hareket Kontrol� ---
    float dirX = 0.0f;
    float dirY = 0.0f;
    if (GetAsyncKeyState('W') & 0x8000) dirY = -1.0f;
    if (GetAsyncKeyState('S') & 0x8000) dirY = 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) dirX = -1.0f;
    if (GetAsyncKeyState('D') & 0x8000) dirX = 1.0f;

    if (dirX != 0.0f || dirY != 0.0f)
    {
        // Y�n vekt�r�n� normalize et
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) {
            dirX /= length;
            dirY /= length;
        }

        // HAREKET HESAPLAMASINDA G�NCEL HIZI KULLAN
        float moveAmountX = currentSpeed * dirX * fDeltaTime;
        float moveAmountY = currentSpeed * dirY * fDeltaTime;

        float newX = m_rcPosition.left + moveAmountX;
        float newY = m_rcPosition.top + moveAmountY;
        RECT rcNewPos = { (int)newX, (int)newY, (int)newX + GetWidth(), (int)newY + GetHeight() };

        // Duvar �arp��ma kontrol�
        if (!m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, rcNewPos.top / TILE_SIZE) &&
            !m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, rcNewPos.top / TILE_SIZE) &&
            !m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE) &&
            !m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE))
        {
            SetPosition((int)newX, (int)newY);
        }
    }
}