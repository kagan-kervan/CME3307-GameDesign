//
// Created by ahmet on 3.06.2025.
//
#include <string> // Bu satýrý en baþa ekleyin
#include "Game.h"
#include <vector>
#include "Enemy.h"

//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0,0,4000,4000 };

GameEngine* game_engine;
Sprite* charSprite;
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
Bitmap* _pPlayerMissileBitmap;
HINSTANCE   instance;
int window_X, window_Y;
extern RECT globalBounds; // Make sure globalBounds is accessible
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

void GameStart(HWND hWindow)
{
    srand(GetTickCount());
    offscreenDC = CreateCompatibleDC(GetDC(hWindow));
    offscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        game_engine->GetWidth(), game_engine->GetHeight());
    SelectObject(offscreenDC, offscreenBitmap);


    HDC hDC = GetDC(hWindow);
    Bitmap* grassBit = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    _pPlayerMissileBitmap = new Bitmap(hDC, IDB_MISSILE, instance);
    TILE_SIZE = wallBitmap->GetHeight();
    charBitmap = new Bitmap(hDC, IDB_BITMAP3, instance);
    background = new Background(window_X, window_Y, RGB(0, 0, 0));

    // DONMAYI AZALTMAK ÝÇÝN: Labirent boyutunu test için makul bir seviyeye getirdim.
    // Performansýn iyi olduðundan emin olunca tekrar büyütebilirsiniz.
    mazeGenerator = new MazeGenerator(15, 15);
    GenerateMaze(grassBit);

    // Player can be created after maze
    charSprite = new Player(charBitmap, mazeGenerator);

    // Spawn player in a guaranteed open space
    int playerX, playerY;
    do {
        playerX = (rand() % (25 * 2));
        playerY = (rand() % (25 * 2));
    } while (mazeGenerator->IsWall(playerX, playerY));
    charSprite->SetPosition(playerX * TILE_SIZE, playerY * TILE_SIZE);

    game_engine->AddSprite(charSprite);

    // Initialize Camera and FOV AFTER player is positioned
    camera = new Camera(0, 0, window_X, window_Y);
    CenterCameraOnSprite(charSprite); // Initial camera position
    fovEffect = new FOVBackground(charSprite, 90, 150);

    _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, instance);
    _pEnemyMissileBitmap = new Bitmap(hDC, IDB_BMISSILE, instance);

    // Create and add multiple enemies
 // Create and add multiple enemies
    // Düþmanlarý oluþtur
    for (int i = 0; i < 5; i++) // Sayýyý test için azalttým
    {
        EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;

        // HAREKET HATASI DÜZELTÝLDÝ: Düþmaný tüm harita sýnýrlarýyla (globalBounds) oluþturuyoruz.
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
    delete _pPlayerMissileBitmap;
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
    // Draw background with camera offset
    background->Draw(hDC, camera->x, camera->y);

    CenterCameraOnSprite(charSprite);

    // Draw all sprites that are visible in the camera's viewport
    RECT camRect = { camera->x, camera->y, camera->x + camera->width, camera->y + camera->height };



    // Draw all non-collidable tiles
    for (const auto& tile : nonCollidableTiles) {
        // Adjust for camera offset if needed
        tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    // Draw all sprites with camera offset
    for (Sprite* sprite : game_engine->GetSprites()) {
        sprite->Draw(hDC, camera->x, camera->y);
    }
    fovEffect->Draw(hDC, camera->x, camera->y);
}

void GameCycle()
{
    // Update the background
    background->Update();
    game_engine->UpdateSprites(); // This updates player and enemies

    // MOUSE BUG FIX: Pass camera coordinates to the FOV update function.
    if (fovEffect && camera) {
        fovEffect->Update(camera->x, camera->y);
    }

    // Obtain a device context for repainting the game
    HWND  hWindow = game_engine->GetWindow();
    HDC   hDC = GetDC(hWindow);

    // Paint the game to the offscreen device context
    // Center camera on sprite
    CenterCameraOnSprite(charSprite);

    GamePaint(offscreenDC);
    // Blit the offscreen bitmap to the game screen
    BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
        offscreenDC, 0, 0, SRCCOPY);

    // Cleanup
    ReleaseDC(hWindow, hDC);
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

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
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
    return TRUE;
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