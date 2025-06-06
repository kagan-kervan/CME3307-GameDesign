#include "Player.h"
#include "GameEngine.h"
#include "resource.h"

extern GameEngine* _pGame;
extern Bitmap* _pMissileBitmap;

Player::Player(Bitmap* pBitmap, Maze* pMaze)
    : Sprite(pBitmap), m_pMaze(pMaze)
{
    m_iSpeed = 4;
}

SPRITEACTION Player::Update()
{
    HandleInput();

    // Hareket i�in Sprite::Update'i �a��rm�yoruz, ��nk� pozisyonu HandleInput'ta kendimiz ayarl�yoruz.
    // Sadece animasyon gibi ba�ka �zellikler varsa �a��r�labilir.
    // return Sprite::Update(); // Bu sat�r yerine a�a��dakini kullan�n:

    UpdateFrame(); // Sadece animasyon karesini g�nceller
    if (m_bDying)
        return SA_KILL;

    return SA_NONE;
}

void Player::HandleInput()
{
    POINT currentPos = { m_rcPosition.left, m_rcPosition.top };
    int moveX = 0;
    int moveY = 0;

    if (GetAsyncKeyState(VK_UP) & 0x8000)    moveY = -m_iSpeed;
    if (GetAsyncKeyState(VK_DOWN) & 0x8000)  moveY = m_iSpeed;
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)  moveX = -m_iSpeed;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) moveX = m_iSpeed;

    int newX = currentPos.x + moveX;
    int newY = currentPos.y + moveY;

    RECT rcNewPos = { newX, newY, newX + GetWidth(), newY + GetHeight() };

    if (!m_pMaze->IsWall(rcNewPos.left, rcNewPos.top) &&
        !m_pMaze->IsWall(rcNewPos.right - 1, rcNewPos.top) &&
        !m_pMaze->IsWall(rcNewPos.left, rcNewPos.bottom - 1) &&
        !m_pMaze->IsWall(rcNewPos.right - 1, rcNewPos.bottom - 1))
    {
        SetPosition(newX, newY);
    }

    static int fireDelay = 0;
    if (++fireDelay > 10 && (GetAsyncKeyState(VK_SPACE) & 0x8000))
    {
        fireDelay = 0;
        RECT rcBounds = { 0, 0, MAZE_WIDTH * TILE_SIZE, MAZE_HEIGHT * TILE_SIZE };
        Sprite* pBullet = new Sprite(_pMissileBitmap, rcBounds, BA_DIE);
        pBullet->SetPosition(m_rcPosition.left + (GetWidth() / 2) - (pBullet->GetWidth() / 2), m_rcPosition.top);
        pBullet->SetVelocity(0, -8);
        _pGame->AddSprite(pBullet);
    }
}