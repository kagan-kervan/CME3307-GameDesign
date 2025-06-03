//
// Created by ahmet on 3.06.2025.
//

#include "Game.h"


BOOL GameInitialize(HINSTANCE hInstance)
{
    window_X = 1500;
    window_Y = 700;
    // Create the game engine
    game_engine = new GameEngine(hInstance, TEXT("(Space Out)"),
        TEXT("(Space Out)"), IDI_SPACEOUT, IDI_SPACEOUT_SM, window_X, window_Y);
    if (game_engine == NULL)
        return FALSE;

    // Set the frame rate
    game_engine->SetFrameRate(30);

    // Store the instance handle
    instance = hInstance;

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

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
    // Create the starry background
    backgroundBitmap = new Bitmap(hDC, R"(background.bmp)");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    camera = new Camera(0, 0, window_X, window_Y);
    mazeGenerator = new MazeGenerator(20,20);
    GenerateMaze();

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
    background->Draw(hDC, camera->x,camera->y);
    // Draw all sprites with camera offset
    for (auto* sprite : game_engine->GetSprites()) {
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
    GamePaint(offscreenDC);
    GamePaint(hDC);

    // Blit the offscreen bitmap to the game screen
    /*BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
        offscreenDC, 0, 0, SRCCOPY);*/

    // Cleanup
    ReleaseDC(hWindow, hDC);
}

void HandleKeys()
{
    const int CAMERA_SPEED = 10;
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)   
        camera->Move(-CAMERA_SPEED, 0);
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)  
        camera->Move(CAMERA_SPEED, 0);
    if (GetAsyncKeyState(VK_UP) & 0x8000)     
        camera->Move(0, -CAMERA_SPEED);
    if (GetAsyncKeyState(VK_DOWN) & 0x8000)   
        camera->Move(0, CAMERA_SPEED);

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


void GenerateMaze() {
    mazeGenerator->generateMaze();
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->getMaze();

    int tile_height = wallBitmap->GetHeight();
    int tile_width = wallBitmap->GetWidth();
    RECT rcBounds = { 0, 0, window_X, window_Y };

    for (int y = 0; y < mazeArray.size(); ++y) {
        for (int x = 0; x < mazeArray[y].size(); ++x) {
            if (mazeArray[y][x] == -1) { // It's a wall
                int posX = x * tile_width;
                int posY = y * tile_height;
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap,rcBounds, BA_STOP);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
        }
    }
}
