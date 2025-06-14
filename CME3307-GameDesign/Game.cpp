//
// Created by ahmet on 3.06.2025.
//
#include <string> // Bu satýrý en baþa ekleyin
#include "Game.h"
#include <vector>
#include "Enemy.h"
#include <random> // YENİ: Rastgele konum bulmak için
//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0,0,4000,4000 };
// YENİ: Düşman spawn zamanlayıcısı için değişken tanımı ve interval
DWORD g_dwLastSpawnTime = 0;
const DWORD ENEMY_SPAWN_INTERVAL = 7000; // 7 saniye (milisaniye cinsinden)
GameEngine* game_engine;
Player* charSprite;
MazeGenerator* mazeGenerator;
Bitmap* _pEnemyMissileBitmap;
FOVBackground* fovEffect;
int TILE_SIZE;
Camera* camera;
HDC         offscreenDC;
HBITMAP     offscreenBitmap;
Background* background;
Bitmap* wallBitmap;
Bitmap* charBitmap;
Bitmap* _pEnemyBitmap;
HINSTANCE   instance;
int window_X, window_Y;
extern RECT globalBounds; // Make sure globalBounds is accessible
Bitmap* _pPlayerMissileBitmap;


Bitmap* healthPWBitmap = nullptr;
Bitmap* ammoPWBitmap = nullptr;
Bitmap* armorPWBitmap = nullptr;
Bitmap* pointPWBitmap = nullptr;
Bitmap* floorBitmap = nullptr;
Bitmap* keyBitmap = nullptr;
Bitmap* endPointBitmap = nullptr;
Bitmap* secondWeaponBitmap = nullptr;

bool isLevelFinished = false;
int currentLevel; 

BOOL GameInitialize(HINSTANCE hInst)
{
    window_X = 1500;
    window_Y = 700;
    game_engine = new GameEngine(hInst, TEXT("Maze Game"), TEXT("Maze Game"),
        IDI_SPACEOUT, IDI_SPACEOUT_SM, window_X, window_Y);
    if (game_engine == NULL)
        return FALSE;

    game_engine->SetFrameRate(30);
    instance = hInst;

    return TRUE;
}
// DEĞİŞİKLİK: Camera sınıfının yeni kurucu metodunu kullanıyoruz.
Camera::Camera(Sprite* target, int w, int h)
    : m_pTarget(target), width(w), height(h), m_fLerpFactor(0.08f) // Yumuşatma faktörünü ayarla
{
    if (m_pTarget)
    {
        // Başlangıç pozisyonunu direkt hedefe ayarla
        RECT pos = m_pTarget->GetPosition();
        m_fCurrentX = (float)(pos.left + m_pTarget->GetWidth() / 2 - width / 2);
        m_fCurrentY = (float)(pos.top + m_pTarget->GetHeight() / 2 - height / 2);
        x = (int)m_fCurrentX;
        y = (int)m_fCurrentY;
    }
    else
    {
        x = y = 0;
        m_fCurrentX = m_fCurrentY = 0.0f;
    }
}

// YENİ: Kameranın yumuşak hareketini sağlayan Update fonksiyonu.
void Camera::Update()
{
    if (!m_pTarget) return;

    // 1. Hedef pozisyonu hesapla (ekranın ortası oyuncunun üzerinde olacak şekilde)
    RECT pos = m_pTarget->GetPosition();
    float targetX = (float)(pos.left + m_pTarget->GetWidth() / 2 - width / 2);
    float targetY = (float)(pos.top + m_pTarget->GetHeight() / 2 - height / 2);

    // 2. Mevcut pozisyonu hedefe doğru yumuşakça kaydır (Interpolation)
    m_fCurrentX += (targetX - m_fCurrentX) * m_fLerpFactor;
    m_fCurrentY += (targetY - m_fCurrentY) * m_fLerpFactor;

    // 3. Tamsayı pozisyonlarını güncelle
    x = (int)round(m_fCurrentX);
    y = (int)round(m_fCurrentY);
}

void GameStart(HWND hWindow)
{
    srand(GetTickCount());
    offscreenDC = CreateCompatibleDC(GetDC(hWindow));
    offscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        game_engine->GetWidth(), game_engine->GetHeight());
    SelectObject(offscreenDC, offscreenBitmap);

    HDC hDC = GetDC(hWindow);
    LoadBitmaps(hDC);
    _pPlayerMissileBitmap = new Bitmap(hDC, IDB_MISSILE, instance);
    TILE_SIZE = wallBitmap->GetHeight();
    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    // DONMAYI AZALTMAK İÇİN: Labirent boyutunu test için makul bir seviyeye getirdim.
    // Performansın iyi olduğundan emin olunca tekrar büyütebilirsiniz.
    mazeGenerator = new MazeGenerator(15, 15);
    charSprite = new Player(charBitmap, mazeGenerator);
    currentLevel = 1;
    GenerateLevel(currentLevel);
    game_engine->AddSprite(charSprite);

    // DEĞİŞİKLİK: Kamera artık oyuncuyu hedef alarak oluşturuluyor.
    camera = new Camera(charSprite, window_X, window_Y);
    fovEffect = new FOVBackground(charSprite, 90, 350); // Fener mesafesini biraz artırdım


    // Create and add multiple enemies
 // Create and add multiple enemies
    // Düşmanları oluştur
    for (int i = 0; i < 5; i++) // Sayıyı test için azalttım
    {
        EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;

        // HAREKET HATASI DÜZELTİLDİ: Düşmanı tüm harita sınırlarıyla (globalBounds) oluşturuyoruz.
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);

        int ex, ey;
        do {
            ex = (rand() % (15 * 2));
            ey = (rand() % (15 * 2));
        } while (mazeGenerator->IsWall(ex, ey));

        pEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}

void GameEnd()
{
    // Close the MIDI player for the background music
    game_engine->CloseMIDIPlayer();

    // Cleanup the offscreen device context and bitmap
    DeleteObject(offscreenBitmap);
    DeleteDC(offscreenDC);

    // Cleanup bitmaps
    delete _pEnemyMissileBitmap;

    // Cleanup the background
    delete background;

    // Cleanup the sprites
    game_engine->CleanupSprites();

    // Cleanup the game engine
    delete game_engine;
}

void GameActivate(HWND hWindow)
{
    // Resume the background music
}

void GameDeactivate(HWND hWindow)
{
    // Pause the background music
}

void GamePaint(HDC hDC)
{
    // DEĞİŞİKLİK: CenterCameraOnSprite artık çağrılmıyor. Kamera kendi kendini güncelliyor.
    background->Draw(hDC, 0, 0); // Arkaplan artık kameraya göre oynamamalı, sabit kalmalı.

    // Draw all non-collidable tiles
    for (const auto& tile : nonCollidableTiles) {
        tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    // Draw all sprites with camera offset
    for (Sprite* sprite : game_engine->GetSprites()) {
        sprite->Draw(hDC, camera->x, camera->y);
    }

    // Fener efekti en son çizilir
    fovEffect->Draw(hDC, camera->x, camera->y);
}

void GameCycle()
{
    // YENİ: Kamerayı her döngüde güncelle
    if (camera) {
        camera->Update();
    }

    background->Update();
    game_engine->UpdateSprites();

    // YENİ: Düşman spawn etme mantığı
    if (GetTickCount() > g_dwLastSpawnTime + ENEMY_SPAWN_INTERVAL)
    {
        SpawnEnemyNearPlayer();
        g_dwLastSpawnTime = GetTickCount(); // Zamanlayıcıyı sıfırla
    }

    if (isLevelFinished) {
        OnLevelComplete();
        isLevelFinished = false;
        return;
    }

    if (fovEffect && camera) {
        fovEffect->Update(camera->x, camera->y);
    }

    HWND  hWindow = game_engine->GetWindow();
    HDC   hDC = GetDC(hWindow);

    // DEĞİŞİKLİK: CenterCameraOnSprite artık çağrılmıyor.
    GamePaint(offscreenDC);

    BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
        offscreenDC, 0, 0, SRCCOPY);

    ReleaseDC(hWindow, hDC);
}
// YENİ: Oyuncunun yakınına rastgele bir düşman spawn eden fonksiyon
void SpawnEnemyNearPlayer()
{
    if (!mazeGenerator || !charSprite || !_pEnemyBitmap || !game_engine) return;

    // Oyuncunun tile koordinatlarını al
    RECT playerPos = charSprite->GetPosition();
    int playerTileX = playerPos.left / TILE_SIZE;
    int playerTileY = playerPos.top / TILE_SIZE;

    // Oyuncunun etrafında rastgele bir boş tile bulmaya çalış
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(-5, 5); // Oyuncunun +/- 5 tile uzağında bir yer

    int spawnTileX, spawnTileY;
    int tryCount = 0;
    const int maxTries = 20; // Sonsuz döngüyü önlemek için

    do {
        spawnTileX = playerTileX + distr(gen);
        spawnTileY = playerTileY + distr(gen);
        tryCount++;
    } while (mazeGenerator->IsWall(spawnTileX, spawnTileY) && tryCount < maxTries);

    // Eğer geçerli bir yer bulunduysa düşmanı oluştur
    if (tryCount < maxTries)
    {
        EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;

        Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);

        pEnemy->SetPosition(spawnTileX * TILE_SIZE, spawnTileY * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}
void HandleKeys()
{

}

void MouseButtonDown(int x, int y, BOOL bLeft)
{

    // BU FONKSÝYON AYNI KALIYOR
    if (bLeft)
    {
        int targetWorldX = x + camera->x;
        int targetWorldY = y + camera->y;

        Player* pPlayer = static_cast<Player*>(charSprite);
        if (pPlayer)
        {
            pPlayer->Fire(targetWorldX, targetWorldY);
        }
    }
}

void MouseButtonUp(int x, int y, BOOL bLeft)
{

}

void MouseMove(int x, int y)
{
    if (fovEffect)
        fovEffect->UpdateMousePos(x, y);
}

void HandleJoystick(JOYSTATE jsJoystickState) {

}

void GenerateMaze(Bitmap* tileBit) {
    mazeGenerator->generateMaze();
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->GetMaze();

    int tile_height = wallBitmap->GetHeight();
    int tile_width = wallBitmap->GetWidth();
    RECT rcBounds = { 0, 0, 4000, 4000 }; // or based on maze size

    for (size_t y = 0; y < mazeArray.size(); ++y) {
        for (size_t x = 0; x < mazeArray[y].size(); ++x) {

            int posX = x * tile_width;
            int posY = y * tile_height;
            if (mazeArray[y][x] == -1) { // It's a wall
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP, SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
            else {
                AddNonCollidableTile(posX, posY, tileBit); // Instead of creating a Sprite
            }
        }
    }
}

/**
 * @brief Clears old sprites and generates a new level based on the level number.
 * This function builds the visual and physical world from the MazeGenerator data.
 * @param level The level number to generate.
 */
void GenerateLevel(int level) {
    // 1. (Optional but Recommended) Clean up sprites from the previous level.
    // CleanupLevel(); // You would write this function to delete old wall/item sprites.

    // 2. Generate the logical layout for the new level
    mazeGenerator->SetupLevel(level);
    const auto& mazeArray = mazeGenerator->GetMaze();

    // 3. Get tile dimensions (assuming all tiles are the same size)
    if (wallBitmap == nullptr || floorBitmap == nullptr) {
        // Add a safety check to prevent crashes if bitmaps aren't loaded
        game_engine->ErrorQuit(TEXT("Essential bitmaps (wall/floor) are not loaded!"));
        return;
    }
    int tile_width = wallBitmap->GetWidth();
    int tile_height = wallBitmap->GetHeight();
    TILE_SIZE = tile_width; // Update global TILE_SIZE if needed

    // Define the bounding box for all sprites
    // This should be large enough to contain the entire maze
    RECT rcBounds = { 0, 0, mazeArray[0].size() * tile_width, mazeArray.size() * tile_height };

    // 4. Iterate through the maze data and create the game world
    for (size_t y = 0; y < mazeArray.size(); ++y) {
        for (size_t x = 0; x < mazeArray[y].size(); ++x) {
            int posX = x * tile_width;
            int posY = y * tile_height;
            POINT pos = { posX, posY };

            // Use the integer value from the maze array
            int tileValue = mazeArray[y][x];

            // Use a switch to handle each type of tile
            switch (static_cast<TileType>(tileValue)) {
            case TileType::WALL: {
                // Create a collidable wall sprite
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP,SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
                break;
            }

            case TileType::KEY: {
                // Create a collidable key sprite
                Sprite* key = new Sprite(keyBitmap, rcBounds, BA_STOP);
                key->SetPosition(pos);
                // To identify this sprite on collision, you might set a unique ID or use a subclass
                // For now, the game logic can check the bitmap, but that's not ideal.
                game_engine->AddSprite(key);
                // Also draw a floor tile underneath the key
                AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }

            case TileType::HEALTH_PACK: {
                Sprite* item = new Sprite(healthPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::ARMOR_PACK: {
                Sprite* item = new Sprite(armorPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::WEAPON_AMMO: {
                Sprite* item = new Sprite(ammoPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::EXTRA_SCORE: {
                Sprite* item = new Sprite(pointPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            //case TileType::SECOND_WEAPON: {
            //    Sprite* item = new Sprite(secondWeaponBitmap, rcBounds, BA_STOP);
            //    item->SetPosition(pos);
            //    game_engine->AddSprite(item);
            //    AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
            //    break;
            //}

            case TileType::END_POINT: {
                // Create a collidable end point sprite
                Sprite* endPoint = new Sprite(endPointBitmap, rcBounds, BA_STOP);
                endPoint->SetPosition(pos);
                game_engine->AddSprite(endPoint);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::START_POINT:
            case TileType::PATH:
            default:
                // For paths and the start point, just draw a non-collidable floor tile
                AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
        }
    }

    // 5. After generating the level, place the player at the start position
    std::pair<int, int> startPosCoords = mazeGenerator->GetStartPos();
    if (charSprite != nullptr && startPosCoords.first != -1) {
        charSprite->SetPosition(startPosCoords.first * tile_width, startPosCoords.second * tile_height);
    }
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap) {
    nonCollidableTiles.push_back({ x, y, bitmap });
}

void CenterCameraOnSprite(Sprite* sprite) {
    if (!sprite || !camera) return;
    RECT pos = sprite->GetPosition();
    int spriteWidth = sprite->GetWidth();
    int spriteHeight = sprite->GetHeight();

    int centerX = pos.left + spriteWidth / 2;
    int centerY = pos.top + spriteHeight / 2;

    int camX = centerX - camera->width / 2;
    int camY = centerY - camera->height / 2;

    camera->SetPosition(camX, camY);
}

void LoadBitmaps(HDC hDC) {

    floorBitmap = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    charBitmap = new Bitmap(hDC, IDB_BITMAP3, instance);
    _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, instance);
    _pEnemyMissileBitmap = new Bitmap(hDC, IDB_BMISSILE, instance);
    healthPWBitmap = new Bitmap(hDC, "Health.bmp");
    ammoPWBitmap = new Bitmap(hDC, "Ammo.bmp");
    pointPWBitmap = new Bitmap(hDC, "Point.bmp");
    armorPWBitmap = new Bitmap(hDC, "Armor.bmp");
    keyBitmap = new Bitmap(hDC, "Key.bmp");
    endPointBitmap = new Bitmap(hDC, "Gate.bmp");
}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    // Çarpışan spritelardan birinin player olup olmadığını anla
    Player* pPlayer = nullptr;
    Sprite* pOther = nullptr;

    // Çarpýþan spritelarýn tiplerini al
    SpriteType hitterType = pSpriteHitter->GetType();
    SpriteType hitteeType = pSpriteHittee->GetType();

    // --- YENÝ KURAL: AYNI TÝP MERMÝLER ÇARPIÞMAZ ---
    // Bu kural, shotgun gibi ayný anda birden fazla mermi ateþlendiðinde
    // mermilerin birbirine takýlmasýný önler.
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        // Hiçbir þey yapma, bu bir çarpýþma deðil.
        return FALSE;
    }
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        // Düþman mermileri de kendi aralarýnda çarpýþmasýn.
        return FALSE;
    }
    // --------------------------------------------------


    // --- KURAL: MERMÝ vs MERMÝ ÇARPIÞMASI ---
    if ((hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) ||
        (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE))
    {
        pSpriteHitter->Kill();
        pSpriteHittee->Kill();
        return FALSE;
    }


    // --- OYUNCU MERMÝSÝ ÝLE ÝLGÝLÝ ÇARPIÞMALAR ---
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE || hitteeType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        Sprite* missile = (hitterType == SPRITE_TYPE_PLAYER_MISSILE) ? pSpriteHitter : pSpriteHittee;
        Sprite* other = (hitterType == SPRITE_TYPE_PLAYER_MISSILE) ? pSpriteHittee : pSpriteHitter;

        if (other->GetType() == SPRITE_TYPE_WALL)
        {
            missile->Kill();
            return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_ENEMY)
        {
            missile->Kill();
            other->Kill();
            return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_PLAYER)
        {
            return FALSE;
        }
    }

    // --- DÜÞMAN MERMÝSÝ ÝLE ÝLGÝLÝ ÇARPIÞMALAR ---
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE || hitteeType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        Sprite* missile = (hitterType == SPRITE_TYPE_ENEMY_MISSILE) ? pSpriteHitter : pSpriteHittee;
        Sprite* other = (hitterType == SPRITE_TYPE_ENEMY_MISSILE) ? pSpriteHittee : pSpriteHitter;

        if (other->GetType() == SPRITE_TYPE_WALL)
        {
            missile->Kill();
            return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_PLAYER)
        {
            missile->Kill();
            other->Kill();
            return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_ENEMY)
        {
            return FALSE;
        }
    }


    // --- VARSAYILAN DAVRANIÞ ---

    if (pSpriteHitter == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHitter);
        pOther = pSpriteHittee;
    }
    else if (pSpriteHittee == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHittee);
        pOther = pSpriteHitter;
    }
    else {
        // Çarpışanlardan hiçbiri player değil, bu fonksiyonda ilgilenmiyoruz.
        // (Örn: Düşman mermisi duvara çarparsa)
        return FALSE;
    }

    // Player bir şeyle çarpıştı, neyle çarpıştığını bitmap'inden anla
    Bitmap* pOtherBitmap = pOther->GetBitmap();

    // 1. Anahtar ile çarpışma
    if (pOtherBitmap == keyBitmap)
    {
        // PlaySound(...); // Anahtar alma sesi
        pPlayer->AddKey();
        pOther->Kill(); // Anahtarı haritadan sil
        // UI'da anahtar sayısını güncelle...
    }
    // 2. Can paketi ile çarpışma
    else if (pOtherBitmap == healthPWBitmap)
    {
        pPlayer->AddHealth(20);
        pOther->Kill();
    }
    // 3. Zırh paketi ile çarpışma
    else if (pOtherBitmap == armorPWBitmap)
    {
        pPlayer->AddArmor(20);
        pOther->Kill();
    }
    // 4. Puan ile çarpışma
    else if (pOtherBitmap == pointPWBitmap)
    {
        pPlayer->AddScore(50);
        pOther->Kill();
    }
    // 5. İkinci silah ile çarpışma
    else if (pOtherBitmap == secondWeaponBitmap)
    {
        pPlayer->GiveSecondWeapon();
        pOther->Kill();
    }
    // 6. Mermi ile çarpışma (sadece ikinci silah varsa işe yarar)
    else if (pOtherBitmap == ammoPWBitmap)
    {
        if (pPlayer->HasSecondWeapon()) {
            pPlayer->AddSecondaryAmmo(10);
            pOther->Kill();
        }
        // İkinci silah yoksa, mermiyi alamaz, sprite silinmez.
    }
    // 7. Bitiş noktası (Gate) ile çarpışma
    else if (pOtherBitmap == endPointBitmap)
    {
        // Seviyeyi bitirmek için gereken anahtar sayısını hesapla
        // (Tasarımımıza göre Seviye 1'de 1, 2'de 2, ... 4 ve sonrasında 4 anahtar)
        int requiredKeys = min(4, currentLevel);

        if (pPlayer->GetKeys() >= requiredKeys)
        {
            // Yeterli anahtar var! Seviyeyi bitir.
            // PlaySound(...); // Seviye tamamlama sesi
            isLevelFinished = true;
        }
        // Yeterli anahtar yoksa hiçbir şey yapma, kapı kapalı kalır.
    }

    // Bu fonksiyonun TRUE veya FALSE dönmesi, spriteların birbirinin
    // içinden geçip geçemeyeceğini belirler. Genelde item'lar için FALSE
    // dönmek daha mantıklıdır, böylece oyuncu item'ın üzerinden geçebilir.
    return FALSE;
}

// Yeni seviye oluşturacak olan yardımcı fonksiyon
void OnLevelComplete() {
    currentLevel++;

    // UI gösterimi veya bekleme süresi
    // Sleep(3000); // 3 saniye bekle

    CleanupLevel(); // Mevcut haritadaki duvarları, item'ları vb. temizle
    GenerateLevel(currentLevel); // Yeni seviyeyi oluştur
}

// Bu fonksiyon, player hariç tüm spriteları temizlemeli
void CleanupLevel() {
    // nonCollidableTiles'ı temizle
    nonCollidableTiles.clear();
    // 1. Oyuncuyu motorun listesinden geçici olarak çıkar (ama silme!)
    game_engine->RemoveSprite(charSprite);

    // 2. Şimdi listede oyuncu olmadığı için, kalan her şeyi güvenle silebiliriz.
    game_engine->CleanupSprites();

    // 3. Oyuncuyu temizlenmiş listeye geri ekle.
    game_engine->AddSprite(charSprite);
}