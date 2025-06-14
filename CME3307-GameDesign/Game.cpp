// Game.cpp

#include <string> // std::to_wstring için
#include "Game.h"
#include <vector>
#include "Enemy.h" // EnemyType enum'u için
#include <random>
#include <limits>    // std::numeric_limits için eklendi
#include <cmath>     // sqrt, pow için eklendi
#include <algorithm> // std::min, std::max için
#undef max           // Windows.h'daki max ile std::max çakışmasını önlemek için
#undef min
//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0, 0, 4000, 4000 }; // Geniş bir alan

DWORD g_dwLastSpawnTime = 0;
const DWORD ENEMY_SPAWN_INTERVAL = 6000; // 15 saniye

DWORD g_dwLastClosestEnemySpawnTime = 0;
const DWORD CLOSEST_ENEMY_SPAWN_INTERVAL = 3000; // 6 saniye

GameEngine* game_engine;
Player* charSprite;
MazeGenerator* mazeGenerator;
Bitmap* _pEnemyMissileBitmap;
FOVBackground* fovEffect;
int TILE_SIZE; // Global TILE_SIZE
Camera* camera;
HDC offscreenDC;
HBITMAP offscreenBitmap;
Background* background;
Bitmap* wallBitmap;
Bitmap* charBitmap;
Bitmap* _pEnemyBitmap;
HINSTANCE instance;
int window_X, window_Y;
Bitmap* _pPlayerMissileBitmap;

Bitmap* healthPWBitmap = nullptr;
Bitmap* ammoPWBitmap = nullptr;
Bitmap* armorPWBitmap = nullptr;
Bitmap* pointPWBitmap = nullptr;
Bitmap* floorBitmap = nullptr;
Bitmap* keyBitmap = nullptr;
Bitmap* endPointBitmap = nullptr;
Bitmap* secondWeaponBitmap = nullptr; // Kullanılmıyorsa null kalabilir

bool isLevelFinished = false;
int currentLevel;

// YARDIMCI FONKSİYON: Belirtilen bir RECT alanının labirentte tamamen boş olup olmadığını kontrol eder
bool IsAreaClearForSpawn(int tileX, int tileY, int spriteWidthInTiles, int spriteHeightInTiles)
{
    if (!mazeGenerator || TILE_SIZE == 0)
        return false;

    for (int y = 0; y < spriteHeightInTiles; ++y)
    {
        for (int x = 0; x < spriteWidthInTiles; ++x)
        {
            if (mazeGenerator->IsWall(tileX + x, tileY + y))
            {
                return false;
            }
        }
    }
    return true;
}

BOOL GameInitialize(HINSTANCE hInst)
{
    window_X = 1500;
    window_Y = 700;
    game_engine = new GameEngine(hInst, TEXT("Maze Game"), TEXT("Maze Game"),
        IDI_SPACEOUT, IDI_SPACEOUT_SM, window_X, window_Y);
    if (game_engine == NULL)
        return FALSE;

    game_engine->SetFrameRate(30); // 30 FPS
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
    if (!m_pTarget)
        return;

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
    if (!game_engine) return; // game_engine null ise çık

    srand(GetTickCount());
    offscreenDC = CreateCompatibleDC(GetDC(hWindow));
    offscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        game_engine->GetWidth(), game_engine->GetHeight());
    SelectObject(offscreenDC, offscreenBitmap);

    HDC hDC = GetDC(hWindow);
    LoadBitmaps(hDC); // Bitmap'leri yükle

    if (!_pPlayerMissileBitmap && instance)
        _pPlayerMissileBitmap = new Bitmap(hDC, IDB_MISSILE, instance);

    if (wallBitmap) TILE_SIZE = wallBitmap->GetHeight(); else TILE_SIZE = 50;

    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    mazeGenerator = new MazeGenerator(15, 15); // Labirent boyutu (örneğin 15x15)

    if (charBitmap && mazeGenerator)
        charSprite = new Player(charBitmap, mazeGenerator);

    currentLevel = 1;
    GenerateLevel(currentLevel); // Seviyeyi oluştur

    if (charSprite)
        game_engine->AddSprite(charSprite);

    if (charSprite)
        camera = new Camera(charSprite, window_X, window_Y);

    if (charSprite)
        fovEffect = new FOVBackground(charSprite, 90, 350, 75); // FOV ayarları

    // Başlangıç düşmanları
    if (mazeGenerator && _pEnemyBitmap && charSprite && TILE_SIZE > 0)
    {
        const auto& mazeData = mazeGenerator->GetMaze();
        if (!mazeData.empty() && !mazeData[0].empty())
        {
            int enemySpriteWidthInTiles = (_pEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
            int enemySpriteHeightInTiles = (_pEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

            for (int i = 0; i < 5; i++) // 5 başlangıç düşmanı
            {
                EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;
                Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
                    mazeGenerator, charSprite, type);
                int ex, ey;
                int tryCount = 0;
                const int maxSpawnTries = 50;
                do
                {
                    ex = (rand() % (static_cast<int>(mazeData[0].size()) - enemySpriteWidthInTiles + 1));
                    ey = (rand() % (static_cast<int>(mazeData.size()) - enemySpriteHeightInTiles + 1));
                    tryCount++;
                } while (!IsAreaClearForSpawn(ex, ey, enemySpriteWidthInTiles, enemySpriteHeightInTiles) && tryCount < maxSpawnTries);

                if (tryCount < maxSpawnTries)
                {
                    pEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE);
                    game_engine->AddSprite(pEnemy);
                }
                else
                {
                    delete pEnemy; // Yer bulunamadıysa sil
                }
            }
        }
    }
    g_dwLastSpawnTime = GetTickCount(); // İlk spawn için zamanlayıcıyı başlat
    g_dwLastClosestEnemySpawnTime = GetTickCount();
}

void GameEnd()
{
    if (game_engine) game_engine->CloseMIDIPlayer();

    if (offscreenBitmap) DeleteObject(offscreenBitmap);
    if (offscreenDC) DeleteDC(offscreenDC);

    // charSprite GameEngine tarafından CleanupSprites ile silinecek.
    // Bu yüzden burada charSprite'ı silmeye gerek yok, sadece null yapalım.
    // Ancak eğer CleanupLevel oyuncuyu koruyorsa, o zaman GameEnd'de oyuncu ayrıca silinmeli.
    // Mevcut mantıkta CleanupSprites her şeyi siliyor.

    if (game_engine) {
        game_engine->CleanupSprites(); // Bu çağrı charSprite dahil her şeyi siler
        delete game_engine; game_engine = nullptr;
    }
    charSprite = nullptr; // GameEngine silindiği için charSprite artık geçersiz

    // Bitmap'leri sil
    delete _pEnemyMissileBitmap; _pEnemyMissileBitmap = nullptr;
    delete background; background = nullptr;
    delete wallBitmap; wallBitmap = nullptr;
    delete charBitmap; charBitmap = nullptr; // charSprite silindiği için bu da silinmeli
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
    // Oyunu aktive et (örneğin, duraklatılmışsa devam ettir)
}

void GameDeactivate(HWND hWindow)
{
    // Oyunu deaktive et (örneğin, pencere odaktan çıkınca duraklat)
}

void GamePaint(HDC hDC)
{
    if (!hDC) return;

    if (background && camera) background->Draw(hDC, 0, 0); // Arka plan kameradan bağımsız çizilir

    for (const auto& tile : nonCollidableTiles)
    {
        if (tile.bitmap && camera) // camera null kontrolü
            tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    if (game_engine && camera) // camera null kontrolü
    {
        // Sprite'ları çizmeden önce oyuncunun ölü olup olmadığını kontrol et
        bool playerIsDead = false;
        if (charSprite) {
            Player* pPlayer = static_cast<Player*>(charSprite);
            if (pPlayer && pPlayer->IsDead()) {
                playerIsDead = true;
            }
        }

        for (Sprite* sprite : game_engine->GetSprites())
        {
            if (sprite) {
                // Eğer oyuncu öldüyse ve bu sprite oyuncu ise çizme (veya bir "ölü sprite" çiz)
                if (playerIsDead && sprite == charSprite) {
                    // İsteğe bağlı: Oyuncunun ölü halini gösteren bir sprite çizilebilir
                    // Veya hiçbir şey çizilmez
                    continue;
                }
                sprite->Draw(hDC, camera->x, camera->y);
            }
        }
    }

    if (fovEffect && camera) // camera null kontrolü
        fovEffect->Draw(hDC, camera->x, camera->y);

    // Oyuncu bilgilerini ekrana yazdır
    if (charSprite)
    {
        Player* pPlayer = static_cast<Player*>(charSprite);
        if (pPlayer) // pPlayer null değilse
        {
            SetTextColor(hDC, RGB(255, 255, 255)); // Beyaz yazı
            SetBkMode(hDC, TRANSPARENT);          // Şeffaf arka plan

            std::wstring healthText = L"Health: " + std::to_wstring(pPlayer->GetHealth());
            TextOutW(hDC, 10, 10, healthText.c_str(), static_cast<int>(healthText.length()));

            std::wstring armorText = L"Armor: " + std::to_wstring(pPlayer->GetArmor());
            TextOutW(hDC, 10, 30, armorText.c_str(), static_cast<int>(armorText.length()));

            std::wstring scoreText = L"Score: " + std::to_wstring(pPlayer->GetScore());
            TextOutW(hDC, 10, 50, scoreText.c_str(), static_cast<int>(scoreText.length()));

            std::wstring keysText = L"Keys: " + std::to_wstring(pPlayer->GetKeys());
            TextOutW(hDC, 10, 70, keysText.c_str(), static_cast<int>(keysText.length()));

            std::wstring PistolAmmoText = L"Pis A: " + std::to_wstring(pPlayer->GetPistolAmmo());
            TextOutW(hDC, 10, 90, PistolAmmoText.c_str(), static_cast<int>(PistolAmmoText.length()));

            std::wstring ShotgunAmmoText = L"Shot A: " + std::to_wstring(pPlayer->GetShotgunAmmo());
            TextOutW(hDC, 10, 110, ShotgunAmmoText.c_str(), static_cast<int>(ShotgunAmmoText.length()));

            std::wstring SMGAmmoText = L"SMG A: " + std::to_wstring(pPlayer->GetSMGAmmo());
            TextOutW(hDC, 10, 130, SMGAmmoText.c_str(), static_cast<int>(SMGAmmoText.length()));

            // Eğer oyuncu öldüyse "GAME OVER" yazdır
            if (pPlayer->IsDead()) {
                SetTextColor(hDC, RGB(255, 0, 0)); // Kırmızı yazı
                HFONT hFont, hOldFont;
                // Daha büyük bir font oluştur (örneğin Arial, 48 punto)
                hFont = CreateFont(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    VARIABLE_PITCH, TEXT("Arial"));
                hOldFont = (HFONT)SelectObject(hDC, hFont);

                std::wstring gameOverText = L"GAME OVER";
                RECT screenRect = { 0, 0, window_X, window_Y };
                DrawTextW(hDC, gameOverText.c_str(), -1, &screenRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hDC, hOldFont); // Eski fontu geri yükle
                DeleteObject(hFont);        // Oluşturulan fontu sil
            }
        }
    }
}

void GameCycle()
{
    if (!game_engine) return;

    // Oyuncu ölmüşse oyun döngüsünü durdur veya farklı bir mantık işlet
    if (charSprite) {
        Player* pPlayer = static_cast<Player*>(charSprite);
        if (pPlayer && pPlayer->IsDead()) {
            // Oyun bitti, sadece GamePaint'i çağırıp ekranda GAME OVER göstermeye devam et.
            // Yeni sprite update'leri veya spawn'ları yapma.
            HWND hWindow = game_engine->GetWindow();
            if (hWindow) {
                HDC hDC = GetDC(hWindow);
                if (hDC) {
                    GamePaint(offscreenDC);
                    BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
                        offscreenDC, 0, 0, SRCCOPY);
                    ReleaseDC(hWindow, hDC);
                }
            }
            return; // Oyun döngüsünün geri kalanını atla
        }
    }


    if (camera) camera->Update();
    if (background) background->Update();

    game_engine->UpdateSprites(); // Bu, oyuncu dahil tüm spriteları günceller

    // Düşman spawn mantığı
    if (GetTickCount() - g_dwLastSpawnTime > ENEMY_SPAWN_INTERVAL)
    {
        SpawnEnemyNearPlayer();
        g_dwLastSpawnTime = GetTickCount();
    }

    if (GetTickCount() - g_dwLastClosestEnemySpawnTime > CLOSEST_ENEMY_SPAWN_INTERVAL)
    {
        SpawnEnemyNearClosest();
        g_dwLastClosestEnemySpawnTime = GetTickCount();
    }

    if (isLevelFinished)
    {
        OnLevelComplete();
        isLevelFinished = false; // Seviye tamamlandıktan sonra sıfırla
        return; // Yeni seviye yüklendi, bu döngüyü bitir
    }

    if (fovEffect && camera) fovEffect->Update(camera->x, camera->y);

    HWND hWindow = game_engine->GetWindow();
    if (hWindow)
    {
        HDC hDC = GetDC(hWindow);
        if (hDC)
        {
            GamePaint(offscreenDC); // offscreenDC'ye çizim yap
            BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
                offscreenDC, 0, 0, SRCCOPY); // Ekrana kopyala
            ReleaseDC(hWindow, hDC);
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
    std::uniform_int_distribution<> distr(-7, 7); // Arama alanı

    int spawnTileX, spawnTileY;
    int tryCount = 0;
    const int maxTries = 30;

    const auto& mazeData = mazeGenerator->GetMaze();
    if (mazeData.empty() || mazeData[0].empty()) return;
    int mazeWidthInTiles = static_cast<int>(mazeData[0].size());
    int mazeHeightInTiles = static_cast<int>(mazeData.size());

    int enemySpriteWidthInTiles = (_pEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
    int enemySpriteHeightInTiles = (_pEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

    do
    {
        spawnTileX = playerTileX + distr(gen);
        spawnTileY = playerTileY + distr(gen);
        tryCount++;

        if (spawnTileX < 0 || spawnTileX + enemySpriteWidthInTiles > mazeWidthInTiles ||
            spawnTileY < 0 || spawnTileY + enemySpriteHeightInTiles > mazeHeightInTiles)
        {
            continue;
        }
    } while (!IsAreaClearForSpawn(spawnTileX, spawnTileY, enemySpriteWidthInTiles, enemySpriteHeightInTiles) && tryCount < maxTries);

    if (tryCount < maxTries)
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

        int enemySpriteWidthInTiles = (_pEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
        int enemySpriteHeightInTiles = (_pEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

        if (IsAreaClearForSpawn(enemyTileX, enemyTileY, enemySpriteWidthInTiles, enemySpriteHeightInTiles))
        {
            EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
            Enemy* newEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
                mazeGenerator, charSprite, type);
            newEnemy->SetPosition(enemyTileX * TILE_SIZE, enemyTileY * TILE_SIZE);
            game_engine->AddSprite(newEnemy);
        }
    }
}

void HandleKeys()
{
    // Player sınıfı kendi girişlerini yönetiyor
}

void MouseButtonDown(int x, int y, BOOL bLeft)
{
    if (bLeft && camera && charSprite)
    {
        // Oyuncu ölmüşse ateş etme
        Player* pPlayerCasted = static_cast<Player*>(charSprite);
        if (pPlayerCasted && pPlayerCasted->IsDead()) return;

        int targetWorldX = x + camera->x;
        int targetWorldY = y + camera->y;

        if (pPlayerCasted) // Zaten cast edilmişti, tekrar Player* pPlayer = ... gerek yok
        {
            pPlayerCasted->Fire(targetWorldX, targetWorldY);
        }
    }
}

void MouseButtonUp(int x, int y, BOOL bLeft)
{
    // Gerekirse implemente edilebilir
}

void MouseMove(int x, int y)
{
    if (fovEffect)
        fovEffect->UpdateMousePos(x, y);
}

void HandleJoystick(JOYSTATE jsJoystickState)
{
    // Joystick desteği gerekirse implemente edilebilir
}

// GenerateMaze, GenerateLevel tarafından kapsandığı için kaldırılabilir veya basitleştirilebilir
void GenerateMaze(Bitmap* tileBit)
{
    if (!mazeGenerator || !wallBitmap || !game_engine || TILE_SIZE == 0) return;

    mazeGenerator->generateMaze(); // Sadece temel labirent yollarını ve duvarlarını oluşturur
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->GetMaze();
    if (mazeArray.empty() || mazeArray[0].empty()) return;

    int tile_width = TILE_SIZE;
    int tile_height = TILE_SIZE;
    RECT rcBounds = { 0, 0, static_cast<long>(mazeArray[0].size() * tile_width), static_cast<long>(mazeArray.size() * tile_height) };

    for (size_t r = 0; r < mazeArray.size(); ++r)
    {
        for (size_t c = 0; c < mazeArray[r].size(); ++c)
        {
            if (mazeArray[r][c] == static_cast<int>(TileType::WALL))
            {
                int posX = static_cast<int>(c * tile_width);
                int posY = static_cast<int>(r * tile_height);
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP, SPRITE_TYPE_WALL);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
        }
    }
}


void GenerateLevel(int level)
{
    if (!mazeGenerator || !game_engine || !wallBitmap || !floorBitmap || TILE_SIZE == 0)
    {
        if (game_engine) game_engine->ErrorQuit(TEXT("Essential resources for level generation are missing!"));
        return;
    }

    CleanupLevel(); // Önceki seviyenin sprite'larını temizle (oyuncu hariç)

    mazeGenerator->SetupLevel(level); // Yeni seviye için mantıksal labirenti oluştur
    const auto& mazeArray = mazeGenerator->GetMaze();
    if (mazeArray.empty() || mazeArray[0].empty())
    {
        if (game_engine) game_engine->ErrorQuit(TEXT("Maze data is empty after SetupLevel!"));
        return;
    }

    int tile_width = TILE_SIZE;
    int tile_height = TILE_SIZE;
    RECT rcBounds = { 0, 0, static_cast<long>(mazeArray[0].size() * tile_width), static_cast<long>(mazeArray.size() * tile_height) };

    for (size_t r = 0; r < mazeArray.size(); ++r)
    {
        for (size_t c = 0; c < mazeArray[r].size(); ++c)
        {
            int posX = static_cast<int>(c * tile_width);
            int posY = static_cast<int>(r * tile_height);
            POINT pos = { posX, posY };
            int tileValue = mazeArray[r][c];
            Bitmap* itemBitmap = nullptr;

            switch (static_cast<TileType>(tileValue))
            {
            case TileType::WALL:
            {
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

            case TileType::START_POINT: // Başlangıç noktası için özel bir sprite yoksa, zemin çizilir
            case TileType::PATH:
            default:
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                continue; // Item oluşturma mantığını atla
            }

            if (itemBitmap) // Eğer bir item ise
            {
                Sprite* item = new Sprite(itemBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap); // Item'ın altına zemin
            }
            else if (static_cast<TileType>(tileValue) != TileType::WALL) {
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap); // Duvar değilse ve item da değilse zemin çiz
            }
        }
    }

    // Oyuncuyu başlangıç noktasına yerleştir
    if (charSprite && mazeGenerator)
    {
        std::pair<int, int> startPosCoords = mazeGenerator->GetStartPos();
        if (startPosCoords.first != -1 && TILE_SIZE > 0)
        {
            charSprite->SetPosition(startPosCoords.first * tile_width, startPosCoords.second * tile_height);
        }
    }
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap)
{
    if (bitmap)
        nonCollidableTiles.push_back({ x, y, bitmap });
}

void LoadBitmaps(HDC hDC)
{
    if (!hDC) return;

    floorBitmap = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    if (instance) // instance null değilse
    {
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
    SpriteType hitterType = pSpriteHitter->GetType();
    SpriteType hitteeType = pSpriteHittee->GetType();

    // Oyuncu ölmüşse, başka çarpışmaları işleme (özellikle hasar verenleri)
    if (charSprite) {
        Player* castedPlayer = static_cast<Player*>(charSprite);
        if (castedPlayer && castedPlayer->IsDead()) {
            // Sadece duvarla çarpışmaya izin ver (hareketi durdurmak için)
            if ((pSpriteHitter == charSprite && hitteeType == SPRITE_TYPE_WALL) ||
                (pSpriteHittee == charSprite && hitterType == SPRITE_TYPE_WALL)) {
                return TRUE;
            }
            return FALSE; // Diğer tüm çarpışmaları yoksay
        }
    }


    if (hitterType == SPRITE_TYPE_ENEMY && hitteeType == SPRITE_TYPE_ENEMY) return FALSE;
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE) return FALSE;
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) return FALSE;

    if ((hitterType == SPRITE_TYPE_PLAYER_MISSILE && hitteeType == SPRITE_TYPE_ENEMY_MISSILE) ||
        (hitterType == SPRITE_TYPE_ENEMY_MISSILE && hitteeType == SPRITE_TYPE_PLAYER_MISSILE))
    {
        pSpriteHitter->Kill();
        pSpriteHittee->Kill();
        return FALSE;
    }

    // Oyuncu mermisi
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_ENEMY) { pSpriteHitter->Kill(); pSpriteHittee->Kill(); if (charSprite) static_cast<Player*>(charSprite)->AddScore(10); return FALSE; }
        if (hitteeType == SPRITE_TYPE_PLAYER) return FALSE; // Kendiyle çarpışmaz
    }
    else if (hitteeType == SPRITE_TYPE_PLAYER_MISSILE) // Simetrik
    {
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_ENEMY) { pSpriteHittee->Kill(); pSpriteHitter->Kill(); if (charSprite) static_cast<Player*>(charSprite)->AddScore(10); return FALSE; }
        if (hitterType == SPRITE_TYPE_PLAYER) return FALSE;
    }

    // Düşman mermisi
    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_PLAYER && pSpriteHittee == charSprite) {
            pSpriteHitter->Kill();
            static_cast<Player*>(pSpriteHittee)->TakeDamage(12); // Varsayılan mermi hasarı
            return FALSE;
        }
        if (hitteeType == SPRITE_TYPE_ENEMY) return FALSE; // Kendiyle çarpışmaz
    }
    else if (hitteeType == SPRITE_TYPE_ENEMY_MISSILE) // Simetrik
    {
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_PLAYER && pSpriteHitter == charSprite) {
            pSpriteHittee->Kill();
            static_cast<Player*>(pSpriteHitter)->TakeDamage(12); // Varsayılan mermi hasarı
            return FALSE;
        }
        if (hitterType == SPRITE_TYPE_ENEMY) return FALSE;
    }

    // Oyuncu ile ilgili diğer çarpışmalar
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

        // Item'lar
        if (pOtherBitmap == keyBitmap) { pPlayer->AddKey(); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == healthPWBitmap) { pPlayer->AddHealth(20); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == armorPWBitmap) { pPlayer->AddArmor(20); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == pointPWBitmap) { pPlayer->AddScore(50); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == ammoPWBitmap) { pPlayer->AddSecondaryAmmo(10); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == endPointBitmap) {
            int requiredKeys = std::min(4, currentLevel);
            if (pPlayer->GetKeys() >= requiredKeys) isLevelFinished = true;
            return FALSE;
        }

        // Oyuncu vs Düşman (Temas)
        if (otherType == SPRITE_TYPE_ENEMY) {
            Enemy* pEnemy = static_cast<Enemy*>(pOtherSpriteForPlayer);
            if (pEnemy) {
                if (pEnemy->GetEnemyType() == EnemyType::CHASER) pPlayer->TakeDamage(1);
                else if (pEnemy->GetEnemyType() == EnemyType::TURRET) pPlayer->TakeDamage(1); // Turret temas hasarı
            }
            return TRUE; // Hareketi engelle
        }

        // Oyuncu vs Duvar
        if (otherType == SPRITE_TYPE_WALL) return TRUE; // Hareketi engelle
    }

    return FALSE; // Diğer tüm durumlar için engelleme yok
}

void OnLevelComplete()
{
    currentLevel++;
    CleanupLevel(); // Oyuncuyu koruyarak temizle
    GenerateLevel(currentLevel);
    g_dwLastSpawnTime = GetTickCount(); // Zamanlayıcıları sıfırla
    g_dwLastClosestEnemySpawnTime = GetTickCount();
}

void CleanupLevel()
{
    if (!game_engine) return;
    nonCollidableTiles.clear();

    if (charSprite) {
        game_engine->RemoveSprite(charSprite); // Oyuncuyu geçici olarak çıkar
    }
    game_engine->CleanupSprites(); // Geri kalan her şeyi sil

    if (charSprite) {
        // Eğer oyuncu bir önceki seviyede ölmediyse ve hala geçerliyse geri ekle.
        // Player::IsDead() kontrolü burada da yapılabilir.
        // Ancak charSprite'ı null yapmadıysak, AddSprite güvenli olmalı.
        game_engine->AddSprite(charSprite);
    }
}