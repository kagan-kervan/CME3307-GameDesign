// Game.cpp

#include <string>
#include "Game.h"
#include <vector>
#include "Enemy.h"
#include <random>
#include <limits> // std::numeric_limits için eklendi
#include <cmath>  // sqrt, pow için eklendi
#include <algorithm>
#undef max // Windows.h'daki max ile std::max çakışmasını önlemek için
#undef min
//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0,0,4000,4000 };

DWORD g_dwLastSpawnTime = 0;
const DWORD ENEMY_SPAWN_INTERVAL = 7000;

DWORD g_dwLastClosestEnemySpawnTime = 0;
const DWORD CLOSEST_ENEMY_SPAWN_INTERVAL = 3000;

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
// extern RECT globalBounds; // Zaten yukarıda tanımlı
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

// YARDIMCI FONKSİYON: Belirtilen bir RECT alanının labirentte tamamen boş olup olmadığını kontrol eder
// Düşmanın tüm köşelerinin duvar içinde olmamasını sağlar.
bool IsAreaClearForSpawn(int tileX, int tileY, int spriteWidthInTiles, int spriteHeightInTiles) {
    if (!mazeGenerator || TILE_SIZE == 0) return false;

    // Düşmanın kaplayacağı tüm tile'ları kontrol et
    for (int y = 0; y < spriteHeightInTiles; ++y) {
        for (int x = 0; x < spriteWidthInTiles; ++x) {
            if (mazeGenerator->IsWall(tileX + x, tileY + y)) {
                return false; // Herhangi bir kısmı duvardaysa, alan temiz değil
            }
        }
    }
    return true; // Tüm alan boş
}


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
        m_fCurrentX = static_cast<float>(pos.left + m_pTarget->GetWidth() / 2 - width / 2);
        m_fCurrentY = static_cast<float>(pos.top + m_pTarget->GetHeight() / 2 - height / 2);
        x = static_cast<int>(m_fCurrentX);
        y = static_cast<int>(m_fCurrentY);
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
    float targetX = static_cast<float>(pos.left + m_pTarget->GetWidth() / 2 - width / 2);
    float targetY = static_cast<float>(pos.top + m_pTarget->GetHeight() / 2 - height / 2);

    m_fCurrentX += (targetX - m_fCurrentX) * m_fLerpFactor;
    m_fCurrentY += (targetY - m_fCurrentY) * m_fLerpFactor;

    x = static_cast<int>(round(m_fCurrentX));
    y = static_cast<int>(round(m_fCurrentY));
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
    if (!_pPlayerMissileBitmap && instance) // instance null değilse yükle
        _pPlayerMissileBitmap = new Bitmap(hDC, IDB_MISSILE, instance);

    if (wallBitmap) TILE_SIZE = wallBitmap->GetHeight(); else TILE_SIZE = 50; // wallBitmap yoksa varsayılan

    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    mazeGenerator = new MazeGenerator(15, 15); // Labirent boyutu

    if (charBitmap && mazeGenerator) // Null kontrolü
        charSprite = new Player(charBitmap, mazeGenerator);

    currentLevel = 1;
    GenerateLevel(currentLevel);

    if (game_engine && charSprite) // Null kontrolü
        game_engine->AddSprite(charSprite);

    if (charSprite) // Null kontrolü
        camera = new Camera(charSprite, window_X, window_Y);

    if (charSprite) // Null kontrolü
        fovEffect = new FOVBackground(charSprite, 90, 350, 75);


    // Başlangıç düşmanları
    if (mazeGenerator && _pEnemyBitmap && charSprite && game_engine && TILE_SIZE > 0) {
        const auto& mazeData = mazeGenerator->GetMaze();
        if (!mazeData.empty() && !mazeData[0].empty()) {
            int enemySpriteWidthInTiles = (_pEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
            int enemySpriteHeightInTiles = (_pEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

            for (int i = 0; i < 5; i++)
            {
                EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;
                Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
                    mazeGenerator, charSprite, type);
                int ex, ey;
                int tryCount = 0;
                const int maxSpawnTries = 50; // Daha fazla deneme şansı
                do {
                    ex = (rand() % (mazeData[0].size() - enemySpriteWidthInTiles + 1)); // Sprite'ın taşmamasını sağla
                    ey = (rand() % (mazeData.size() - enemySpriteHeightInTiles + 1));    // Sprite'ın taşmamasını sağla
                    tryCount++;
                } while (!IsAreaClearForSpawn(ex, ey, enemySpriteWidthInTiles, enemySpriteHeightInTiles) && tryCount < maxSpawnTries);

                if (tryCount < maxSpawnTries) { // Geçerli bir yer bulunduysa
                    pEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE);
                    game_engine->AddSprite(pEnemy);
                }
                else {
                    delete pEnemy; // Yer bulunamadıysa düşmanı sil
                }
            }
        }
    }
}

void GameEnd()
{
    if (game_engine) game_engine->CloseMIDIPlayer();

    if (offscreenBitmap) DeleteObject(offscreenBitmap);
    if (offscreenDC) DeleteDC(offscreenDC);

    delete _pEnemyMissileBitmap; _pEnemyMissileBitmap = nullptr;
    delete background; background = nullptr;

    if (game_engine) {
        game_engine->CleanupSprites(); // Bu, charSprite dahil tüm spriteları siler
        charSprite = nullptr; // charSprite artık GameEngine tarafından yönetiliyor ve silindi
        delete game_engine; game_engine = nullptr;
    }


    delete wallBitmap; wallBitmap = nullptr;
    delete charBitmap; charBitmap = nullptr; // charSprite silindiği için bu da silinebilir
    delete _pEnemyBitmap; _pEnemyBitmap = nullptr;
    delete healthPWBitmap; healthPWBitmap = nullptr;
    delete ammoPWBitmap; ammoPWBitmap = nullptr;
    delete pointPWBitmap; pointPWBitmap = nullptr;
    delete armorPWBitmap; armorPWBitmap = nullptr;
    delete floorBitmap; floorBitmap = nullptr;
    delete keyBitmap; keyBitmap = nullptr;
    delete endPointBitmap; endPointBitmap = nullptr;
    delete _pPlayerMissileBitmap; _pPlayerMissileBitmap = nullptr;
    // if (secondWeaponBitmap) { delete secondWeaponBitmap; secondWeaponBitmap = nullptr; }

    delete mazeGenerator; mazeGenerator = nullptr;
    delete camera; camera = nullptr;
    delete fovEffect; fovEffect = nullptr;
}

void GameActivate(HWND hWindow)
{
}

void GameDeactivate(HWND hWindow)
{
}

void GamePaint(HDC hDC)
{
    if (background && camera) background->Draw(hDC, 0, 0); // Arka plan kameradan bağımsız

    for (const auto& tile : nonCollidableTiles) {
        if (tile.bitmap && camera)
            tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    if (game_engine && camera) {
        for (Sprite* sprite : game_engine->GetSprites()) {
            if (sprite)
                sprite->Draw(hDC, camera->x, camera->y);
        }
    }

    if (fovEffect && camera)
        fovEffect->Draw(hDC, camera->x, camera->y);
}

void GameCycle()
{
    if (camera) {
        camera->Update();
    }

    if (background) background->Update();
    if (game_engine) game_engine->UpdateSprites();

    if (GetTickCount() > g_dwLastSpawnTime + ENEMY_SPAWN_INTERVAL)
    {
        SpawnEnemyNearPlayer();
        g_dwLastSpawnTime = GetTickCount();
    }

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

    if (game_engine) {
        HWND  hWindow = game_engine->GetWindow();
        if (hWindow) { // hWindow null kontrolü
            HDC   hDC = GetDC(hWindow);
            if (hDC) { // hDC null kontrolü
                GamePaint(offscreenDC); // offscreenDC'ye çiz

                BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
                    offscreenDC, 0, 0, SRCCOPY);

                ReleaseDC(hWindow, hDC);
            }
        }
    }
}

void SpawnEnemyNearPlayer()
{
    if (!mazeGenerator || !charSprite || !_pEnemyBitmap || !game_engine || TILE_SIZE == 0) return;

    RECT playerPos = charSprite->GetPosition();
    int playerTileX = playerPos.left / TILE_SIZE;
    int playerTileY = playerPos.top / TILE_SIZE;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(-7, 7); // Arama alanını biraz genişletelim

    int spawnTileX, spawnTileY;
    int tryCount = 0;
    const int maxTries = 30; // Deneme sayısını artıralım

    const auto& mazeData = mazeGenerator->GetMaze();
    if (mazeData.empty() || mazeData[0].empty()) return;
    int mazeWidthInTiles = mazeData[0].size();
    int mazeHeightInTiles = mazeData.size();

    // Düşman sprite'ının tile cinsinden boyutları
    int enemySpriteWidthInTiles = (_pEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE; // Yukarı yuvarla
    int enemySpriteHeightInTiles = (_pEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE; // Yukarı yuvarla


    do {
        spawnTileX = playerTileX + distr(gen);
        spawnTileY = playerTileY + distr(gen);
        tryCount++;

        // Labirent sınırları içinde mi ve sprite taşmıyor mu kontrol et
        if (spawnTileX < 0 || spawnTileX + enemySpriteWidthInTiles > mazeWidthInTiles ||
            spawnTileY < 0 || spawnTileY + enemySpriteHeightInTiles > mazeHeightInTiles) {
            continue;
        }
        // DÜZELTME: IsAreaClearForSpawn çağrısını kullan
    } while (!IsAreaClearForSpawn(spawnTileX, spawnTileY, enemySpriteWidthInTiles, enemySpriteHeightInTiles) && tryCount < maxTries);

    if (tryCount < maxTries) // Geçerli bir yer bulunduysa
    {
        EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);

        pEnemy->SetPosition(spawnTileX * TILE_SIZE, spawnTileY * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}

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
        if (sprite && sprite->GetType() == SPRITE_TYPE_ENEMY)
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

        // Düşman sprite'ının tile cinsinden boyutları
        int enemySpriteWidthInTiles = (_pEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
        int enemySpriteHeightInTiles = (_pEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

        // Spawn konumu olarak en yakın düşmanın mevcut tile'ını kullanıyoruz.
        // Bu tile'ın boş olduğundan emin olmalıyız.
        if (IsAreaClearForSpawn(enemyTileX, enemyTileY, enemySpriteWidthInTiles, enemySpriteHeightInTiles))
        {
            EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
            Enemy* newEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
                mazeGenerator, charSprite, type);
            // Yeni düşmanı, eski düşmanla aynı tile'a yerleştir.
            // Bu, üst üste binmelerine neden olabilir, bu istenmiyorsa farklı bir boş tile aranmalı.
            // Şimdilik aynı yere koyuyoruz, çünkü `SpriteCollision` içinde düşmanların çarpışmadığı varsayılıyor.
            newEnemy->SetPosition(enemyTileX * TILE_SIZE, enemyTileY * TILE_SIZE);
            game_engine->AddSprite(newEnemy);
        }
        // Eğer en yakın düşmanın olduğu yer doluysa (bu pek olası değil ama),
        // alternatif bir spawn mantığı eklenebilir (örneğin etrafındaki boş bir tile'a).
    }
}

void HandleKeys()
{
}

void MouseButtonDown(int x, int y, BOOL bLeft)
{
    if (bLeft && camera && charSprite)
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

// GenerateMaze fonksiyonu büyük ölçüde GenerateLevel tarafından kapsandığı için
// ve item/diğer tile yerleşimlerini yapmadığı için ya basitleştirilmeli ya da kaldırılmalı.
// Şimdilik bırakıyorum ama GenerateLevel ana odak olmalı.
void GenerateMaze(Bitmap* tileBit) {
    if (!mazeGenerator || !wallBitmap || !game_engine || TILE_SIZE == 0) return;

    mazeGenerator->generateMaze();
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->GetMaze();
    if (mazeArray.empty() || mazeArray[0].empty()) return;

    int tile_width = TILE_SIZE;
    int tile_height = TILE_SIZE;
    RECT rcBounds = { 0, 0, static_cast<long>(mazeArray[0].size() * tile_width), static_cast<long>(mazeArray.size() * tile_height) };

    for (size_t r = 0; r < mazeArray.size(); ++r) { // y yerine r (row)
        for (size_t c = 0; c < mazeArray[r].size(); ++c) { // x yerine c (column)
            int posX = c * tile_width;
            int posY = r * tile_height;
            // mazeArray[y][x] yerine mazeArray[r][c]
            if (mazeArray[r][c] == static_cast<int>(TileType::WALL)) {
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP, SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
        }
    }
}

void GenerateLevel(int level) {
    if (!mazeGenerator || !game_engine || !wallBitmap || !floorBitmap || TILE_SIZE == 0) {
        if (game_engine) game_engine->ErrorQuit(TEXT("Essential resources for level generation are missing!"));
        return;
    }

    // Önceki seviyenin spritelarını temizle (oyuncu hariç)
    CleanupLevel(); // Bu fonksiyon oyuncuyu koruyacak şekilde güncellenmeli

    mazeGenerator->SetupLevel(level);
    const auto& mazeArray = mazeGenerator->GetMaze();
    if (mazeArray.empty() || mazeArray[0].empty()) {
        if (game_engine) game_engine->ErrorQuit(TEXT("Maze data is empty after SetupLevel!"));
        return;
    }

    int tile_width = TILE_SIZE;
    int tile_height = TILE_SIZE;

    RECT rcBounds = { 0, 0, static_cast<long>(mazeArray[0].size() * tile_width), static_cast<long>(mazeArray.size() * tile_height) };

    for (size_t r = 0; r < mazeArray.size(); ++r) {
        for (size_t c = 0; c < mazeArray[r].size(); ++c) {
            int posX = c * tile_width;
            int posY = r * tile_height;
            POINT pos = { posX, posY };
            int tileValue = mazeArray[r][c];

            Bitmap* itemBitmap = nullptr; // Geçici bitmap pointer'ı

            switch (static_cast<TileType>(tileValue)) {
            case TileType::WALL: {
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP, SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
                break;
            }
            case TileType::KEY: itemBitmap = keyBitmap; break;
            case TileType::HEALTH_PACK: itemBitmap = healthPWBitmap; break;
            case TileType::ARMOR_PACK: itemBitmap = armorPWBitmap; break;
            case TileType::WEAPON_AMMO: itemBitmap = ammoPWBitmap; break;
            case TileType::EXTRA_SCORE: itemBitmap = pointPWBitmap; break;
            case TileType::END_POINT: itemBitmap = endPointBitmap; break;
                // case TileType::SECOND_WEAPON: itemBitmap = secondWeaponBitmap; break;

            case TileType::START_POINT: // Başlangıç noktasına özel bir şey yapılmıyorsa floor ile aynı
            case TileType::PATH:
            default:
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                continue; // switch'ten sonraki item oluşturma kısmını atla
            }

            // Eğer bir item bitmap'i atandıysa, item sprite'ını oluştur ve zemini çiz
            if (itemBitmap) {
                Sprite* item = new Sprite(itemBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
            }
            else if (static_cast<TileType>(tileValue) != TileType::WALL) {
                // Eğer itemBitmap null ise (örn. bitmap yüklenmemişse) ama duvar da değilse,
                // en azından bir zemin çizelim.
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
            }
        }
    }

    if (charSprite && mazeGenerator) { // Null kontrolleri
        std::pair<int, int> startPosCoords = mazeGenerator->GetStartPos();
        if (startPosCoords.first != -1 && TILE_SIZE > 0) {
            charSprite->SetPosition(startPosCoords.first * tile_width, startPosCoords.second * tile_height);
        }
    }
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap) {
    if (bitmap)
        nonCollidableTiles.push_back({ x, y, bitmap });
}


void LoadBitmaps(HDC hDC) {
    if (!hDC) return; // hDC null ise çık

    floorBitmap = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    if (instance) { // instance null değilse yükle
        charBitmap = new Bitmap(hDC, IDB_BITMAP3, instance);
        _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, instance);
        _pEnemyMissileBitmap = new Bitmap(hDC, IDB_BMISSILE, instance);
    }
    healthPWBitmap = new Bitmap(hDC, "Health.bmp");
    ammoPWBitmap = new Bitmap(hDC, "Ammo.bmp");
    pointPWBitmap = new Bitmap(hDC, "Point.bmp");
    armorPWBitmap = new Bitmap(hDC, "Armor.bmp");
    keyBitmap = new Bitmap(hDC, "Key.bmp");
    endPointBitmap = new Bitmap(hDC, "Gate.bmp");
}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    if (!pSpriteHitter || !pSpriteHittee) return FALSE;

    Player* pPlayer = nullptr;
    // Sprite* pOther = nullptr; // Bu değişken artık pSpriteHitter veya pSpriteHittee olacak

    SpriteType hitterType = pSpriteHitter->GetType();
    SpriteType hitteeType = pSpriteHittee->GetType();

    if (hitterType == SPRITE_TYPE_ENEMY && hitteeType == SPRITE_TYPE_ENEMY)
    {
        return FALSE;
    }

    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE) return FALSE;
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) return FALSE;

    if ((hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) ||
        (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE))
    {
        pSpriteHitter->Kill();
        pSpriteHittee->Kill();
        return FALSE;
    }

    // Oyuncu mermisi ile çarpışmalar
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE) {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_ENEMY) {
            pSpriteHitter->Kill();
            pSpriteHittee->Kill();
            if (charSprite) static_cast<Player*>(charSprite)->AddScore(10);
            return FALSE;
        }
        // Player_Missile vs Player -> FALSE (oyuncu kendi mermisiyle çarpışmaz)
        if (hitteeType == SPRITE_TYPE_PLAYER) return FALSE;
    }
    else if (hitteeType == SPRITE_TYPE_PLAYER_MISSILE) { // simetrik durum
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_ENEMY) {
            pSpriteHittee->Kill();
            pSpriteHitter->Kill();
            if (charSprite) static_cast<Player*>(charSprite)->AddScore(10);
            return FALSE;
        }
        if (hitterType == SPRITE_TYPE_PLAYER) return FALSE;
    }


    // Düşman mermisi ile çarpışmalar
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE) {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_PLAYER) {
            pSpriteHitter->Kill();
            // Player* playerPtr = static_cast<Player*>(pSpriteHittee);
            // playerPtr->TakeDamage(10); // Hasar mekanizması
            return FALSE;
        }
        // Enemy_Missile vs Enemy -> FALSE (düşman kendi mermisiyle çarpışmaz)
        if (hitteeType == SPRITE_TYPE_ENEMY) return FALSE;
    }
    else if (hitteeType == SPRITE_TYPE_ENEMY_MISSILE) { // simetrik durum
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_PLAYER) {
            pSpriteHittee->Kill();
            // Player* playerPtr = static_cast<Player*>(pSpriteHitter);
            // playerPtr->TakeDamage(10);
            return FALSE;
        }
        if (hitterType == SPRITE_TYPE_ENEMY) return FALSE;
    }


    // Oyuncu ile ilgili çarpışmalar
    Sprite* pOtherSpriteForPlayer = nullptr;
    if (pSpriteHitter == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHitter);
        pOtherSpriteForPlayer = pSpriteHittee;
    }
    else if (pSpriteHittee == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHittee);
        pOtherSpriteForPlayer = pSpriteHitter;
    }

    if (pPlayer && pOtherSpriteForPlayer) {
        Bitmap* pOtherBitmap = pOtherSpriteForPlayer->GetBitmap();
        SpriteType otherType = pOtherSpriteForPlayer->GetType();

        if (pOtherBitmap == keyBitmap) { pPlayer->AddKey(); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == healthPWBitmap) { pPlayer->AddHealth(20); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == armorPWBitmap) { pPlayer->AddArmor(20); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == pointPWBitmap) { pPlayer->AddScore(50); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == ammoPWBitmap) { pPlayer->AddSecondaryAmmo(10); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == endPointBitmap) {
            int requiredKeys = std::min(4, currentLevel); // std::min kullan
            if (pPlayer->GetKeys() >= requiredKeys) {
                isLevelFinished = true;
            }
            return FALSE;
        }

        if (otherType == SPRITE_TYPE_ENEMY)
        {
            // Player vs Enemy temasında TRUE dönerek geçişi engelle
            // Player* playerPtr = pPlayer; 
            // playerPtr->TakeDamage(5); // Temas hasarı
            return TRUE;
        }

        if (otherType == SPRITE_TYPE_WALL)
        {
            return TRUE;
        }
    }

    // Eğer yukarıdaki koşulların hiçbiri karşılanmazsa, varsayılan olarak engelleme
    // (Örn: İki item, bir item bir duvarla (gerçi duvarlar zaten BA_STOP olurdu) vb.)
    // Ancak genellikle bu tür durumlar zaten Sprite'ın kendi BA_STOP'u ile çözülür.
    // Eğer pSpriteHitter veya pSpriteHittee BA_STOP ise ve diğer sprite ile kesişiyorsa,
    // GameEngine::UpdateSprites içindeki mantık zaten pozisyonu geri alacaktır.
    // Bu fonksiyonun TRUE dönmesi, bu geri almayı tetikler.
    // FALSE dönmesi ise, "bu çarpışma benim için önemli değil, GameEngine normal davransın" demektir.
    return FALSE;
}

void OnLevelComplete() {
    currentLevel++;
    CleanupLevel();
    GenerateLevel(currentLevel);
    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
}

void CleanupLevel() {
    if (!game_engine) return;

    nonCollidableTiles.clear();

    // charSprite null değilse ve listedeyse çıkar.
    // GameEngine::RemoveSprite null olmayan bir sprite bekler.
    if (charSprite) {
        // Oyuncunun listede olup olmadığını kontrol etmek iyi bir pratik olabilir
        // ama RemoveSprite zaten bulunamazsa bir şey yapmaz.
        game_engine->RemoveSprite(charSprite);
    }

    game_engine->CleanupSprites(); // Geri kalan her şeyi siler

    if (charSprite) { // Oyuncu hala hayattaysa (null değilse) geri ekle
        game_engine->AddSprite(charSprite);
    }
}