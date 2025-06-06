// MazeGame.cpp (Eski SpaceOut.cpp'nin yerine)

#include "MazeGame.h"

// Global Deðiþkenlerin Tanýmlanmasý
HINSTANCE   _hInstance;
GameEngine* _pGame;
Bitmap* _pWallBitmap;
Bitmap* _pFloorBitmap;
Bitmap* _pPlayerBitmap;
Bitmap* _pEnemyBitmap;
Bitmap* _pMissileBitmap;
Maze* _pMaze;
Player* _pPlayerSprite;
HDC         _hOffscreenDC;
HBITMAP     _hOffscreenBitmap;

BOOL GameInitialize(HINSTANCE hInstance)
{
    _pGame = new GameEngine(hInstance, TEXT("Maze Game"), TEXT("Maze Game"),
        IDI_SPACEOUT, IDI_SPACEOUT_SM, 800, 600);
    if (_pGame == NULL) return FALSE;
    _pGame->SetFrameRate(60);
    _hInstance = hInstance;
    return TRUE;
}

void NewGame();

void GameStart(HWND hWindow)
{
    srand(GetTickCount());

    _hOffscreenDC = CreateCompatibleDC(GetDC(hWindow));
    _hOffscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        _pGame->GetWidth(), _pGame->GetHeight());
    SelectObject(_hOffscreenDC, _hOffscreenBitmap);

    HDC hDC = GetDC(hWindow);
    _pWallBitmap = new Bitmap(hDC, IDB_WALL, _hInstance);
    _pFloorBitmap = new Bitmap(hDC, IDB_FLOOR, _hInstance);
    _pPlayerBitmap = new Bitmap(hDC, IDB_PLAYER, _hInstance);
    _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, _hInstance);
    _pMissileBitmap = new Bitmap(hDC, IDB_BULLET, _hInstance);

    ReleaseDC(hWindow, hDC);
    NewGame();
}

void GameEnd()
{
    DeleteObject(_hOffscreenBitmap);
    DeleteDC(_hOffscreenDC);

    delete _pWallBitmap;
    delete _pFloorBitmap;
    delete _pPlayerBitmap;
    delete _pEnemyBitmap;
    delete _pMissileBitmap;
    delete _pMaze;

    _pGame->CleanupSprites();
    delete _pGame;
}

// LÝNKER HATALARINI ÇÖZMEK ÝÇÝN BOÞ FONKSÝYONLAR
void GameActivate(HWND hWindow) {}
void GameDeactivate(HWND hWindow) {}

void GamePaint(HDC hDC)
{
    if (_pMaze)
        _pMaze->Draw(hDC, _pGame->GetCamera());

    _pGame->DrawSprites(hDC);
}

void GameCycle()
{
    _pGame->UpdateSprites();

    if (_pPlayerSprite)
    {
        RECT playerPos = _pPlayerSprite->GetPosition();
        int camX = playerPos.left - (_pGame->GetWidth() / 2);
        int camY = playerPos.top - (_pGame->GetHeight() / 2);

        int maxCamX = (MAZE_WIDTH * TILE_SIZE) - _pGame->GetWidth();
        int maxCamY = (MAZE_HEIGHT * TILE_SIZE) - _pGame->GetHeight();
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > maxCamX) camX = maxCamX;
        if (camY > maxCamY) camY = maxCamY;

        _pGame->SetCamera(camX, camY);
    }

    HWND hWindow = _pGame->GetWindow();
    HDC  hDC = GetDC(hWindow);
    GamePaint(_hOffscreenDC);
    BitBlt(hDC, 0, 0, _pGame->GetWidth(), _pGame->GetHeight(),
        _hOffscreenDC, 0, 0, SRCCOPY);
    ReleaseDC(hWindow, hDC);
}

void HandleKeys() {}

void MouseButtonDown(int x, int y, BOOL bLeft) {}
void MouseButtonUp(int x, int y, BOOL bLeft) {}
void MouseMove(int x, int y) {}
BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    return FALSE;
}
void SpriteDying(Sprite* pSprite) {}

void NewGame()
{
    _pGame->CleanupSprites();

    if (_pMaze) delete _pMaze;
    _pMaze = new Maze(_pWallBitmap, _pFloorBitmap);

    _pPlayerSprite = new Player(_pPlayerBitmap, _pMaze);
    _pPlayerSprite->SetPosition(TILE_SIZE * 2, TILE_SIZE * 2);
    _pGame->AddSprite(_pPlayerSprite);

    for (int i = 0; i < 5; i++)
    {
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, _pMaze, _pPlayerSprite);
        int ex, ey;
        do {
            ex = (rand() % (MAZE_WIDTH - 2) + 1) * TILE_SIZE;
            ey = (rand() % (MAZE_HEIGHT - 2) + 1) * TILE_SIZE;
        } while (_pMaze->IsWall(ex, ey));
        pEnemy->SetPosition(ex, ey);
        _pGame->AddSprite(pEnemy);
    }
}