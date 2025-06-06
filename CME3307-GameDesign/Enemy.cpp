// Enemy.cpp
#include "Enemy.h"
#include <cmath> // abs() fonksiyonu için


Enemy::Enemy(Bitmap* pBitmap, MazeGenerator* pMaze, Sprite* pPlayer)
    : Sprite(pBitmap), m_pMaze(pMaze), m_pPlayer(pPlayer)
{
    m_iSpeed = 16; // Düþman hýzý
    m_iTick = 0;
}

SPRITEACTION Enemy::Update()
{
    m_iTick++;
    if (m_iTick > 5) // Her 5 frame'de bir yönünü güncelle
    {
        m_iTick = 0;
        ChasePlayer();
    }

    // Sprite'ýn temel Update fonksiyonunu çaðýr (hareket ve animasyon için)
    return Sprite::Update();
}

void Enemy::ChasePlayer()
{
    if (!m_pPlayer) return;

    RECT playerPos = m_pPlayer->GetPosition();
    int dx = playerPos.left - m_rcPosition.left;
    int dy = playerPos.top - m_rcPosition.top;

    int moveX = 0;
    int moveY = 0;

    // Basit bir takip AI'si:
    // Yatayda mý yoksa dikeyde mi daha uzaksak, o yönde hareket etmeye öncelik ver.
    if (abs(dx) > abs(dy))
    {
        moveX = (dx > 0) ? m_iSpeed : -m_iSpeed;
    }
    else
    {
        moveY = (dy > 0) ? m_iSpeed : -m_iSpeed;
    }

    // Gitmek istediðimiz yön bir duvarsa, diðer yönü dene.
    RECT currentPos = GetPosition();
    //TO-DO: Add tile size as global
    int tilePosX = (currentPos.left + moveX) / TILE_SIZE;
    int tilePosY = (currentPos.top + moveY) / TILE_SIZE;
    if (m_pMaze->IsWall(tilePosX, tilePosY))
    {
        // Asýl yön týkalý, alternatifi dene
        if (moveX != 0) // Yatay hareket týkalýysa dikeyi dene
        {
            moveX = 0;
            moveY = (dy > 0) ? m_iSpeed : -m_iSpeed;
        }
        else // Dikey hareket týkalýysa yatayý dene
        {
            moveY = 0;
            moveX = (dx > 0) ? m_iSpeed : -m_iSpeed;
        }

        // Alternatif yön de týkalýysa, dur
        if (m_pMaze->IsWall(tilePosX, tilePosY))
        {
            moveX = 0;
            moveY = 0;
        }
    }

    // Hýzý ayarla
    SetVelocity(moveX, moveY);
}