#include "Player.h"
#include "GameEngine.h"
#include "resource.h"

extern GameEngine* _pGame;
extern Bitmap* _pMissileBitmap;

Player::Player(Bitmap* pBitmap, MazeGenerator* pMaze)
    : Sprite(pBitmap), m_pMaze(pMaze)
{
    m_iSpeed = 4;
}

SPRITEACTION Player::Update()
{
    HandleInput();

    // Hareket için Sprite::Update'i çaðýrmýyoruz, çünkü pozisyonu HandleInput'ta kendimiz ayarlýyoruz.
    // Sadece animasyon gibi baþka özellikler varsa çaðýrýlabilir.
    // return Sprite::Update(); // Bu satýr yerine aþaðýdakini kullanýn:

    UpdateFrame(); // Sadece animasyon karesini günceller
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

    if (!m_pMaze->IsWall(rcNewPos.left/TILE_SIZE, rcNewPos.top/TILE_SIZE) &&
        !m_pMaze->IsWall((rcNewPos.right - 1)/TILE_SIZE, rcNewPos.top/TILE_SIZE) &&
        !m_pMaze->IsWall(rcNewPos.left / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE) &&
        !m_pMaze->IsWall((rcNewPos.right - 1) / TILE_SIZE, (rcNewPos.bottom - 1) / TILE_SIZE))
    {
        SetPosition(newX, newY);
    }

    /*static int fireDelay = 0;
    if (++fireDelay > 10 && (GetAsyncKeyState(VK_SPACE) & 0x8000))
    {
        fireDelay = 0;
        RECT rcBounds = { 0, 0, m_pMaze->getMaze().size() * 50, m_pMaze->getMaze()[0].size() * 50 };
        Sprite* pBullet = new Sprite(_pMissileBitmap, rcBounds, BA_DIE);
        pBullet->SetPosition(m_rcPosition.left + (GetWidth() / 2) - (pBullet->GetWidth() / 2), m_rcPosition.top);
        pBullet->SetVelocity(0, -8);
        _pGame->AddSprite(pBullet);
    }*/
}