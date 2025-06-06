// MazeGame.cpp (Eski SpaceOut.cpp'nin yerine)

#include "MazeGame.h"

// Global Deðiþkenlerin Tanýmlanmasý
HINSTANCE   _hInstance;
GameEngine* _pGame;
Bitmap* _pWallBitmap;
Bitmap* _pFloorBitmap;
Bitmap* _pPlayerBitmap;
Bitmap* _pEnemyBitmap;
Bitmap* _pMissileBitmap; // Mermi için Player.cpp'de extern ile eriþiliyor
Maze* _pMaze;
Player* _pPlayerSprite;
HDC         _hOffscreenDC;
HBITMAP     _hOffscreenBitmap;

BOOL GameInitialize(HINSTANCE hInstance)
{
    _pGame = new GameEngine(hInstance, TEXT("Maze Game"), TEXT("Maze Game"),
        IDI_SPACEOUT, IDI_SPACEOUT_SM, 800, 600); // Pencere boyutunu büyüttük
    if (_pGame == NULL) return FALSE;
    _pGame->SetFrameRate(60);
    _hInstance = hInstance;
    return TRUE;
}

void NewGame(); // Ýleriye dönük bildirim

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

void GamePaint(HDC hDC)
{
    // Labirenti çiz (kamera pozisyonunu dikkate alarak)
    _pMaze->Draw(hDC, _pGame->GetCamera());

    // Sprite'larý çiz (GameEngine bu iþi kamera ile hallediyor)
    _pGame->DrawSprites(hDC);
}

void GameCycle()
{
    // Sprite'larý güncelle (Player input, Enemy AI, mermi hareketi vs.)
    _pGame->UpdateSprites();

    // Kamerayý oyuncuyu ortalayacak þekilde güncelle
    if (_pPlayerSprite)
    {
        RECT playerPos = _pPlayerSprite->GetPosition();
        int camX = playerPos.left - (_pGame->GetWidth() / 2);
        int camY = playerPos.top - (_pGame->GetHeight() / 2);

        // Kameranýn labirent dýþýna çýkmasýný engelle
        int maxCamX = (MAZE_WIDTH * TILE_SIZE) - _pGame->GetWidth();
        int maxCamY = (MAZE_HEIGHT * TILE_SIZE) - _pGame->GetHeight();
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > maxCamX) camX = maxCamX;
        if (camY > maxCamY) camY = maxCamY;

        _pGame->SetCamera(camX, camY);
    }

    // Ekrana çizim yap (offscreen buffer kullanarak)
    HWND hWindow = _pGame->GetWindow();
    HDC  hDC = GetDC(hWindow);
    GamePaint(_hOffscreenDC);
    BitBlt(hDC, 0, 0, _pGame->GetWidth(), _pGame->GetHeight(),
        _hOffscreenDC, 0, 0, SRCCOPY);
    ReleaseDC(hWindow, hDC);
}

void HandleKeys() {
    // Input iþlemleri artýk Player::Update içinde yapýlýyor.
    // Burasý genel kontroller için kullanýlabilir (örn: Oyunu yeniden baþlat)
}

void MouseButtonDown(int x, int y, BOOL bLeft) {}
void MouseButtonUp(int x, int y, BOOL bLeft) {}
void MouseMove(int x, int y) {}
BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    // TODO: Mermi-düþman çarpýþmasýný burada ele al
    return FALSE;
}
void SpriteDying(Sprite* pSprite) {}

void NewGame()
{
    _pGame->CleanupSprites();

    // Labirenti oluþtur
    _pMaze = new Maze(_pWallBitmap, _pFloorBitmap);

    // Oyuncuyu oluþtur ve ekle
    _pPlayerSprite = new Player(_pPlayerBitmap, _pMaze);
    _pPlayerSprite->SetPosition(TILE_SIZE * 2, TILE_SIZE * 2); // Baþlangýç pozisyonu
    _pGame->AddSprite(_pPlayerSprite);

    // Birkaç düþman oluþtur ve ekle
    for (int i = 0; i < 5; i++)
    {
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, _pMaze, _pPlayerSprite);
        // Düþmanýn duvarda baþlamadýðýndan emin ol
        int ex, ey;
        do {
            ex = (rand() % (MAZE_WIDTH - 2) + 1) * TILE_SIZE;
            ey = (rand() % (MAZE_HEIGHT - 2) + 1) * TILE_SIZE;
        } while (_pMaze->IsWall(ex, ey));
        pEnemy->SetPosition(ex, ey);
        _pGame->AddSprite(pEnemy);
    }
}