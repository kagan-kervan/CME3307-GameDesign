//-----------------------------------------------------------------
// Space Out Application
// C++ Source - SpaceOut.cpp
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "CME3307-GameDesign.h"

//-----------------------------------------------------------------
// Game Engine Functions
//-----------------------------------------------------------------
BOOL GameInitialize(HINSTANCE hInstance)
{
    // Create the game engine
    _pGame = new GameEngine(hInstance, TEXT("Space Out"),
        TEXT("Space Out"), IDI_SPACEOUT, IDI_SPACEOUT_SM, 600, 450);
    if (_pGame == NULL)
        return FALSE;

    // Set the frame rate
    _pGame->SetFrameRate(30);

    // Store the instance handle
    _hInstance = hInstance;

    return TRUE;
}

void GameStart(HWND hWindow)
{
    // Seed the random number generator
    srand(GetTickCount());

    // Create the offscreen device context and bitmap
    _hOffscreenDC = CreateCompatibleDC(GetDC(hWindow));
    _hOffscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        _pGame->GetWidth(), _pGame->GetHeight());
    SelectObject(_hOffscreenDC, _hOffscreenBitmap);

    // Create and load the bitmaps
    HDC hDC = GetDC(hWindow);
    _pDesertBitmap = new Bitmap(hDC, IDB_DESERT, _hInstance);
    // Create the starry background
    _pBackground = new StarryBackground(600, 450);

    // Play the background music
        //_pGame->PlayMIDISong(TEXT("Music.mid"));

}

void GameEnd()
{
    // Close the MIDI player for the background music
    _pGame->CloseMIDIPlayer();

    // Cleanup the offscreen device context and bitmap
    DeleteObject(_hOffscreenBitmap);
    DeleteDC(_hOffscreenDC);

    // Cleanup the bitmaps
    delete _pDesertBitmap;
    delete _pCarBitmap;
    delete _pSmCarBitmap;
    delete _pMissileBitmap;
    delete _pBlobboBitmap;
    delete _pBMissileBitmap;
    delete _pJellyBitmap;
    delete _pJMissileBitmap;
    delete _pTimmyBitmap;
    delete _pTMissileBitmap;
    delete _pSmExplosionBitmap;
    delete _pLgExplosionBitmap;
    delete _pGameOverBitmap;

    // Cleanup the background
    delete _pBackground;

    // Cleanup the sprites
    _pGame->CleanupSprites();

    // Cleanup the game engine
    delete _pGame;
}

void GameActivate(HWND hWindow)
{
    // Resume the background music
    _pGame->PlayMIDISong(TEXT(""), FALSE);
}

void GameDeactivate(HWND hWindow)
{
    // Pause the background music
    _pGame->PauseMIDISong();
}

void GamePaint(HDC hDC)
{
    // Draw the background
    _pBackground->Draw(hDC);

    // Draw the desert bitmap
    _pDesertBitmap->Draw(hDC, 0, 371);

    // Draw the sprites
    _pGame->DrawSprites(hDC);

    // Draw the score
    TCHAR szText[64];
    RECT  rect = { 460, 0, 510, 30 };
    wsprintf(szText, "%d", _iScore);
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(255, 255, 255));
    DrawText(hDC, szText, -1, &rect, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);

    // Draw the number of remaining lives (cars)
    for (int i = 0; i < _iNumLives; i++)
        _pSmCarBitmap->Draw(hDC, 520 + (_pSmCarBitmap->GetWidth() * i),
            10, TRUE);

    // Draw the game over message, if necessary
    if (_bGameOver)
        _pGameOverBitmap->Draw(hDC, 190, 149, TRUE);
}

void GameCycle()
{
    if (!_bGameOver)
    {
        // Randomly add aliens
        if ((rand() % _iDifficulty) == 0)
            AddAlien();

        // Update the background
        _pBackground->Update();

        // Update the sprites
        _pGame->UpdateSprites();

        // Obtain a device context for repainting the game
        HWND  hWindow = _pGame->GetWindow();
        HDC   hDC = GetDC(hWindow);

        // Paint the game to the offscreen device context
        GamePaint(_hOffscreenDC);

        // Blit the offscreen bitmap to the game screen
        BitBlt(hDC, 0, 0, _pGame->GetWidth(), _pGame->GetHeight(),
            _hOffscreenDC, 0, 0, SRCCOPY);

        // Cleanup
        ReleaseDC(hWindow, hDC);
    }
}

void HandleKeys()
{
    if (!_bGameOver)
    {
        // Move the car based upon left/right key presses
        POINT ptVelocity = _pCarSprite->GetVelocity();
        if (GetAsyncKeyState(VK_LEFT) < 0)
        {
            // Move left
            ptVelocity.x = max(ptVelocity.x - 1L, -4L);
            _pCarSprite->SetVelocity(ptVelocity);
        }
        else if (GetAsyncKeyState(VK_RIGHT) < 0)
        {
            // Move right
            ptVelocity.x = min(ptVelocity.x + 2L, 6L);
            _pCarSprite->SetVelocity(ptVelocity);
        }

        // Fire missiles based upon spacebar presses
        if ((++_iFireInputDelay > 6) && GetAsyncKeyState(VK_SPACE) < 0)
        {
            // Create a new missile sprite
            RECT  rcBounds = { 0, 0, 600, 450 };
            RECT  rcPos = _pCarSprite->GetPosition();
            Sprite* pSprite = new Sprite(_pMissileBitmap, rcBounds, BA_DIE);
            pSprite->SetPosition(rcPos.left + 15, 400);
            pSprite->SetVelocity(0, -7);
            _pGame->AddSprite(pSprite);


            // Reset the input delay
            _iFireInputDelay = 0;
        }
    }

    // Start a new game based upon an Enter (Return) key press
    if (_bGameOver && (GetAsyncKeyState(VK_RETURN) < 0))
        // Start a new game
        NewGame();
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

void HandleJoystick(JOYSTATE jsJoystickState)
{
}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    // See if a player missile and an alien have collided
    Bitmap* pHitter = pSpriteHitter->GetBitmap();
    Bitmap* pHittee = pSpriteHittee->GetBitmap();
    if ((pHitter == _pMissileBitmap && (pHittee == _pBlobboBitmap ||
        pHittee == _pJellyBitmap || pHittee == _pTimmyBitmap)) ||
        (pHittee == _pMissileBitmap && (pHitter == _pBlobboBitmap ||
            pHitter == _pJellyBitmap || pHitter == _pTimmyBitmap)))
    {
        // Play the small explosion sound

        // Kill both sprites
        pSpriteHitter->Kill();
        pSpriteHittee->Kill();

        // Create a large explosion sprite at the alien's position
        RECT rcBounds = { 0, 0, 600, 450 };
        RECT rcPos;
        if (pHitter == _pMissileBitmap)
            rcPos = pSpriteHittee->GetPosition();
        else
            rcPos = pSpriteHitter->GetPosition();
        Sprite* pSprite = new Sprite(_pLgExplosionBitmap, rcBounds);
        pSprite->SetNumFrames(8, TRUE);
        pSprite->SetPosition(rcPos.left, rcPos.top);
        _pGame->AddSprite(pSprite);

        // Update the score
        _iScore += 25;
        _iDifficulty = max(80 - (_iScore / 20), 20);
    }

    // See if an alien missile has collided with the car
    if ((pHitter == _pCarBitmap && (pHittee == _pBMissileBitmap ||
        pHittee == _pJMissileBitmap || pHittee == _pTMissileBitmap)) ||
        (pHittee == _pCarBitmap && (pHitter == _pBMissileBitmap ||
            pHitter == _pJMissileBitmap || pHitter == _pTMissileBitmap)))
    {
        // Play the large explosion sound

        // Kill the missile sprite
        if (pHitter == _pCarBitmap)
            pSpriteHittee->Kill();
        else
            pSpriteHitter->Kill();

        // Create a large explosion sprite at the car's position
        RECT rcBounds = { 0, 0, 600, 480 };
        RECT rcPos;
        if (pHitter == _pCarBitmap)
            rcPos = pSpriteHitter->GetPosition();
        else
            rcPos = pSpriteHittee->GetPosition();
        Sprite* pSprite = new Sprite(_pLgExplosionBitmap, rcBounds);
        pSprite->SetNumFrames(8, TRUE);
        pSprite->SetPosition(rcPos.left, rcPos.top);
        _pGame->AddSprite(pSprite);

        // Move the car back to the start
        _pCarSprite->SetPosition(300, 405);

        // See if the game is over
        if (--_iNumLives == 0)
        {
            // Play the game over sound
            _bGameOver = TRUE;
        }
    }

    return FALSE;
}

void SpriteDying(Sprite* pSprite)
{
    // See if an alien missile sprite is dying
    if (pSprite->GetBitmap() == _pBMissileBitmap ||
        pSprite->GetBitmap() == _pJMissileBitmap ||
        pSprite->GetBitmap() == _pTMissileBitmap)
    {
        // Play the small explosion sound

        // Create a small explosion sprite at the missile's position
        RECT rcBounds = { 0, 0, 600, 450 };
        RECT rcPos = pSprite->GetPosition();
        Sprite* pSprite = new Sprite(_pSmExplosionBitmap, rcBounds);
        pSprite->SetNumFrames(8, TRUE);
        pSprite->SetPosition(rcPos.left, rcPos.top);
        _pGame->AddSprite(pSprite);
    }
}

//-----------------------------------------------------------------
// Functions
//-----------------------------------------------------------------
void NewGame()
{
    // Clear the sprites
    _pGame->CleanupSprites();

    // Create the car sprite
    RECT rcBounds = { 0, 0, 600, 450 };
    _pCarSprite = new Sprite(_pCarBitmap, rcBounds, BA_WRAP);
    _pCarSprite->SetPosition(300, 405);
    _pGame->AddSprite(_pCarSprite);

    // Initialize the game variables
    _iFireInputDelay = 0;
    _iScore = 0;
    _iNumLives = 3;
    _iDifficulty = 80;
    _bGameOver = FALSE;

    // Play the background music
    _pGame->PlayMIDISong();
}

void AddAlien()
{

}
