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
    Bitmap* grassBit = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    TILE_SIZE = wallBitmap->GetHeight();
    charBitmap = new Bitmap(hDC, "player.bmp");
    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    camera = new Camera(0, 0, window_X, window_Y);
    mazeGenerator = new MazeGenerator(12, 12);
    GenerateMaze(grassBit);

    //Create player
    charSprite = new Player(charBitmap, mazeGenerator);
    charSprite->SetPosition(12 * TILE_SIZE+5,5 * TILE_SIZE+5);
    camera->SetPosition(charSprite->GetPosition().left, charSprite->GetPosition().top);
    game_engine->AddSprite(charSprite);
    mazeGenerator->setValue(charSprite->GetPosition().top / TILE_SIZE - 5, charSprite->GetPosition().left / TILE_SIZE - 5, 1);

    _pEnemyBitmap = new Bitmap(hDC, "enemy.bmp");


    // Birkaç düþman oluþtur ve ekle
    for (int i = 0; i < 1; i++)
    {
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, mazeGenerator, charSprite);
        // Düþmanýn duvarda baþlamadýðýndan emin ol
        int ex, ey;
        do {
            ex = (rand() % (MAZE_WIDTH - 2) + 1);
            ey = (rand() % (MAZE_HEIGHT - 2) + 1);
        } while (mazeGenerator->IsWall(ex, ey));
        pEnemy->SetPosition(ex*TILE_SIZE-5, ey*TILE_SIZE-5);
        game_engine->AddSprite(pEnemy);
        mazeGenerator->setValue(ex, ey, 2);
    }
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
        RECT pos = sprite->GetPosition();
        int spriteWidth = sprite->GetBitmap()->GetWidth();
        int spriteHeight = sprite->GetBitmap()->GetHeight();

        RECT spriteRect = pos;
        sprite->Draw(hDC, camera->x, camera->y);
    }
}

void GameCycle()
{
    // Update the background
    background->Update();

    // Update the sprites
    game_engine->UpdateSprites();

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

}

void HandleJoystick(JOYSTATE jsJoystickState) {

}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    return TRUE;
}


void GenerateMaze(Bitmap* tileBit) {
    mazeGenerator->generateMaze();
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->GetMaze();

    int tile_height = wallBitmap->GetHeight();
    int tile_width = wallBitmap->GetWidth();
    RECT rcBounds = { 0, 0, 4000, 4000 }; // or based on maze size

    for (int y = 0; y < mazeArray.size(); ++y) {
        for (int x = 0; x < mazeArray[y].size(); ++x) {

            int posX = x * tile_width;
            int posY = y * tile_height;
            if (mazeArray[y][x] == -1) { // It's a wall
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
            else {
                AddNonCollidableTile(posX, posY, tileBit); // Instead of creating a Sprite
            }
            AddNonCollidableTile(posX, posY, tileBit); // Instead of creating a Sprite
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

