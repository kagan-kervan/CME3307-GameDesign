//
// Created by ahmet on 3.06.2025.
//

#include "Game.h"


BOOL GameInitialize(HINSTANCE hInstance)
{
    window_X = 1500;
    window_Y = 800;
    // Create the game engine
    game_engine = new GameEngine(hInstance, TEXT("(Space Out)"),
        TEXT("(Space Out)"), IDI_SPACEOUT, IDI_SPACEOUT_SM, window_X, window_Y);
    if (game_engine == NULL)
        return FALSE;

    // Set the frame rate
    game_engine->SetFrameRate(30);

    // Store the instance handle
    instance = hInstance;

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
    // Draw the background
    background->Draw(hDC);

    // Draw the sprites
    game_engine->DrawSprites(hDC);
}

void GameCycle()
{

}

void HandleKeys()
{

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
                Sprite* wall = new Sprite(wallBitmap, pos, { 0,0 }, 1, rcBounds, BA_STOP);
                game_engine->AddSprite(wall);
            }
        }
    }
}
