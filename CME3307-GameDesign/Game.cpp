// Game.cpp

#include <string> // std::to_wstring için
#include "Game.h"
#include <vector>
#include "Enemy.h" // EnemyType enum'u için
#include <random>
#include <limits>    // std::numeric_limits için eklendi
#include <cmath>     // sqrt, pow için eklendi
#include <algorithm> // std::min, std::max için
#include <fstream>   // NEW: For file I/O (high scores)
#include <iostream>  // NEW: For file I/O (high scores)

#include <sstream>   // YENİ: String'leri ayrıştırmak için
#include <ctime>     // YENİ: Mevcut zamanı almak için
#pragma warning(disable : 4996) // YENİ: std::localtime için gelen uyarıyı devre dışı bırak

#undef max           // Windows.h'daki max ile std::max çakışmasını önlemek için
#undef min
//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0, 0, 4000, 4000 }; // Geniş bir alan

DWORD g_dwLastSpawnTime = 0;
const DWORD ENEMY_SPAWN_INTERVAL = 6000; // 6 saniye

DWORD g_dwLastClosestEnemySpawnTime = 0;
const DWORD CLOSEST_ENEMY_SPAWN_INTERVAL = 3000; // 3 saniye

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

// NEW: Global variables for the level transition screen and UI fonts
bool  g_bInLevelTransition = false;
DWORD g_dwLevelTransitionStartTime = 0;
const DWORD LEVEL_TRANSITION_DURATION = 3000; // 3 seconds
HFONT g_hUIFont = NULL;
HFONT g_hBigFont = NULL;


// NEW: Global variables for high score system
const char* HIGH_SCORE_FILE = "highscores.txt";
const int     MAX_HIGH_SCORES = 5;
std::vector<HighScoreEntry> g_HighScores;
bool          g_bScoreSaved = false; // Flag to prevent saving score multiple times



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

    // NEW: Load high scores from file at the start of the game
    LoadHighScores();
    g_bScoreSaved = false; // Reset score-saved flag for the new game

    // NEW: Create fonts for the UI
    g_hUIFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
    g_hBigFont = CreateFont(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, TEXT("Impact"));


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

    if (game_engine) {
        game_engine->CleanupSprites();
        delete game_engine; game_engine = nullptr;
    }
    charSprite = nullptr;

    delete _pEnemyMissileBitmap; _pEnemyMissileBitmap = nullptr;
    delete background; background = nullptr;
    delete wallBitmap; wallBitmap = nullptr;
    delete charBitmap; charBitmap = nullptr;
    delete _pEnemyBitmap; _pEnemyBitmap = nullptr;
    delete healthPWBitmap; healthPWBitmap = nullptr;
    delete ammoPWBitmap; ammoPWBitmap = nullptr;
    delete pointPWBitmap; pointPWBitmap = nullptr;
    delete armorPWBitmap; armorPWBitmap = nullptr;
    delete floorBitmap; floorBitmap = nullptr;
    delete keyBitmap; keyBitmap = nullptr;
    delete endPointBitmap; endPointBitmap = nullptr;
    delete _pPlayerMissileBitmap; _pPlayerMissileBitmap = nullptr;

    delete mazeGenerator; mazeGenerator = nullptr;
    delete camera; camera = nullptr;
    delete fovEffect; fovEffect = nullptr;

    // NEW: Delete the GDI font objects
    if (g_hUIFont) DeleteObject(g_hUIFont);
    if (g_hBigFont) DeleteObject(g_hBigFont);
}

void GameActivate(HWND hWindow) {}
void GameDeactivate(HWND hWindow) {}

// MODIFIED: GamePaint is now cleaner. The UI drawing is moved to the DrawUI function.
void GamePaint(HDC hDC)
{
    if (!hDC) return;

    if (background && camera) background->Draw(hDC, 0, 0);

    for (const auto& tile : nonCollidableTiles)
    {
        if (tile.bitmap && camera)
            tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    if (game_engine && camera)
    {
        bool playerIsDead = (charSprite && static_cast<Player*>(charSprite)->IsDead());

        for (Sprite* sprite : game_engine->GetSprites())
        {
            if (sprite) {
                if (playerIsDead && sprite == charSprite) {
                    continue;
                }
                sprite->Draw(hDC, camera->x, camera->y);
            }
        }
    }

    if (fovEffect && camera)
        fovEffect->Draw(hDC, camera->x, camera->y);

    // NEW: All UI is now drawn by a dedicated function.
    DrawUI(hDC);
}


// MODIFIED: GameCycle now handles the level transition state.
void GameCycle()
{
    if (!game_engine) return;

    // First, check if we are in a level transition
    if (g_bInLevelTransition)
    {
        // If the transition screen has been shown long enough...
        if (GetTickCount() - g_dwLevelTransitionStartTime > LEVEL_TRANSITION_DURATION)
        {
            g_bInLevelTransition = false; // End the transition
            OnLevelComplete();          // And now, actually load the next level
        }
        // During the transition, we don't update game logic, we just paint.
    }
    // If not in transition, run the normal game loop
    else
    {
        // Player death check
        if (charSprite) {
            Player* pPlayer = static_cast<Player*>(charSprite);
            if (pPlayer && pPlayer->IsDead()) {
                // If the player is dead, we stop updating game logic and just paint
                // The painting part is handled at the end of GameCycle
                if (!g_bScoreSaved)
                {
                    CheckAndSaveScore(pPlayer->GetScore());
                    g_bScoreSaved = true;
                }
            }
            else // Player is alive, run normal game logic
            {
                if (camera) camera->Update();
                if (background) background->Update();

                game_engine->UpdateSprites();

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

                // Check if the level has just been completed
                if (isLevelFinished)
                {
                    isLevelFinished = false; // Reset the trigger
                    g_bInLevelTransition = true; // Start the transition
                    g_dwLevelTransitionStartTime = GetTickCount();
                    // We DO NOT call OnLevelComplete() here yet. We wait for the transition to finish.
                }

                if (fovEffect && camera) fovEffect->Update(camera->x, camera->y);
            }
        }
    }


    // Painting happens every cycle, regardless of game state (playing, dead, or transition)
    HWND hWindow = game_engine->GetWindow();
    if (hWindow)
    {
        HDC hDC = GetDC(hWindow);
        if (hDC)
        {
            GamePaint(offscreenDC); // Draw everything to the offscreen buffer
            BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
                offscreenDC, 0, 0, SRCCOPY); // Copy buffer to screen
            ReleaseDC(hWindow, hDC);
        }
    }
}


// NEW: This function handles drawing all UI elements.
void DrawUI(HDC hDC)
{
    if (charSprite == nullptr || g_hUIFont == NULL || g_hBigFont == NULL) return;

    Player* pPlayer = static_cast<Player*>(charSprite);
    HFONT hOldFont = (HFONT)SelectObject(hDC, g_hUIFont);
    SetBkMode(hDC, TRANSPARENT);

    if (g_bInLevelTransition)
    {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        RECT rcOverlay = { 0, 0, window_X, window_Y };
        FillRect(hDC, &rcOverlay, hBrush);
        DeleteObject(hBrush);

        SetTextColor(hDC, RGB(170, 255, 170));
        SelectObject(hDC, g_hBigFont);

        std::wstring levelText = L"LEVEL " + std::to_wstring(currentLevel) + L" COMPLETE";
        DrawTextW(hDC, levelText.c_str(), -1, &rcOverlay, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hDC, hOldFont);
        return;
    }

    SetTextColor(hDC, RGB(255, 255, 255));
    SelectObject(hDC, g_hUIFont);

    std::wstring text;
    int yPos = 10;
    int xPos = 10;
    int yIncrement = 20;

    text = L"Health: " + std::to_wstring(pPlayer->GetHealth());
    TextOutW(hDC, xPos, yPos, text.c_str(), static_cast<int>(text.length()));
    yPos += yIncrement;

    text = L"Armor: " + std::to_wstring(pPlayer->GetArmor());
    TextOutW(hDC, xPos, yPos, text.c_str(), static_cast<int>(text.length()));
    yPos += yIncrement;

    text = L"Score: " + std::to_wstring(pPlayer->GetScore());
    TextOutW(hDC, xPos, yPos, text.c_str(), static_cast<int>(text.length()));
    yPos += yIncrement;

    int requiredKeys = std::min(4, currentLevel);
    text = L"Keys: " + std::to_wstring(pPlayer->GetKeys()) + L" / " + std::to_wstring(requiredKeys);
    TextOutW(hDC, xPos, yPos, text.c_str(), static_cast<int>(text.length()));

    if (pPlayer->IsDead()) {
        SetTextColor(hDC, RGB(255, 0, 0));
        SelectObject(hDC, g_hBigFont);

        std::wstring gameOverText = L"GAME OVER";
        RECT screenRect = { 0, 0, window_X, window_Y };
        DrawTextW(hDC, gameOverText.c_str(), -1, &screenRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hDC, g_hUIFont);
        SetTextColor(hDC, RGB(220, 220, 220));

        yPos = screenRect.bottom / 2 + 60;
        yIncrement = 25;
        xPos = window_X / 2 - 150; // Metin için alanı genişlet

        std::wstring hsTitle = L"High Scores";
        TextOutW(hDC, xPos, yPos, hsTitle.c_str(), static_cast<int>(hsTitle.length()));
        yPos += yIncrement;

        int rank = 1;
        for (const auto& entry : g_HighScores)
        {
            std::wstring wTimestamp(entry.timestamp.begin(), entry.timestamp.end());
            std::wstring scoreText = std::to_wstring(rank) + L". " + std::to_wstring(entry.score) + L"  (" + wTimestamp + L")";
            TextOutW(hDC, xPos, yPos, scoreText.c_str(), static_cast<int>(scoreText.length()));
            yPos += yIncrement;
            rank++;
        }

        // YENİ: "Tekrar Oyna" talimatını ekle
        yPos += 20; // Yüksek skor listesinin altına biraz boşluk bırak
        std::wstring restartText = L"Press SPACE to Play Again";
        SetTextColor(hDC, RGB(255, 255, 150)); // Açık sarı bir renk

        // Metni ekranın ortasına yatay olarak hizala
        RECT rcRestartText = { 0, yPos, window_X, yPos + 30 };
        DrawTextW(hDC, restartText.c_str(), -1, &rcRestartText, DT_CENTER | DT_SINGLELINE);
    }

    SelectObject(hDC, hOldFont);
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

void HandleKeys() {

    // YENİ: Oyuncu öldüğünde yeniden başlatmak için SPACE tuşunu kontrol et
    if (charSprite->IsDead() && GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        RestartGame();
    }
}

void MouseButtonDown(int x, int y, BOOL bLeft)
{
    if (bLeft && camera && charSprite)
    {
        Player* pPlayerCasted = static_cast<Player*>(charSprite);
        if (pPlayerCasted && pPlayerCasted->IsDead()) return;

        int targetWorldX = x + camera->x;
        int targetWorldY = y + camera->y;

        if (pPlayerCasted)
        {
            pPlayerCasted->Fire(targetWorldX, targetWorldY);
        }
    }
}

void MouseButtonUp(int x, int y, BOOL bLeft) {}

void MouseMove(int x, int y)
{
    if (fovEffect)
        fovEffect->UpdateMousePos(x, y);
}

void HandleJoystick(JOYSTATE jsJoystickState) {}

void GenerateMaze(Bitmap* tileBit)
{
    if (!mazeGenerator || !wallBitmap || !game_engine || TILE_SIZE == 0) return;

    mazeGenerator->generateMaze();
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

    CleanupLevel();

    mazeGenerator->SetupLevel(level);
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

            case TileType::START_POINT:
            case TileType::PATH:
            default:
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
                continue;
            }

            if (itemBitmap)
            {
                Sprite* item = new Sprite(itemBitmap, rcBounds, BA_STOP, SPRITE_TYPE_GENERIC);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
            }
            else if (static_cast<TileType>(tileValue) != TileType::WALL) {
                if (floorBitmap) AddNonCollidableTile(posX, posY, floorBitmap);
            }
        }
    }

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
    if (instance)
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

    if (charSprite) {
        Player* castedPlayer = static_cast<Player*>(charSprite);
        if (castedPlayer && castedPlayer->IsDead()) {
            if ((pSpriteHitter == charSprite && hitteeType == SPRITE_TYPE_WALL) ||
                (pSpriteHittee == charSprite && hitterType == SPRITE_TYPE_WALL)) {
                return TRUE;
            }
            return FALSE;
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

    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_ENEMY) { pSpriteHitter->Kill(); pSpriteHittee->Kill(); if (charSprite) static_cast<Player*>(charSprite)->AddScore(10); return FALSE; }
        if (hitteeType == SPRITE_TYPE_PLAYER) return FALSE;
    }
    else if (hitteeType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_ENEMY) { pSpriteHittee->Kill(); pSpriteHitter->Kill(); if (charSprite) static_cast<Player*>(charSprite)->AddScore(10); return FALSE; }
        if (hitterType == SPRITE_TYPE_PLAYER) return FALSE;
    }

    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_PLAYER && pSpriteHittee == charSprite) {
            pSpriteHitter->Kill();
            static_cast<Player*>(pSpriteHittee)->TakeDamage(12);
            return FALSE;
        }
        if (hitteeType == SPRITE_TYPE_ENEMY) return FALSE;
    }
    else if (hitteeType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_PLAYER && pSpriteHitter == charSprite) {
            pSpriteHittee->Kill();
            static_cast<Player*>(pSpriteHitter)->TakeDamage(12);
            return FALSE;
        }
        if (hitterType == SPRITE_TYPE_ENEMY) return FALSE;
    }

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
            int requiredKeys = std::min(4, currentLevel);
            if (pPlayer->GetKeys() >= requiredKeys) isLevelFinished = true;
            return FALSE;
        }

        if (otherType == SPRITE_TYPE_ENEMY) {
            Enemy* pEnemy = static_cast<Enemy*>(pOtherSpriteForPlayer);
            if (pEnemy) {
                if (pEnemy->GetEnemyType() == EnemyType::CHASER) pPlayer->TakeDamage(1);
                else if (pEnemy->GetEnemyType() == EnemyType::TURRET) pPlayer->TakeDamage(1);
            }
            return TRUE;
        }

        if (otherType == SPRITE_TYPE_WALL) return TRUE;
    }

    return FALSE;
}

// MODIFIED: This function is now called AFTER the level transition screen.
void OnLevelComplete()
{
    currentLevel++;
    // CleanupLevel is called by GenerateLevel now to ensure correct order
    GenerateLevel(currentLevel);
    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
}

void CleanupLevel()
{
    if (!game_engine) return;
    nonCollidableTiles.clear();

    if (charSprite) {
        game_engine->RemoveSprite(charSprite);
    }
    game_engine->CleanupSprites();

    if (charSprite) {
        game_engine->AddSprite(charSprite);
        charSprite->ResetKeys();
    }
}



// DEĞİŞTİRİLDİ: Dosyadan zaman damgalı yüksek skorları yükler.
void LoadHighScores()
{
    std::ifstream file(HIGH_SCORE_FILE);
    g_HighScores.clear();
    std::string line;
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string scoreStr, timestamp;

            // Satırı ayrıştır: skor,zaman_damgası
            if (std::getline(ss, scoreStr, ',') && std::getline(ss, timestamp))
            {
                try {
                    int score = std::stoi(scoreStr);
                    g_HighScores.push_back({ score, timestamp });
                }
                catch (...) { /* Bozuk satırları yoksay */ }
            }
        }
        file.close();
    }
    // Skorları azalan sırada sırala
    std::sort(g_HighScores.rbegin(), g_HighScores.rend());
}

// DEĞİŞTİRİLDİ: Dosyaya zaman damgalı yüksek skorları kaydeder.
void SaveHighScores()
{
    std::ofstream file(HIGH_SCORE_FILE);
    if (file.is_open())
    {
        for (const auto& entry : g_HighScores)
        {
            file << entry.score << "," << entry.timestamp << std::endl;
        }
        file.close();
    }
}

// DEĞİŞTİRİLDİ: Yeni skoru zaman damgasıyla ekler, sıralar, listeyi kırpar ve kaydeder.
void CheckAndSaveScore(int finalScore)
{
    HighScoreEntry newEntry = { finalScore, GetCurrentTimestamp() };
    g_HighScores.push_back(newEntry);
    std::sort(g_HighScores.rbegin(), g_HighScores.rend()); // Skora göre azalan sıralama

    if (g_HighScores.size() > MAX_HIGH_SCORES)
    {
        g_HighScores.resize(MAX_HIGH_SCORES);
    }

    SaveHighScores();
}

std::string GetCurrentTimestamp() {
    std::time_t t = std::time(nullptr);
    char buffer[100];
    // Format: YYYY-MM-DD HH:MM:SS
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t))) {
        return std::string(buffer);
    }
    return ""; // Hata durumunda boş string döndür
}

// YENİ: Oyunu yeniden başlatan ana fonksiyon
void RestartGame()
{
    if (!charSprite || !game_engine || !mazeGenerator) return;

    // 1. Oyuncu durumunu sıfırla
    static_cast<Player*>(charSprite)->Reset();

    // 2. Oyun durumunu sıfırla
    g_bScoreSaved = false;
    currentLevel = 1;

    // 3. Eski seviyeyi temizle
    CleanupLevel(); // Bu fonksiyon zaten oyuncu dışındaki tüm spriteları temizler

    // 4. Yeni seviye 1'i oluştur
    GenerateLevel(currentLevel); // Bu aynı zamanda oyuncuyu başlangıç pozisyonuna yerleştirir

    // 6. Zamanlayıcıları sıfırla
    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
}
