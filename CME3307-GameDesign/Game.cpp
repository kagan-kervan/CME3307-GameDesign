#include <string> 
#include "Game.h"
#include <vector>
#include "Enemy.h" 
#include <random>
#include <limits>    
#include <cmath>     
#include <algorithm> 
#include <fstream>   
#include <iostream>  
#include <sstream>   
#include <ctime>     
#pragma warning(disable : 4996) 

#undef max           
#undef min

// YENİ: LerpAngle fonksiyonunun tanımı (gövdesi) buraya taşındı.
#ifndef PI
#define PI 3.14159265358979323846
#endif
double LerpAngle(double a, double b, float t)
{
    double diff = b - a;
    if (diff > PI) diff -= 2 * PI;
    if (diff < -PI) diff += 2 * PI;
    return a + diff * t;
}
//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0, 0, 4000, 4000 };

DWORD g_dwLastSpawnTime = 0;
const DWORD ENEMY_SPAWN_INTERVAL = 10000;

DWORD g_dwLastClosestEnemySpawnTime = 0;
const DWORD CLOSEST_ENEMY_SPAWN_INTERVAL = 5000;

DWORD g_dwLastRobotTurretSpawnTime = 0; // YENİ
const DWORD ROBOT_TURRET_SPAWN_INTERVAL = 20000; // YENİ: 20 saniye

HCURSOR g_hCrosshairCursor = NULL;
GameEngine* game_engine;
Player* charSprite;
MazeGenerator* mazeGenerator;
Bitmap* _pEnemyBitmap = nullptr;
Bitmap* _pTurretEnemyBitmap = nullptr;
Bitmap* _pRobotTurretEnemyBitmap = nullptr; // YENİ
Bitmap* _pEnemyMissileBitmap = nullptr;
FOVBackground* fovEffect;
int TILE_SIZE;
Camera* camera;
HDC offscreenDC;
HBITMAP offscreenBitmap;
Background* background;
Bitmap* wallBitmap = nullptr;
Bitmap* charBitmap = nullptr;
HINSTANCE instance;
int window_X, window_Y;
Bitmap* _pPlayerMissileBitmap = nullptr;

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

bool  g_bInLevelTransition = false;
DWORD g_dwLevelTransitionStartTime = 0;
const DWORD LEVEL_TRANSITION_DURATION = 3000;
HFONT g_hUIFont = NULL;
HFONT g_hBigFont = NULL;

const char* HIGH_SCORE_FILE = "highscores.txt";
const int     MAX_HIGH_SCORES = 5;
std::vector<HighScoreEntry> g_HighScores;
bool          g_bScoreSaved = false;


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
    if (!game_engine) return;
    g_hCrosshairCursor = LoadCursor(NULL, IDC_CROSS);
    if (g_hCrosshairCursor == NULL) {
        OutputDebugString(TEXT("Nişangah imleci yüklenemedi!\n"));
    }

    srand(GetTickCount());
    offscreenDC = CreateCompatibleDC(GetDC(hWindow));
    offscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        game_engine->GetWidth(), game_engine->GetHeight());
    SelectObject(offscreenDC, offscreenBitmap);

    HDC hDC = GetDC(hWindow);
    LoadBitmaps(hDC);

    LoadHighScores();
    g_bScoreSaved = false;

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
    mazeGenerator = new MazeGenerator(15, 15);

    if (charBitmap && mazeGenerator)
        charSprite = new Player(charBitmap, mazeGenerator);

    currentLevel = 1;
    GenerateLevel(currentLevel);

    if (charSprite)
        game_engine->AddSprite(charSprite);

    if (charSprite)
        camera = new Camera(charSprite, window_X, window_Y);

    if (charSprite)
        fovEffect = new FOVBackground(charSprite, 90, 350, 75);

    // Başlangıç düşmanları
    if (mazeGenerator && (_pEnemyBitmap || _pTurretEnemyBitmap || _pRobotTurretEnemyBitmap) && charSprite && TILE_SIZE > 0)
    {
        const auto& mazeData = mazeGenerator->GetMaze();
        if (!mazeData.empty() && !mazeData[0].empty())
        {
            // CHASER ve TURRET düşmanları için referans bitmap (boyut için)
            Bitmap* referenceBitmapForSize = _pEnemyBitmap ? _pEnemyBitmap : _pTurretEnemyBitmap;
            if (!referenceBitmapForSize && !_pRobotTurretEnemyBitmap) { // RobotTurret de bir referans olabilir
                if (game_engine) game_engine->ErrorQuit(TEXT("Düşman boyutları için referans bitmap bulunamadı!"));
                return;
            }
            if (!referenceBitmapForSize) referenceBitmapForSize = _pRobotTurretEnemyBitmap;


            int enemySpriteWidthInTiles = (referenceBitmapForSize->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
            int enemySpriteHeightInTiles = (referenceBitmapForSize->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

            // Mevcut 5 düşmanı spawn et
            for (int i = 0; i < 5; i++)
            {
                EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER; // İlk 2'si turret, kalanı chaser
                Bitmap* selectedEnemyBitmap = nullptr;

                if (type == EnemyType::TURRET) {
                    selectedEnemyBitmap = _pTurretEnemyBitmap;
                }
                else {
                    selectedEnemyBitmap = _pEnemyBitmap;
                }

                if (!selectedEnemyBitmap) selectedEnemyBitmap = _pEnemyBitmap;
                if (!selectedEnemyBitmap) {
                    OutputDebugString(TEXT("Uygun düşman bitmap'i bulunamadı, düşman oluşturulamadı.\n"));
                    continue;
                }

                Enemy* pEnemy = new Enemy(selectedEnemyBitmap, globalBounds, BA_STOP,
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
                    delete pEnemy;
                }
            }

            // YENİ: Başlangıçta 10 adet ROBOT_TURRET spawn et
            if (_pRobotTurretEnemyBitmap) {
                // Robot turret için boyutları al (bitmap farklı olabilir)
                int robotTurretWidthInTiles = (_pRobotTurretEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
                int robotTurretHeightInTiles = (_pRobotTurretEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

                for (int i = 0; i < 10; i++) {
                    Enemy* pRobotEnemy = new Enemy(_pRobotTurretEnemyBitmap, globalBounds, BA_STOP,
                        mazeGenerator, charSprite, EnemyType::ROBOT_TURRET);
                    // pRobotEnemy->SetNumFrames(8); // Enemy constructor'ında zaten yapılıyor

                    int ex, ey;
                    int tryCount = 0;
                    const int maxSpawnTries = 50;
                    do {
                        ex = (rand() % (static_cast<int>(mazeData[0].size()) - robotTurretWidthInTiles + 1));
                        ey = (rand() % (static_cast<int>(mazeData.size()) - robotTurretHeightInTiles + 1));
                        tryCount++;
                    } while (!IsAreaClearForSpawn(ex, ey, robotTurretWidthInTiles, robotTurretHeightInTiles) && tryCount < maxSpawnTries);

                    if (tryCount < maxSpawnTries) {
                        pRobotEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE);
                        game_engine->AddSprite(pRobotEnemy);
                    }
                    else {
                        delete pRobotEnemy;
                    }
                }
            }
        }
    }
    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
    g_dwLastRobotTurretSpawnTime = GetTickCount(); // YENİ
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


    delete _pEnemyBitmap; _pEnemyBitmap = nullptr;
    delete _pTurretEnemyBitmap; _pTurretEnemyBitmap = nullptr;
    delete _pRobotTurretEnemyBitmap; _pRobotTurretEnemyBitmap = nullptr; // YENİ
    delete _pEnemyMissileBitmap; _pEnemyMissileBitmap = nullptr;
    delete background; background = nullptr;
    delete wallBitmap; wallBitmap = nullptr;
    delete charBitmap; charBitmap = nullptr;
    delete healthPWBitmap; healthPWBitmap = nullptr;
    delete ammoPWBitmap; ammoPWBitmap = nullptr;
    delete pointPWBitmap; pointPWBitmap = nullptr;
    delete armorPWBitmap; armorPWBitmap = nullptr;
    delete floorBitmap; floorBitmap = nullptr;
    delete keyBitmap; keyBitmap = nullptr;
    delete endPointBitmap; endPointBitmap = nullptr;
    delete _pPlayerMissileBitmap; _pPlayerMissileBitmap = nullptr;
    delete secondWeaponBitmap; secondWeaponBitmap = nullptr;

    delete mazeGenerator; mazeGenerator = nullptr;
    delete camera; camera = nullptr;
    delete fovEffect; fovEffect = nullptr;

    if (g_hUIFont) DeleteObject(g_hUIFont);
    if (g_hBigFont) DeleteObject(g_hBigFont);
}

void GameActivate(HWND hWindow) {}
void GameDeactivate(HWND hWindow) {}

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

    if (fovEffect && camera && !charSprite->IsDead())
        fovEffect->Draw(hDC, camera->x, camera->y);

    DrawUI(hDC);
}

void GameCycle()
{
    if (!game_engine) return;

    if (g_bInLevelTransition)
    {
        if (GetTickCount() - g_dwLevelTransitionStartTime > LEVEL_TRANSITION_DURATION)
        {
            g_bInLevelTransition = false;
            OnLevelComplete();
        }
    }
    else
    {
        if (charSprite) {
            Player* pPlayer = static_cast<Player*>(charSprite);
            if (pPlayer && pPlayer->IsDead()) {
                if (!g_bScoreSaved)
                {
                    CheckAndSaveScore(pPlayer->GetScore());
                    g_bScoreSaved = true;
                }
            }
            else
            {
                if (camera) camera->Update();
                if (background) background->Update();

                game_engine->UpdateSprites();

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

                // YENİ: Robot Turret spawn kontrolü
                if (GetTickCount() - g_dwLastRobotTurretSpawnTime > ROBOT_TURRET_SPAWN_INTERVAL)
                {
                    SpawnRobotTurretEnemy();
                    g_dwLastRobotTurretSpawnTime = GetTickCount();
                }


                if (isLevelFinished)
                {
                    isLevelFinished = false;
                    g_bInLevelTransition = true;
                    g_dwLevelTransitionStartTime = GetTickCount();
                }

                if (fovEffect && camera) fovEffect->Update(camera->x, camera->y);
            }
        }
    }

    HWND hWindow = game_engine->GetWindow();
    if (hWindow)
    {
        HDC hDC = GetDC(hWindow);
        if (hDC)
        {
            GamePaint(offscreenDC);
            BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
                offscreenDC, 0, 0, SRCCOPY);
            ReleaseDC(hWindow, hDC);
        }
    }
}

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

    RECT screenRect = { 0, 0, window_X, window_Y };

    int staminaBarWidth = 200;
    int barHeight = 15;
    int barX = 10;
    int staminaBarY = screenRect.bottom - 65;

    float staminaPercent = pPlayer->GetStamina() / pPlayer->GetMaxStamina();
    if (staminaPercent < 0.0f) staminaPercent = 0.0f;
    if (staminaPercent > 1.0f) staminaPercent = 1.0f;


    HBRUSH hRedBrush = CreateSolidBrush(RGB(70, 70, 0));
    RECT bgRect = { barX, staminaBarY, barX + staminaBarWidth, staminaBarY + barHeight };
    FillRect(hDC, &bgRect, hRedBrush);
    DeleteObject(hRedBrush);

    HBRUSH hYellowBrush = CreateSolidBrush(RGB(255, 255, 0));
    RECT fgRect = { barX, staminaBarY, barX + (int)(staminaBarWidth * staminaPercent), staminaBarY + barHeight };
    FillRect(hDC, &fgRect, hYellowBrush);
    DeleteObject(hYellowBrush);

    SelectObject(hDC, GetStockObject(NULL_BRUSH));
    SelectObject(hDC, GetStockObject(WHITE_PEN));
    Rectangle(hDC, bgRect.left - 1, bgRect.top - 1, bgRect.right + 1, bgRect.bottom + 1);

    const WeaponStats& stats = pPlayer->GetCurrentWeaponStats();
    std::wstring ammoText;
    if (pPlayer->IsReloading()) {
        ammoText = L"RELOADING...";
        SetTextColor(hDC, RGB(255, 100, 100));
    }
    else {
        std::wstring totalAmmoStr = (stats.totalAmmo == -1) ? L"∞" : std::to_wstring(stats.totalAmmo);
        ammoText = L"Ammo: " + std::to_wstring(stats.currentAmmoInClip) + L" / " + totalAmmoStr;
        SetTextColor(hDC, RGB(255, 255, 255));
    }

    RECT ammoRect = { 10, screenRect.bottom - 40, 200, screenRect.bottom - 10 };
    DrawTextW(hDC, ammoText.c_str(), -1, &ammoRect, DT_LEFT | DT_SINGLELINE);

    if (pPlayer->IsDead()) {
        SetTextColor(hDC, RGB(255, 0, 0));
        SelectObject(hDC, g_hBigFont);

        std::wstring gameOverText = L"GAME OVER";
        DrawTextW(hDC, gameOverText.c_str(), -1, &screenRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hDC, g_hUIFont);
        SetTextColor(hDC, RGB(220, 220, 220));

        yPos = screenRect.bottom / 2 + 60;
        yIncrement = 25;
        xPos = window_X / 2 - 150;

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

        yPos += 20;
        std::wstring restartText = L"Press SPACE to Play Again";
        SetTextColor(hDC, RGB(255, 255, 150));
        RECT rcRestartText = { 0, yPos, window_X, yPos + 30 };
        DrawTextW(hDC, restartText.c_str(), -1, &rcRestartText, DT_CENTER | DT_SINGLELINE);
    }

    SelectObject(hDC, hOldFont);
}

void SpawnEnemyNearPlayer()
{
    if (!mazeGenerator || !charSprite || (!_pEnemyBitmap && !_pTurretEnemyBitmap) || !game_engine || TILE_SIZE == 0) return;

    RECT playerPos = charSprite->GetPosition();
    int playerTileX = playerPos.left / TILE_SIZE;
    int playerTileY = playerPos.top / TILE_SIZE;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(-7, 7);

    int spawnTileX, spawnTileY;
    int tryCount = 0;
    const int maxTries = 30;

    const auto& mazeData = mazeGenerator->GetMaze();
    if (mazeData.empty() || mazeData[0].empty()) return;
    int mazeWidthInTiles = static_cast<int>(mazeData[0].size());
    int mazeHeightInTiles = static_cast<int>(mazeData.size());

    // DEĞİŞİKLİK: RobotTurret hariç diğerleri için referans bitmap
    Bitmap* referenceBitmapForSize = _pEnemyBitmap ? _pEnemyBitmap : _pTurretEnemyBitmap;
    if (!referenceBitmapForSize) return;

    int enemySpriteWidthInTiles = (referenceBitmapForSize->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
    int enemySpriteHeightInTiles = (referenceBitmapForSize->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

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
        // DEĞİŞİKLİK: Sadece CHASER ve TURRET spawn edilecek, ROBOT_TURRET kendi fonksiyonuyla spawn olur.
        EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
        Bitmap* selectedEnemyBitmap = nullptr;

        if (type == EnemyType::TURRET) {
            selectedEnemyBitmap = _pTurretEnemyBitmap;
        }
        else { // CHASER
            selectedEnemyBitmap = _pEnemyBitmap;
        }

        if (!selectedEnemyBitmap) selectedEnemyBitmap = _pEnemyBitmap;
        if (!selectedEnemyBitmap) {
            OutputDebugString(TEXT("SpawnEnemyNearPlayer: Uygun düşman bitmap'i bulunamadı.\n"));
            return;
        }

        Enemy* pEnemy = new Enemy(selectedEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);
        pEnemy->SetPosition(spawnTileX * TILE_SIZE, spawnTileY * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}

// YENİ: Robot Turret Düşmanı Spawn Fonksiyonu
void SpawnRobotTurretEnemy()
{
    if (!mazeGenerator || !charSprite || !_pRobotTurretEnemyBitmap || !game_engine || TILE_SIZE == 0) return;

    const auto& mazeData = mazeGenerator->GetMaze();
    if (mazeData.empty() || mazeData[0].empty()) return;
    int mazeWidthInTiles = static_cast<int>(mazeData[0].size());
    int mazeHeightInTiles = static_cast<int>(mazeData.size());

    int enemySpriteWidthInTiles = (_pRobotTurretEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
    int enemySpriteHeightInTiles = (_pRobotTurretEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

    int spawnTileX, spawnTileY;
    int tryCount = 0;
    const int maxTries = 50; // Daha fazla deneme şansı verelim, çünkü haritanın herhangi bir yerinde olabilir

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrX(0, mazeWidthInTiles - enemySpriteWidthInTiles);
    std::uniform_int_distribution<> distrY(0, mazeHeightInTiles - enemySpriteHeightInTiles);


    do
    {
        spawnTileX = distrX(gen);
        spawnTileY = distrY(gen);
        tryCount++;
    } while (!IsAreaClearForSpawn(spawnTileX, spawnTileY, enemySpriteWidthInTiles, enemySpriteHeightInTiles) && tryCount < maxTries);

    if (tryCount < maxTries)
    {
        Enemy* pEnemy = new Enemy(_pRobotTurretEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, EnemyType::ROBOT_TURRET);
        // pEnemy->SetNumFrames(8); // Constructor'da ayarlandı
        pEnemy->SetPosition(spawnTileX * TILE_SIZE, spawnTileY * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
    else {
        OutputDebugString(TEXT("SpawnRobotTurretEnemy: Uygun spawn noktası bulunamadı.\n"));
    }
}


void SpawnEnemyNearClosest()
{
    if (!mazeGenerator || !charSprite || (!_pEnemyBitmap && !_pTurretEnemyBitmap) || !game_engine || game_engine->GetSprites().empty() || TILE_SIZE == 0) return;

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
            // DEĞİŞİKLİK: RobotTurret bu spawn tipine dahil edilmeyecek.
            if (currentEnemy->GetEnemyType() == EnemyType::ROBOT_TURRET) continue;

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

        Bitmap* referenceBitmapForSize = _pEnemyBitmap ? _pEnemyBitmap : _pTurretEnemyBitmap;
        if (!referenceBitmapForSize) return;

        int enemySpriteWidthInTiles = (referenceBitmapForSize->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
        int enemySpriteHeightInTiles = (referenceBitmapForSize->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

        if (IsAreaClearForSpawn(enemyTileX, enemyTileY, enemySpriteWidthInTiles, enemySpriteHeightInTiles))
        {
            // DEĞİŞİKLİK: Sadece CHASER ve TURRET spawn edilecek
            EnemyType type = (rand() % 2 == 0) ? EnemyType::TURRET : EnemyType::CHASER;
            Bitmap* selectedEnemyBitmap = nullptr;

            if (type == EnemyType::TURRET) {
                selectedEnemyBitmap = _pTurretEnemyBitmap;
            }
            else { // CHASER
                selectedEnemyBitmap = _pEnemyBitmap;
            }
            if (!selectedEnemyBitmap) selectedEnemyBitmap = _pEnemyBitmap;
            if (!selectedEnemyBitmap) {
                OutputDebugString(TEXT("SpawnEnemyNearClosest: Uygun düşman bitmap'i bulunamadı.\n"));
                return;
            }

            Enemy* newEnemy = new Enemy(selectedEnemyBitmap, globalBounds, BA_STOP,
                mazeGenerator, charSprite, type);
            newEnemy->SetPosition(enemyTileX * TILE_SIZE, enemyTileY * TILE_SIZE);
            game_engine->AddSprite(newEnemy);
        }
    }
}

void HandleKeys() {
    if (charSprite && charSprite->IsDead() && GetAsyncKeyState(VK_SPACE) & 0x8000)
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

    if (charSprite) // charSprite'ın Player* olduğunu varsayıyoruz
    {
        // Player* charSprite; olarak tanımlandığı için static_cast'e gerek yok.
        charSprite->UpdateMousePosition(x, y);
    }
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
    if (game_engine) game_engine->PlayMIDISong(TEXT("duke3d27.mid"));
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap)
{
    if (bitmap)
        nonCollidableTiles.push_back({ x, y, bitmap });
}

void LoadBitmaps(HDC hDC)
{
    if (!hDC || !instance) {
        if (game_engine) game_engine->ErrorQuit(TEXT("LoadBitmaps: HDC veya instance null!"));
        return;
    }

    floorBitmap = new Bitmap(hDC, IDB_TILE, instance);
    if (!floorBitmap || floorBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("tile.bmp yÃ¼klenemedi!"));

    wallBitmap = new Bitmap(hDC, IDB_WALL, instance);
    if (!wallBitmap || wallBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("wall.bmp yÃ¼klenemedi!"));

    charBitmap = new Bitmap(hDC, IDB_BITMAP3, instance);
    if (!charBitmap || charBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Oyuncu bitmap'i (IDB_BITMAP3) yüklenemedi!"));

    _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, instance);
    if (!_pEnemyBitmap || _pEnemyBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Genel düşman bitmap'i (IDB_ENEMY) yüklenemedi!"));

    _pTurretEnemyBitmap = new Bitmap(hDC, IDB_TURRET_ENEMY, instance);
    if (!_pTurretEnemyBitmap || _pTurretEnemyBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Turret düşman bitmap'i (IDB_TURRET_ENEMY) yüklenemedi!"));

    // YENİ: Robot Turret bitmap'ini yükle
    _pRobotTurretEnemyBitmap = new Bitmap(hDC, IDB_ROBOT_TURRET_ENEMY, instance);
    if (!_pRobotTurretEnemyBitmap || _pRobotTurretEnemyBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Robot Turret düşman bitmap'i (IDB_ROBOT_TURRET_ENEMY) yüklenemedi!"));


    _pEnemyMissileBitmap = new Bitmap(hDC, IDB_BMISSILE, instance);
    if (!_pEnemyMissileBitmap || _pEnemyMissileBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Düşman mermi bitmap'i (IDB_BMISSILE) yüklenemedi!"));

    _pPlayerMissileBitmap = new Bitmap(hDC, IDB_MISSILE, instance);
    if (!_pPlayerMissileBitmap || _pPlayerMissileBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Oyuncu mermi bitmap'i (IDB_MISSILE) yüklenemedi!"));


    healthPWBitmap = new Bitmap(hDC, IDB_HEALTH, instance);
    if (!healthPWBitmap || healthPWBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Health.bmp yÃ¼klenemedi!"));

    ammoPWBitmap = new Bitmap(hDC, IDB_AMMO, instance);
    if (!ammoPWBitmap || ammoPWBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Ammo.bmp yÃ¼klenemedi!"));

    pointPWBitmap = new Bitmap(hDC, IDB_POINT, instance);
    if (!pointPWBitmap || pointPWBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Point.bmp yÃ¼klenemedi!"));

    armorPWBitmap = new Bitmap(hDC, IDB_ARMOR, instance);
    if (!armorPWBitmap || armorPWBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Armor.bmp yÃ¼klenemedi!"));

    keyBitmap = new Bitmap(hDC, IDB_KEY, instance);
    if (!keyBitmap || keyBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Key.bmp yÃ¼klenemedi!"));

    endPointBitmap = new Bitmap(hDC, IDB_GATE, instance);
    if (!endPointBitmap || endPointBitmap->GetWidth() == 0) if (game_engine) game_engine->ErrorQuit(TEXT("Gate.bmp yÃ¼klenemedi!"));

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

    // DEĞİŞİKLİK: Oyuncu mermisi düşmana çarptığında hasar verme ve can kontrolü
    if (hitterType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_ENEMY) {
            Enemy* pEnemy = static_cast<Enemy*>(pSpriteHittee);
            if (pEnemy) {
                pSpriteHitter->Kill(); // Mermiyi öldür
                pEnemy->TakeDamage(1); // Düşmana 1 hasar ver (mermi hasarı)
                if (game_engine && game_engine->GetInstance()) PlaySound(MAKEINTRESOURCE(IDW_EXPLODE), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);

                if (pEnemy->IsDead()) {
                    if (charSprite) static_cast<Player*>(charSprite)->AddScore(10); // Skor ekle
                    // pSpriteHittee->Kill(); // Enemy::TakeDamage zaten Kill çağırıyor, bu satır gereksiz
                }
            }
            return FALSE;
        }
        if (hitteeType == SPRITE_TYPE_PLAYER) return FALSE;
    }
    else if (hitteeType == SPRITE_TYPE_PLAYER_MISSILE)
    {
        if (hitterType == SPRITE_TYPE_WALL) { pSpriteHittee->Kill(); return FALSE; }
        if (hitterType == SPRITE_TYPE_ENEMY) {
            Enemy* pEnemy = static_cast<Enemy*>(pSpriteHitter);
            if (pEnemy) {
                pSpriteHittee->Kill(); // Mermiyi öldür
                pEnemy->TakeDamage(1); // Düşmana 1 hasar ver
                if (game_engine && game_engine->GetInstance()) PlaySound(MAKEINTRESOURCE(IDW_EXPLODE), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);

                if (pEnemy->IsDead()) {
                    if (charSprite) static_cast<Player*>(charSprite)->AddScore(10);
                    // pSpriteHitter->Kill(); // Enemy::TakeDamage zaten Kill çağırıyor
                }
            }
            return FALSE;
        }
        if (hitterType == SPRITE_TYPE_PLAYER) return FALSE;
    }

    if (hitterType == SPRITE_TYPE_ENEMY_MISSILE)
    {
        if (hitteeType == SPRITE_TYPE_WALL) { pSpriteHitter->Kill(); return FALSE; }
        if (hitteeType == SPRITE_TYPE_PLAYER && pSpriteHittee == charSprite) {
            pSpriteHitter->Kill();
            static_cast<Player*>(pSpriteHittee)->TakeDamage(12);
            PlaySound(MAKEINTRESOURCE(IDW_HIT), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
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
            PlaySound(MAKEINTRESOURCE(IDW_HIT), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
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

        if (pOtherBitmap == keyBitmap) {PlaySound(MAKEINTRESOURCE(IDW_POWERUP), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT); 
        pPlayer->AddKey(); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == healthPWBitmap) { PlaySound(MAKEINTRESOURCE(IDW_POWERUP), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT); 
        pPlayer->AddHealth(20); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == armorPWBitmap) { PlaySound(MAKEINTRESOURCE(IDW_POWERUP), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT); 
        pPlayer->AddArmor(20); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == pointPWBitmap) { PlaySound(MAKEINTRESOURCE(IDW_POWERUP), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT); 
        pPlayer->AddScore(50); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == ammoPWBitmap) { PlaySound(MAKEINTRESOURCE(IDW_POWERUP), game_engine->GetInstance(), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT); 
        pPlayer->AddSecondaryAmmo(10); pOtherSpriteForPlayer->Kill(); return FALSE; }
        if (pOtherBitmap == endPointBitmap) {
            int requiredKeys = std::min(4, currentLevel);
            if (pPlayer->GetKeys() >= requiredKeys) {
                isLevelFinished = true;
                if (game_engine) game_engine->PlayMIDISong(TEXT(""), FALSE);
            }
            return FALSE;
        }

        if (otherType == SPRITE_TYPE_ENEMY) {
            Enemy* pEnemy = static_cast<Enemy*>(pOtherSpriteForPlayer);
            if (pEnemy) {
                if (pEnemy->GetEnemyType() == EnemyType::CHASER) {
                    pPlayer->TakeDamage(1);
                }
            }
            return TRUE;
        }

        if (otherType == SPRITE_TYPE_WALL) return TRUE;
    }

    return FALSE;
}

void OnLevelComplete()
{
    currentLevel++;
    GenerateLevel(currentLevel);
    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
    g_dwLastRobotTurretSpawnTime = GetTickCount(); // YENİ: Seviye geçişinde zamanlayıcıyı sıfırla
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
    std::sort(g_HighScores.rbegin(), g_HighScores.rend());
}

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

void CheckAndSaveScore(int finalScore)
{
    HighScoreEntry newEntry = { finalScore, GetCurrentTimestamp() };
    g_HighScores.push_back(newEntry);
    std::sort(g_HighScores.rbegin(), g_HighScores.rend());

    if (g_HighScores.size() > MAX_HIGH_SCORES)
    {
        g_HighScores.resize(MAX_HIGH_SCORES);
    }

    SaveHighScores();
}

std::string GetCurrentTimestamp() {
    std::time_t t = std::time(nullptr);
    char buffer[100];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t))) {
        return std::string(buffer);
    }
    return "";
}

void RestartGame()
{
    if (!charSprite || !game_engine || !mazeGenerator) return;

    static_cast<Player*>(charSprite)->Reset();
    g_bScoreSaved = false;
    currentLevel = 1;
    isLevelFinished = false;
    g_bInLevelTransition = false;

    CleanupLevel();
    //GenerateLevel(currentLevel); // Bu zaten GameStart içinde çağrılacak gibi duruyor, ya da burada spawn mantığını GameStart'tan ayrı yönet.
                                // Şimdilik GameStart'ın başlangıç düşmanlarını tekrar spawn etmesi için GameStart'ı yeniden çağırmak daha mantıklı.
                                // Ama GenerateLevel ve GameStart'taki düşman spawn mantıkları çakışmamalı.
                                // En temizi, GameStart'ı çağırmak yerine, seviye oluşturma ve başlangıç düşmanlarını spawn etme işlemlerini
                                // buraya almak veya ayrı bir ResetLevelAndSpawnInitialEnemies fonksiyonu yapmak.
                                // Mevcut yapıda GameStart tüm başlangıç kurulumunu yapıyor, o yüzden onu yeniden düzenlemek yerine
                                // burada sadece seviye ve düşmanları resetlemek daha doğru.

    // Oyuncu öldükten sonra başlangıç düşmanlarını tekrar spawn etmek için GameStart'taki spawn mantığını buraya taşıyabiliriz
    // veya GameStart'ı yeniden çağırabiliriz. Yeniden çağırmak bazı şeyleri tekrar başlatabilir.
    // Şimdilik sadece seviyeyi ve zamanlayıcıları resetleyelim.
    // GenerateLevel'den sonra başlangıç düşmanlarını tekrar spawn etmemiz lazım.
    // GameStart'taki gibi bir spawn döngüsü burada da olmalı.

    // Düşmanları ve seviyeyi temizledik, şimdi yeniden oluşturalım.
    GenerateLevel(currentLevel); // Bu, duvarları ve item'ları yerleştirir. Oyuncuyu da başlangıç pozisyonuna koyar.

    // Başlangıç düşmanlarını tekrar spawn edelim (GameStart'taki gibi)
    const auto& mazeData = mazeGenerator->GetMaze();
    if (!mazeData.empty() && !mazeData[0].empty() && TILE_SIZE > 0)
    {
        Bitmap* referenceBitmapForSize = _pEnemyBitmap ? _pEnemyBitmap : _pTurretEnemyBitmap;
        if (!referenceBitmapForSize && !_pRobotTurretEnemyBitmap) {
            if (game_engine) game_engine->ErrorQuit(TEXT("Yeniden başlatmada düşman boyutları için referans bitmap bulunamadı!"));
            return;
        }
        if (!referenceBitmapForSize) referenceBitmapForSize = _pRobotTurretEnemyBitmap;

        int enemySpriteWidthInTiles = (referenceBitmapForSize->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
        int enemySpriteHeightInTiles = (referenceBitmapForSize->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;

        for (int i = 0; i < 5; i++) {
            EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;
            Bitmap* selectedEnemyBitmap = (type == EnemyType::TURRET) ? _pTurretEnemyBitmap : _pEnemyBitmap;
            if (!selectedEnemyBitmap) selectedEnemyBitmap = _pEnemyBitmap;
            if (selectedEnemyBitmap) {
                Enemy* pEnemy = new Enemy(selectedEnemyBitmap, globalBounds, BA_STOP, mazeGenerator, charSprite, type);
                int ex, ey, tryCount = 0;
                do {
                    ex = (rand() % (static_cast<int>(mazeData[0].size()) - enemySpriteWidthInTiles + 1));
                    ey = (rand() % (static_cast<int>(mazeData.size()) - enemySpriteHeightInTiles + 1));
                    tryCount++;
                } while (!IsAreaClearForSpawn(ex, ey, enemySpriteWidthInTiles, enemySpriteHeightInTiles) && tryCount < 50);
                if (tryCount < 50) { pEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE); game_engine->AddSprite(pEnemy); }
                else { delete pEnemy; }
            }
        }
        if (_pRobotTurretEnemyBitmap) {
            int robotTurretWidthInTiles = (_pRobotTurretEnemyBitmap->GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
            int robotTurretHeightInTiles = (_pRobotTurretEnemyBitmap->GetHeight() + TILE_SIZE - 1) / TILE_SIZE;
            for (int i = 0; i < 10; i++) {
                Enemy* pRobotEnemy = new Enemy(_pRobotTurretEnemyBitmap, globalBounds, BA_STOP, mazeGenerator, charSprite, EnemyType::ROBOT_TURRET);
                int ex, ey, tryCount = 0;
                do {
                    ex = (rand() % (static_cast<int>(mazeData[0].size()) - robotTurretWidthInTiles + 1));
                    ey = (rand() % (static_cast<int>(mazeData.size()) - robotTurretHeightInTiles + 1));
                    tryCount++;
                } while (!IsAreaClearForSpawn(ex, ey, robotTurretWidthInTiles, robotTurretHeightInTiles) && tryCount < 50);
                if (tryCount < 50) { pRobotEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE); game_engine->AddSprite(pRobotEnemy); }
                else { delete pRobotEnemy; }
            }
        }
    }


    g_dwLastSpawnTime = GetTickCount();
    g_dwLastClosestEnemySpawnTime = GetTickCount();
    g_dwLastRobotTurretSpawnTime = GetTickCount();


    if (game_engine) game_engine->PlayMIDISong(TEXT("tribal-sci-fi.mid"), TRUE);
}