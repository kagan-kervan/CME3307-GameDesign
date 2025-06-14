// Game.cpp

#include <string>
#include "Game.h"
#include <vector>
#include "Enemy.h"
#include <random>
#include <limits> // std::numeric_limits için eklendi
#include <cmath>  // sqrt, pow için eklendi (SpawnEnemyNearClosest içinde kullanılabilir)
#undef max
//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0,0,4000,4000 };

// Düşman spawn zamanlayıcıları ve intervalleri
DWORD g_dwLastSpawnTime = 0;
const DWORD ENEMY_SPAWN_INTERVAL = 15000; // 15 saniye (milisaniye cinsinden) - DEĞİŞTİRİLDİ

DWORD g_dwLastClosestEnemySpawnTime = 0; // YENİ: En yakın düşmandan spawn için zamanlayıcı
const DWORD CLOSEST_ENEMY_SPAWN_INTERVAL = 6000; // 6 saniye (milisaniye cinsinden) - YENİ

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
extern RECT globalBounds;
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

Camera::Camera(Sprite* target, int w, int h)
    : m_pTarget(target), width(w), height(h), m_fLerpFactor(0.08f)
{
    if (m_pTarget)
    {
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

void Camera::Update()
{
    if (!m_pTarget) return;

    RECT pos = m_pTarget->GetPosition();
    float targetX = (float)(pos.left + m_pTarget->GetWidth() / 2 - width / 2);
    float targetY = (float)(pos.top + m_pTarget->GetHeight() / 2 - height / 2);

    m_fCurrentX += (targetX - m_fCurrentX) * m_fLerpFactor;
    m_fCurrentY += (targetY - m_fCurrentY) * m_fLerpFactor;

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
    TILE_SIZE = wallBitmap->GetHeight(); // TILE_SIZE burada atanıyor
    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    mazeGenerator = new MazeGenerator(15, 15);
    charSprite = new Player(charBitmap, mazeGenerator);
    currentLevel = 1;
    GenerateLevel(currentLevel); // TILE_SIZE atandıktan sonra çağrılıyor
    game_engine->AddSprite(charSprite);

    camera = new Camera(charSprite, window_X, window_Y);
    fovEffect = new FOVBackground(charSprite, 90, 350, 75);


    for (int i = 0; i < 5; i++)
    {
        EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);
        int ex, ey;
        do {
            ex = (rand() % (mazeGenerator->GetMaze()[0].size())); // Labirent genişliğine göre
            ey = (rand() % (mazeGenerator->GetMaze().size()));    // Labirent yüksekliğine göre
        } while (mazeGenerator->IsWall(ex, ey));

        pEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}

void GameEnd()
{
    game_engine->CloseMIDIPlayer();
    DeleteObject(offscreenBitmap);
    DeleteDC(offscreenDC);
    delete _pEnemyMissileBitmap;
    delete background;
    game_engine->CleanupSprites();
    delete game_engine;

    // Diğer bitmap'lerin de silinmesi gerekiyor
    delete wallBitmap;
    delete charBitmap;
    delete _pEnemyBitmap;
    delete healthPWBitmap;
    delete ammoPWBitmap;
    delete pointPWBitmap;
    delete armorPWBitmap;
    delete floorBitmap;
    delete keyBitmap;
    delete endPointBitmap;
    delete _pPlayerMissileBitmap;
    // secondWeaponBitmap null değilse silinmeli
    // if (secondWeaponBitmap) delete secondWeaponBitmap;
    delete mazeGenerator;
    delete camera;
    delete fovEffect;
}

void GameActivate(HWND hWindow)
{
}

void GameDeactivate(HWND hWindow)
{
}

void GamePaint(HDC hDC)
{
    background->Draw(hDC, 0, 0);

    for (const auto& tile : nonCollidableTiles) {
        // tile.bitmap null değilse çiz
        if (tile.bitmap)
            tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    // Draw all sprites with camera offset
    // game_engine->GetSprites() çağrısı bir kopya döndürmemeli, referans olmalı.
    // GameEngine sınıfında GetSprites() const std::vector<Sprite*>& GetSprites() const; olmalı ya da
    // for (Sprite* sprite : game_engine->GetSprites()) yerine
    // const auto& sprites = game_engine->GetSprites(); for (Sprite* sprite : sprites)
    for (Sprite* sprite : game_engine->GetSprites()) { // Bu satırda GetSprites() çağrılıyor
        if (sprite) // Sprite null değilse çiz
            sprite->Draw(hDC, camera->x, camera->y);
    }


    if (fovEffect) // fovEffect null değilse çiz
        fovEffect->Draw(hDC, camera->x, camera->y);
}

void GameCycle()
{
    if (camera) {
        camera->Update();
    }

    if (background) background->Update();
    if (game_engine) game_engine->UpdateSprites();

    // Düşman spawn etme mantığı - Rastgele spawn
    if (GetTickCount() > g_dwLastSpawnTime + ENEMY_SPAWN_INTERVAL)
    {
        SpawnEnemyNearPlayer();
        g_dwLastSpawnTime = GetTickCount();
    }

    // YENİ: Oyuncuya en yakın düşmandan spawn etme mantığı
    if (GetTickCount() > g_dwLastClosestEnemySpawnTime + CLOSEST_ENEMY_SPAWN_INTERVAL)
    {
        SpawnEnemyNearClosest();
        g_dwLastClosestEnemySpawnTime = GetTickCount();
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

    GamePaint(offscreenDC);

    BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
        offscreenDC, 0, 0, SRCCOPY);

    ReleaseDC(hWindow, hDC);
}

void SpawnEnemyNearPlayer()
{
    if (!mazeGenerator || !charSprite || !_pEnemyBitmap || !game_engine || TILE_SIZE == 0) return;

    RECT playerPos = charSprite->GetPosition();
    int playerTileX = playerPos.left / TILE_SIZE;
    int playerTileY = playerPos.top / TILE_SIZE;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(-5, 5); // Oyuncunun +/- 5 tile uzağında bir yer

    int spawnTileX, spawnTileY;
    int tryCount = 0;
    const int maxTries = 20;

    const auto& mazeData = mazeGenerator->GetMaze();
    if (mazeData.empty() || mazeData[0].empty()) return; // Labirent verisi yoksa çık
    int mazeWidthInTiles = mazeData[0].size();
    int mazeHeightInTiles = mazeData.size();

    do {
        spawnTileX = playerTileX + distr(gen);
        spawnTileY = playerTileY + distr(gen);
        tryCount++;

        // Labirent sınırları içinde mi kontrol et
        if (spawnTileX < 0 || spawnTileX >= mazeWidthInTiles ||
            spawnTileY < 0 || spawnTileY >= mazeHeightInTiles) {
            continue; // Sınır dışındaysa yeni koordinat üret
        }
    } while (mazeGenerator->IsWall(spawnTileX, spawnTileY) && tryCount < maxTries);

    // Geçerli bir yer bulunduysa ve duvar değilse düşmanı oluştur
    if (tryCount < maxTries && !mazeGenerator->IsWall(spawnTileX, spawnTileY))
    {
        EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);

        pEnemy->SetPosition(spawnTileX * TILE_SIZE, spawnTileY * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}

// YENİ FONKSİYON: Oyuncuya en yakın düşmandan yeni bir düşman spawn eder
void SpawnEnemyNearClosest()
{
    if (!mazeGenerator || !charSprite || !_pEnemyBitmap || !game_engine || game_engine->GetSprites().empty() || TILE_SIZE == 0) return;

    Enemy* closestEnemy = nullptr;
    float minDistanceSq = std::numeric_limits<float>::max();

    RECT playerRect = charSprite->GetPosition();
    float playerCenterX = static_cast<float>(playerRect.left + charSprite->GetWidth() / 2.0f);
    float playerCenterY = static_cast<float>(playerRect.top + charSprite->GetHeight() / 2.0f);

    for (Sprite* sprite : game_engine->GetSprites())
    {
        if (sprite && sprite->GetType() == SPRITE_TYPE_ENEMY) // sprite null kontrolü eklendi
        {
            Enemy* currentEnemy = static_cast<Enemy*>(sprite);
            RECT enemyRect = currentEnemy->GetPosition();
            float enemyCenterX = static_cast<float>(enemyRect.left + currentEnemy->GetWidth() / 2.0f);
            float enemyCenterY = static_cast<float>(enemyRect.top + currentEnemy->GetHeight() / 2.0f);

            float dx = playerCenterX - enemyCenterX;
            float dy = playerCenterY - enemyCenterY;
            float distanceSq = dx * dx + dy * dy;

            if (distanceSq < minDistanceSq)
            {
                minDistanceSq = distanceSq;
                closestEnemy = currentEnemy;
            }
        }
    }

    if (closestEnemy)
    {
        RECT enemyPos = closestEnemy->GetPosition();
        int enemyTileX = enemyPos.left / TILE_SIZE;
        int enemyTileY = enemyPos.top / TILE_SIZE;

        int spawnTileX = enemyTileX;
        int spawnTileY = enemyTileY;

        const auto& mazeData = mazeGenerator->GetMaze();
        if (mazeData.empty() || mazeData[0].empty()) return;
        int mazeWidthInTiles = mazeData[0].size();
        int mazeHeightInTiles = mazeData.size();

        if (spawnTileX >= 0 && spawnTileX < mazeWidthInTiles &&
            spawnTileY >= 0 && spawnTileY < mazeHeightInTiles &&
            !mazeGenerator->IsWall(spawnTileX, spawnTileY))
        {
            EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
            Enemy* newEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
                mazeGenerator, charSprite, type);
            newEnemy->SetPosition(spawnTileX * TILE_SIZE, spawnTileY * TILE_SIZE);
            game_engine->AddSprite(newEnemy);
        }
    }
}

void HandleKeys()
{
    // Player sınıfı kendi içinde HandleInput ile bunu yönetiyor
}

void MouseButtonDown(int x, int y, BOOL bLeft)
{
    if (bLeft && camera && charSprite) // Null kontrolleri
    {
        int targetWorldX = x + camera->x;
        int targetWorldY = y + camera->y;

        Player* pPlayer = static_cast<Player*>(charSprite);
        if (pPlayer) // pPlayer null değilse Fire çağır
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

void GenerateMaze(Bitmap* tileBit) { // tileBit parametresi artık kullanılmıyor gibi, GenerateLevel'a bakılmalı
    if (!mazeGenerator || !wallBitmap || !game_engine || TILE_SIZE == 0) return;

    mazeGenerator->generateMaze(); // Bu fonksiyon artık sadece duvar ve yol oluşturuyor olmalı.
    // Item yerleşimi SetupLevel içinde.
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->GetMaze();
    if (mazeArray.empty() || mazeArray[0].empty()) return;


    int tile_width = TILE_SIZE;  // wallBitmap->GetWidth() yerine TILE_SIZE kullanıldı
    int tile_height = TILE_SIZE; // wallBitmap->GetHeight() yerine TILE_SIZE kullanıldı
    RECT rcBounds = { 0, 0, static_cast<long>(mazeArray[0].size() * tile_width), static_cast<long>(mazeArray.size() * tile_height) };


    for (size_t y = 0; y < mazeArray.size(); ++y) {
        for (size_t x = 0; x < mazeArray[y].size(); ++x) {
            int posX = x * tile_width;
            int posY = y * tile_height;
            if (mazeArray[y][x] == static_cast<int>(TileType::WALL)) { // TileType enum'u ile karşılaştır
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP, SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
            // Diğer tile'lar (yol, item vb.) GenerateLevel içinde ele alınıyor.
            // Bu fonksiyon artık sadece duvarları eklemeli veya hiç kullanılmamalı,
            // çünkü GenerateLevel tüm seviye oluşturma mantığını içeriyor.
            // Şimdilik, sadece duvar ekleme kısmını bırakıyorum.
            // else {
            //    AddNonCollidableTile(posX, posY, floorBitmap); // tileBit yerine floorBitmap
            // }
        }
    }
}

void GenerateLevel(int level) {
    if (!mazeGenerator || !game_engine || !wallBitmap || !floorBitmap || TILE_SIZE == 0) {
        if (game_engine) game_engine->ErrorQuit(TEXT("Essential resources for level generation are missing!"));
        return;
    }

    // Önceki seviyeden kalanları temizle (eğer bu ilk seviye değilse veya yeniden oluşturuluyorsa)
    // CleanupLevel(); // Bu çağrı, oyuncu ve diğer kalıcı spriteların korunmasını sağlamalı.

    mazeGenerator->SetupLevel(level);
    const auto& mazeArray = mazeGenerator->GetMaze();
    if (mazeArray.empty() || mazeArray[0].empty()) {
        game_engine->ErrorQuit(TEXT("Maze data is empty after SetupLevel!"));
        return;
    }

    int tile_width = TILE_SIZE;
    int tile_height = TILE_SIZE;

    RECT rcBounds = { 0, 0, static_cast<long>(mazeArray[0].size() * tile_width), static_cast<long>(mazeArray.size() * tile_height) };

    for (size_t y = 0; y < mazeArray.size(); ++y) {
        for (size_t x = 0; x < mazeArray[y].size(); ++x) {
            int posX = x * tile_width;
            int posY = y * tile_height;
            POINT pos = { posX, posY };
            int tileValue = mazeArray[y][x];

            switch (static_cast<TileType>(tileValue)) {
            case TileType::WALL: {
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP, SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
                break;
            }
            case TileType::KEY: {
                if (!keyBitmap) break; // Bitmap yoksa atla
                Sprite* key = new Sprite(keyBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC); // Tip önemli olabilir
                key->SetPosition(pos);
                game_engine->AddSprite(key);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
            case TileType::HEALTH_PACK: {
                if (!healthPWBitmap) break;
                Sprite* item = new Sprite(healthPWBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
            case TileType::ARMOR_PACK: {
                if (!armorPWBitmap) break;
                Sprite* item = new Sprite(armorPWBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
            case TileType::WEAPON_AMMO: {
                if (!ammoPWBitmap) break;
                Sprite* item = new Sprite(ammoPWBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
            case TileType::EXTRA_SCORE: {
                if (!pointPWBitmap) break;
                Sprite* item = new Sprite(pointPWBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
            case TileType::END_POINT: {
                if (!endPointBitmap) break;
                Sprite* endPoint = new Sprite(endPointBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                endPoint->SetPosition(pos);
                game_engine->AddSprite(endPoint);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
            case TileType::START_POINT:
            case TileType::PATH:
            default:
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
        }
    }

    std::pair<int, int> startPosCoords = mazeGenerator->GetStartPos();
    if (charSprite != nullptr && startPosCoords.first != -1 && TILE_SIZE > 0) {
        charSprite->SetPosition(startPosCoords.first * tile_width, startPosCoords.second * tile_height);
    }
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap) {
    if (bitmap) // bitmap null değilse ekle
        nonCollidableTiles.push_back({ x, y, bitmap });
}

// CenterCameraOnSprite fonksiyonu artık Camera sınıfının Update metodu ile yönetiliyor.
// Bu global fonksiyon kaldırılabilir veya kullanılmamalıdır.
/*
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
*/

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
    // secondWeaponBitmap yüklemesi MazeGenerator'da yorumlandığı için burada da kaldırılabilir veya eklenebilir.
}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    if (!pSpriteHitter || !pSpriteHittee) return FALSE; // Null kontrolü

    Player* pPlayer = nullptr;
    Sprite* pOther = nullptr;

    SpriteType hitterType = pSpriteHitter->GetType();
    SpriteType hitteeType = pSpriteHittee->GetType();

    // --- DÜŞMANLAR BİRBİRİYLE ÇARPIŞMAZ ---
    if (hitterType == SPRITE_TYPE_ENEMY && hitteeType == SPRITE_TYPE_ENEMY)
    {
        return FALSE; // Düşmanlar birbirinin üzerinden geçebilir
    }
    // ------------------------------------

    // Mermilerin kendi aralarında çarpışmaması
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE) return FALSE;
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) return FALSE;

    // Oyuncu mermisi ile düşman mermisi çarpışması
    if ((hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) ||
        (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE))
    {
        pSpriteHitter->Kill();
        pSpriteHittee->Kill();
        return FALSE;
    }

    // Oyuncu mermisi ile ilgili çarpışmalar
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE || hitteeType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        Sprite* missile = (hitterType == SPRITE_TYPE_PLAYER_MISSILE) ? pSpriteHitter : pSpriteHittee;
        Sprite* other = (hitterType == SPRITE_TYPE_PLAYER_MISSILE) ? pSpriteHittee : pSpriteHitter;

        if (other->GetType() == SPRITE_TYPE_WALL) {
            missile->Kill(); return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_ENEMY) {
            missile->Kill();
            other->Kill(); // Düşmanı öldür
            if (charSprite) static_cast<Player*>(charSprite)->AddScore(10); // Düşman öldürme skoru
            return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_PLAYER) return FALSE; // Oyuncu kendi mermisiyle çarpışmaz
    }

    // Düşman mermisi ile ilgili çarpışmalar
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE || hitteeType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        Sprite* missile = (hitterType == SPRITE_TYPE_ENEMY_MISSILE) ? pSpriteHitter : pSpriteHittee;
        Sprite* other = (hitterType == SPRITE_TYPE_ENEMY_MISSILE) ? pSpriteHittee : pSpriteHitter;

        if (other->GetType() == SPRITE_TYPE_WALL) {
            missile->Kill(); return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_PLAYER) {
            missile->Kill();
            if (charSprite == other) { // Emin olalım ki other gerçekten player sprite'ı
                Player* playerPtr = static_cast<Player*>(other);
                // playerPtr->TakeDamage(10); // Gerçek hasar mekanizması burada olmalı
                // Şimdilik sadece mesaj, can azaltma Player sınıfında ele alınmalı
                // OutputDebugString(L"Player hit by enemy missile!\n");
            }
            return FALSE;
        }
        if (other->GetType() == SPRITE_TYPE_ENEMY) return FALSE; // Düşman kendi mermisiyle çarpışmaz
    }


    // Çarpışanlardan biri oyuncu mu?
    if (pSpriteHitter == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHitter);
        pOther = pSpriteHittee;
    }
    else if (pSpriteHittee == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHittee);
        pOther = pSpriteHitter;
    }
    else {
        // Çarpışanlardan hiçbiri oyuncu değilse ve yukarıdaki özel durumlar da değilse
        // (örneğin iki duvar - normalde olmaz, iki item - normalde olmaz)
        // genel bir kural olarak FALSE dönelim.
        return FALSE;
    }

    // Player bir şeyle çarpıştı
    if (!pPlayer || !pOther) return FALSE;

    Bitmap* pOtherBitmap = pOther->GetBitmap(); // pOther null değilse bitmap al

    // Item'larla çarpışma
    if (pOtherBitmap == keyBitmap) { pPlayer->AddKey(); pOther->Kill(); return FALSE; }
    if (pOtherBitmap == healthPWBitmap) { pPlayer->AddHealth(20); pOther->Kill(); return FALSE; }
    if (pOtherBitmap == armorPWBitmap) { pPlayer->AddArmor(20); pOther->Kill(); return FALSE; }
    if (pOtherBitmap == pointPWBitmap) { pPlayer->AddScore(50); pOther->Kill(); return FALSE; }
    if (pOtherBitmap == ammoPWBitmap) { pPlayer->AddSecondaryAmmo(10); pOther->Kill(); return FALSE; }
    if (pOtherBitmap == endPointBitmap) {
        int requiredKeys = min(4, currentLevel);
        if (pPlayer->GetKeys() >= requiredKeys) {
            isLevelFinished = true;
        }
        return FALSE; // Bitiş noktasının üzerinden geçilebilir
    }

    // Oyuncu ile düşman çarpışması
    if (pOther->GetType() == SPRITE_TYPE_ENEMY)
    {
        // Oyuncu düşmanla temas ederse hasar alabilir veya geri itilebilir.
        // Şimdilik sadece TRUE dönerek birbirlerinin içinden geçmelerini engelleyelim.
        // Player* playerPtr = static_cast<Player*>(pPlayer);
        // playerPtr->TakeDamage(5); // Temas hasarı
        return TRUE;
    }

    // Oyuncu ile duvar çarpışması
    if (pOther->GetType() == SPRITE_TYPE_WALL)
    {
        return TRUE; // Duvarın içinden geçilemez
    }

    return FALSE; // Varsayılan olarak diğer çarpışmalar engellenmez (itemlar vb.)
}

void OnLevelComplete() {
    currentLevel++;
    // İsteğe bağlı: Seviye geçiş ekranı, ses vb.
    // Sleep(2000); // Kısa bir bekleme

    CleanupLevel();
    GenerateLevel(currentLevel);
    // Yeni seviyede düşman spawn zamanlayıcılarını sıfırla
    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
}

void CleanupLevel() {
    if (!game_engine) return;

    nonCollidableTiles.clear();

    // Oyuncuyu listeden geçici olarak çıkar (silme!)
    if (charSprite) game_engine->RemoveSprite(charSprite);

    // Kalan tüm spriteları (duvarlar, itemlar, düşmanlar vb.) temizle
    game_engine->CleanupSprites();

    // Oyuncuyu temizlenmiş listeye geri ekle
    if (charSprite) game_engine->AddSprite(charSprite);
}