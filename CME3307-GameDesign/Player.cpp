// Player.cpp
#include "Player.h"
#include "GameEngine.h" // game_engine global değişkeni için
#include "resource.h"   // IDB_PLAYER_MISSILE gibi kaynak ID'leri için (varsayım)
#include <windows.h>    // GetAsyncKeyState için
#include <string>       // std::to_string (can gösterimi için)
#include <algorithm>    // std::min, std::max için
#undef max
#undef min
// Dışarıdan gelen global değişkenler
extern GameEngine* game_engine;
extern Bitmap* _pPlayerMissileBitmap; // Oyuncu mermi bitmap'i
extern RECT globalBounds; // Mermilerin hareket edeceği genel sınırlar
extern int TILE_SIZE;     // Global TILE_SIZE

// Kurucu Metot
Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap, SPRITE_TYPE_PLAYER), // Sprite tipini kurucuda belirt
    m_pMaze(pMaze)
{
    m_fSpeed = 6.0f; // Hızı biraz daha makul bir değere ayarlayalım (saniyede tile veya piksel)
    // Eski 1500.0f değeri fDeltaTime ile çarpıldığında çok yüksek olabilir.
    // Ya da fDeltaTime'ı Update içinde daha doğru hesaplamalıyız.
    // Şimdilik bu değeri PIXEL_PER_SECOND gibi düşünelim.
    // Update içindeki fDeltaTime ile çarpılacak. Örnek: 200 piksel/saniye
    m_fSpeed = 250.0f; // Saniyede 250 piksel hızında

    m_iFireCooldown = 0;
    m_iHealth = 100;
    m_iArmor = 0;
    m_iKeys = 0;
    m_iScore = 0;
    m_bHasSecondWeapon = false;
    m_iSecondaryAmmo = 0;
    m_currentWeapon = WeaponType::PISTOL;

    // Oyuncu için animasyon ayarları (varsa)
    // SetNumFrames(4, FALSE); // Örneğin 4 frame'li bir yürüme animasyonu
    // SetFrameDelay(5);
}

SPRITEACTION Player::Update()
{
    // Gerçek delta time hesaplaması oyun motorundan gelmeli.
    // Şimdilik sabit bir değer varsayıyoruz (30 FPS için).
    // GameEngine::GetFrameDelay() 1000/FPS verir.
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

    HandleInput(fDeltaTime); // Delta time'ı HandleInput'a geçir

    // Sprite'ın temel Update'ini çağır (animasyon vs. için)
    // Ancak Player hareketi HandleInput'ta yapıldığı için,
    // Sprite::Update pozisyonu tekrar değiştirmemeli.
    // Sprite::Update yerine sadece UpdateFrame() çağrılabilir.
    UpdateFrame(); // Sadece animasyon frame'ini güncelle

    if (m_bDying || IsDead()) // m_bDying Sprite'tan, IsDead Player'dan
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


    m_iFireCooldown = 0; // Silah değiştirince cooldown sıfırlansın
    m_currentWeapon = newWeapon;
}

void Player::HandleInput(float fDeltaTime)
{
    // Silah Değiştirme
    if (GetAsyncKeyState('1') & 0x8000) SwitchWeapon(WeaponType::PISTOL);
    if (GetAsyncKeyState('2') & 0x8000) SwitchWeapon(WeaponType::SHOTGUN); // İkinci silah varsa
    if (GetAsyncKeyState('3') & 0x8000) SwitchWeapon(WeaponType::SMG);     // Üçüncü silah varsa

    // Hareket Hızı
    float currentSpeed = m_fSpeed;
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
        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0) { // 0'a bölme hatasını önle
            dirX /= length;
            dirY /= length;
        }

        float moveAmountX = dirX * currentSpeed * fDeltaTime;
        float moveAmountY = dirY * currentSpeed * fDeltaTime;

        // Yeni pozisyonu hesapla (float olarak)
        float newPosX_f = static_cast<float>(m_rcPosition.left) + moveAmountX;
        float newPosY_f = static_cast<float>(m_rcPosition.top) + moveAmountY;

        // Çarpışma için yeni RECT oluştur
        RECT rcNewPos = {
            static_cast<int>(newPosX_f),
            static_cast<int>(newPosY_f),
            static_cast<int>(newPosX_f + GetWidth()),
            static_cast<int>(newPosY_f + GetHeight())
        };

        // Duvar çarpışma kontrolü (daha sağlam bir yöntem)
        // Oyuncunun 4 köşesini de kontrol et
        bool collision = false;
        if (m_pMaze && TILE_SIZE > 0) { // m_pMaze ve TILE_SIZE null/sıfır değilse
            // Sol üst
            if (m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, rcNewPos.top / TILE_SIZE)) collision = true;
            // Sağ üst
            if (!collision && m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, rcNewPos.top / TILE_SIZE)) collision = true;
            // Sol alt
            if (!collision && m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE)) collision = true;
            // Sağ alt
            if (!collision && m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE)) collision = true;
        }


        if (!collision)
        {
            SetPosition(rcNewPos); // Pozisyonu RECT olarak ayarla
        }
        // Eğer çarpışma varsa, eksen bazlı kaydırma denenebilir (duvara sürtünme efekti için)
        // Ama şimdilik basit tutuyoruz, çarpışma varsa hareket etme.
    }
}

void Player::Fire(int targetX, int targetY)
{
    if (m_iFireCooldown > 0 || !_pPlayerMissileBitmap || !game_engine) return;

    POINT startPos = { m_rcPosition.left + GetWidth() / 2, m_rcPosition.top + GetHeight() / 2 };
    float baseDirX = static_cast<float>(targetX - startPos.x);
    float baseDirY = static_cast<float>(targetY - startPos.y);
    float baseDistance = std::sqrt(baseDirX * baseDirX + baseDirY * baseDirY);

    if (baseDistance == 0) return; // Hedef tam üstünde, ateş etme

    float normBaseX = baseDirX / baseDistance;
    float normBaseY = baseDirY / baseDistance;

    switch (m_currentWeapon)
    {
    case WeaponType::PISTOL:
    {
        m_iFireCooldown = PISTOL_COOLDOWN;
        float velocityX = normBaseX * MISSILE_SPEED_SPS * (game_engine->GetFrameDelay() / 1000.0f); // Hızı delta time ile ölçekle
        float velocityY = normBaseY * MISSILE_SPEED_SPS * (game_engine->GetFrameDelay() / 1000.0f);

        // Missile sınıfı kullanılacaksa:
        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        // pMissile->SetPosition(startPos.x - pMissile->GetWidth() / 2, startPos.y - pMissile->GetHeight() / 2); // Missile kurucusu pozisyonu ayarlar
        game_engine->AddSprite(pMissile);
        break;
    }
    case WeaponType::SHOTGUN:
    {
        // if (!m_bHasSecondWeapon) break; // İkinci silah yoksa ateş etme
        m_iFireCooldown = SHOTGUN_COOLDOWN;
        const int pelletCount = 5; // Saçma sayısı
        const float spreadAngleDeg = 15.0f; // Saçılma açısı (derece)

        for (int i = 0; i < pelletCount; ++i)
        {
            // Her saçma için hafif rastgele bir açı
            float randomAngleOffset = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngleDeg; // -spread/2 ile +spread/2 arası
            float currentAngleRad = atan2(normBaseY, normBaseX) + randomAngleOffset * (3.14159265f / 180.0f);

            float dirX = cos(currentAngleRad);
            float dirY = sin(currentAngleRad);

            float velocityX = dirX * MISSILE_SPEED_SPS * (game_engine->GetFrameDelay() / 1000.0f);
            float velocityY = dirY * MISSILE_SPEED_SPS * (game_engine->GetFrameDelay() / 1000.0f);

            Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
            game_engine->AddSprite(pMissile);
        }
        break;
    }
    case WeaponType::SMG:
    {
        // if (!m_bHasSecondWeapon) break; // Üçüncü silah yoksa ateş etme
        m_iFireCooldown = SMG_COOLDOWN;
        // SMG için hafif bir sekme (shotgun'dan daha az)
        float randomAngleOffset = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 5.0f; // +/- 2.5 derece
        float currentAngleRad = atan2(normBaseY, normBaseX) + randomAngleOffset * (3.14159265f / 180.0f);

        float dirX = cos(currentAngleRad);
        float dirY = sin(currentAngleRad);

        float velocityX = dirX * MISSILE_SPEED_SPS * (game_engine->GetFrameDelay() / 1000.0f);
        float velocityY = dirY * MISSILE_SPEED_SPS * (game_engine->GetFrameDelay() / 1000.0f);

        Missile* pMissile = new Missile(_pPlayerMissileBitmap, globalBounds, startPos, velocityX, velocityY);
        game_engine->AddSprite(pMissile);
        break;
    }
    }
}

// YENİ: Hasar alma fonksiyonu
void Player::TakeDamage(int amount)
{
    if (m_iHealth <= 0) return; // Zaten ölü

    int damageToHealth = amount;
    if (m_iArmor > 0)
    {
        int damageAbsorbedByArmor = std::min(m_iArmor, amount / 2); // Zırh hasarın yarısını emer (örnek)
        m_iArmor -= damageAbsorbedByArmor;
        damageToHealth -= damageAbsorbedByArmor;
    }

    m_iHealth -= damageToHealth;
    m_iHealth = std::max(0, m_iHealth); // Can 0'ın altına düşmesin

    if (m_iHealth <= 0)
    {
        // Oyuncu öldü!
        // m_bDying = TRUE; // Sprite'ın ölme animasyonunu başlatabilir
        // Burada bir oyun sonu ekranı çağrılabilir veya farklı bir işlem yapılabilir.
        // OutputDebugString(L"PLAYER DIED!\n");
    }
}


// Diğer Get/Set metodları aynı kalıyor
void Player::AddKey(int amount) { m_iKeys += amount; }
int  Player::GetKeys() const { return m_iKeys; }

void Player::AddHealth(int amount) { m_iHealth = std::min(100, m_iHealth + amount); }
int  Player::GetHealth() const { return m_iHealth; }

void Player::AddArmor(int amount) { m_iArmor = std::min(100, m_iArmor + amount); }
int  Player::GetArmor() const { return m_iArmor; }

void Player::AddScore(int amount) { m_iScore += amount; }
int  Player::GetScore() const { return m_iScore; }

void Player::GiveSecondWeapon() { m_bHasSecondWeapon = true; } // Bu metodun çağrılması gerekiyor bir yerden
bool Player::HasSecondWeapon() const { return m_bHasSecondWeapon; }

void Player::AddSecondaryAmmo(int amount) { if (m_bHasSecondWeapon) m_iSecondaryAmmo += amount; }
int  Player::GetSecondaryAmmo() const { return m_iSecondaryAmmo; }