//
// Created by ahmet on 3.06.2025.
//

#include "Game.h"

std::vector<Tile> nonCollidableTiles; // Definition
//Global map bounds for accessing whole map
RECT globalBounds = { 0,0,2000,2000 };

BOOL GameInitialize(HINSTANCE hInstance)
{
    window_X = 1500;
    window_Y = 700;
    // Create the game engine
    game_engine = new GameEngine(hInstance, TEXT("Maze Game"), TEXT("Maze Game"),
        IDI_SPACEOUT, IDI_SPACEOUT_SM, window_X, window_Y); // Pencere boyutunu büyüttük
    if (game_engine == NULL)
        return FALSE;

    // Set the frame rate
    game_engine->SetFrameRate(30);

    // Store the instance handle
    instance = hInstance;

    /*AllocConsole();
    freopen("CONOUT$", "w", stdout);*/

    return TRUE;
}

void GameStart(HWND hWindow)
{
    // Seed the random number generator
    srand(GetTickCount());

    // Create the offscreen device context and bitmap
    offscreenDC = CreateCompatibleDC(GetDC(hWindow));
    offscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        game_engine->GetWidth(), game_engine->GetHeight());
    SelectObject(offscreenDC, offscreenBitmap);

    // Create and load the bitmaps
    HDC hDC = GetDC(hWindow);
    tileBit = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    LoadBitmaps(hDC);
    TILE_SIZE = wallBitmap->GetHeight();
    charBitmap = new Bitmap(hDC, "char_pistol.bmp");
    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, instance);
    camera = new Camera(0, 0, window_X, window_Y);
    //mazeGenerator = new MazeGenerator(12, 12);
    //GenerateMaze(tileBit);
    //Create player
    charSprite = new Player(charBitmap, mazeGenerator);
    SetupLevel(1);
    //fovEffect = new FOVBackground(charSprite, 90, 150);

    //// Birkaç düþman oluþtur ve ekle
    //for (int i = 0; i < 1; i++)
    //{
    //    Enemy* pEnemy = new Enemy(_pEnemyBitmap, mazeGenerator, charSprite);
    //    // Düþmanýn duvarda baþlamadýðýndan emin ol
    //    int ex, ey;
    //    do {
    //        ex = (rand() % (MAZE_WIDTH - 2) + 1);
    //        ey = (rand() % (MAZE_HEIGHT - 2) + 1);
    //    } while (mazeGenerator->IsWall(ex, ey));
    //    pEnemy->SetPosition(ex*TILE_SIZE-5, ey*TILE_SIZE-5);
    //    game_engine->AddSprite(pEnemy);
    //    mazeGenerator->setValue(ex, ey, 2);
    //}
}

void LoadBitmaps(HDC hDC) {
    // Load all the bitmaps needed for the game
    keyBitmap = new Bitmap(hDC, TEXT("Key.bmp"));
    healthBitmap = new Bitmap(hDC, TEXT("Health.bmp"));
    armorBitmap = new Bitmap(hDC, TEXT("Armor.bmp"));
    ammoBitmap = new Bitmap(hDC, TEXT("Ammo.bmp"));
    weaponBitmap = new Bitmap(hDC, TEXT("Weapon.bmp"));
    pointBitmap = new Bitmap(hDC, TEXT("Point.bmp"));
}

void SetupLevel(int levelNumber) {
    // Clean up existing sprites
    for (Sprite* sprite : levelSprites) {
        delete sprite;
    }
    levelSprites.clear();

    // Create new level
    if (currentLevel) {
        delete currentLevel;
    }
    currentLevel = new Level(levelNumber);

    // Create sprites for maze elements
    const int TILE_SIZE = 50; // Adjust based on your bitmap sizes

    // Reset player position to level start
    POINT startPos = currentLevel->GetStartPosition();
    // TODO: Set player position
    charSprite->SetPosition(startPos.x * TILE_SIZE-5, startPos.y * TILE_SIZE-5);
    charSprite->SetZOrder(1);

    // Add key sprites
    for (const POINT& keyPos : currentLevel->GetKeys()) {
        Sprite* keySprite = new Sprite(keyBitmap);
        keySprite->SetPosition(
            keyPos.x * TILE_SIZE-10,
            keyPos.y * TILE_SIZE - 10
        );
        keySprite->SetZOrder(1);
        levelSprites.push_back(keySprite);
        GameEngine::GetEngine()->AddSprite(keySprite);
    }

    // Add item sprites
    for (const auto& item : currentLevel->GetItems()) {
        Sprite* itemSprite = nullptr;

        switch (item.second) {
        case ItemType::HEALTH:
            itemSprite = new Sprite(healthBitmap);
            break;
        case ItemType::ARMOR:
            itemSprite = new Sprite(armorBitmap);
            break;
        case ItemType::WEAPON:
            itemSprite = new Sprite(weaponBitmap);
            break;
        case ItemType::AMMO:
            itemSprite = new Sprite(ammoBitmap);
            break;
        case ItemType::POINTS:
            itemSprite = new Sprite(pointBitmap);
            break;
        }

        if (itemSprite) {
            itemSprite->SetPosition(
                item.first.x * TILE_SIZE - 10,
                item.first.y * TILE_SIZE - 10
            );
            itemSprite->SetZOrder(1);
            levelSprites.push_back(itemSprite);
            GameEngine::GetEngine()->AddSprite(itemSprite);
        }
    }

    // Add end point marker (you'll need to create a suitable bitmap for this)
    POINT endPos = currentLevel->GetEndPosition();
    // Example using key bitmap as placeholder - replace with proper end point bitmap
    Sprite* endSprite = new Sprite(keyBitmap); // Replace with proper end point bitmap
    endSprite->SetPosition(
        endPos.x * TILE_SIZE - 10,
        endPos.y * TILE_SIZE - 10
    );
    endSprite->SetZOrder(1);
    levelSprites.push_back(endSprite);
    GameEngine::GetEngine()->AddSprite(endSprite);
    GenerateMaze(tileBit);
}

void StartLevel(int levelNumber) {
    // Clean up previous level if it exists
    if (currentLevel) {
        delete currentLevel;
    }

    // Create new level
    currentLevel = new Level(levelNumber);

    // Initialize UI elements for new level
    // TODO: Update UI with level information

    // Start level timer
    // The level class already tracks this internally
}

void GameEnd()
{
    // Close the MIDI player for the background music
    game_engine->CloseMIDIPlayer();

    // Cleanup the offscreen device context and bitmap
    DeleteObject(offscreenBitmap);
    DeleteDC(offscreenDC);

    // Cleanup the background
    delete background;

    // Cleanup the sprites
    game_engine->CleanupSprites();
    CleanupBitmaps();

    // Cleanup the game engine
    delete game_engine;
}


void CleanupBitmaps() {
    // Clean up all bitmaps
    delete keyBitmap;
    delete healthBitmap;
    delete armorBitmap;
    delete ammoBitmap;
    delete weaponBitmap;
    delete pointBitmap;
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
        RECT pos = sprite->GetPosition();
        int spriteWidth = sprite->GetBitmap()->GetWidth();
        int spriteHeight = sprite->GetBitmap()->GetHeight();

        RECT spriteRect = pos;
        sprite->Draw(hDC, camera->x, camera->y);
    }
    //fovEffect->Draw(hDC,camera->x,camera->y);
}

void GameCycle()
{
    // Update the background
    background->Update();

    // Update the sprites
    game_engine->UpdateSprites();

    //fovEffect->Update();

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
{/*
    const int CAMERA_SPEED = 25;
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        charSprite->SetVelocity(CAMERA_SPEED, 0);
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        charSprite->SetVelocity(-CAMERA_SPEED, 0);
    if (GetAsyncKeyState(VK_UP) & 0x8000)
        charSprite->SetVelocity(0, CAMERA_SPEED);
    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        charSprite->SetVelocity(0, -CAMERA_SPEED);*/

}

void MouseButtonDown(int x, int y, BOOL bLeft)
{

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
    return TRUE;
}


void GenerateMaze(Bitmap* tileBit) {
    int tile_height = wallBitmap->GetHeight();
    int tile_width = wallBitmap->GetWidth();
    RECT rcBounds = { 0, 0, tile_width * Level::MAZE_SIZE, tile_height * Level::MAZE_SIZE };

    // Iterate through the level's maze array
    for (int y = 0; y < Level::MAZE_SIZE; ++y) {
        for (int x = 0; x < Level::MAZE_SIZE; ++x) {
            int posX = x * tile_width;
            int posY = y * tile_height;

            // Add background tile first
            AddNonCollidableTile(posX, posY, tileBit);

            // If it's a wall (1), add wall sprite
            if (currentLevel->GetMazeValue(x, y) == 1) {
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP);
                wall->SetPosition(pos);
                GameEngine::GetEngine()->AddSprite(wall);
                levelSprites.push_back(wall);
            }
        }
    }
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap) {
    nonCollidableTiles.push_back({ x, y, bitmap });
}

void CenterCameraOnSprite(Sprite* sprite) {
    RECT pos = sprite->GetPosition();
    int spriteWidth = sprite->GetBitmap()->GetWidth();
    int spriteHeight = sprite->GetBitmap()->GetHeight();

    int centerX = pos.left + spriteWidth / 2;
    int centerY = pos.top + spriteHeight / 2;

    int camX = centerX - camera->width / 2;
    int camY = centerY - camera->height / 2;

    camera->SetPosition(camX, camY);
}

//void UpdateLevel() {
//    if (!currentLevel) return;
//
//    int currentTime = GetTickCount();
//
//    // Check if time expired
//    if (currentLevel->IsTimeExpired(currentTime)) {
//        // Handle game over due to time
//        EndLevel();
//        return;
//    }
//
//    // Spawn new enemy wave if needed
//    currentLevel->SpawnEnemyWave(currentTime);
//
//    // Update enemies
//    // TODO: Update enemy sprites based on currentLevel->GetEnemies()
//
//    // Check if level is complete
//    if (currentLevel->IsLevelComplete()) {
//        // Calculate final score
//        int finalScore = currentLevel->CalculateFinalScore(
//            currentTime - currentLevel->GetStartTime(),
//            playerHealth,
//            collectedKeys,
//            enemiesKilled,
//            collectedItems
//        );
//
//        // Handle level completion
//        EndLevel();
//
//        // Start next level after delay
//        Sleep(12000); // 12-second delay between levels
//        StartLevel(currentLevel->GetCurrentLevel() + 1);
//    }
//}


void EndLevel() {
    // Clean up level resources
    if (currentLevel) {
        delete currentLevel;
        currentLevel = nullptr;
    }

    // Reset game state for next level
    // TODO: Reset necessary game state variables
}
