// Enemy.cpp
#include "Enemy.h"
#include <cmath> // abs() fonksiyonu i�in

Enemy::Enemy(Bitmap* pBitmap, Maze* pMaze, Sprite* pPlayer)
    : Sprite(pBitmap), m_pMaze(pMaze), m_pPlayer(pPlayer)
{
    m_iSpeed = 2; // D��man h�z�
    m_iTick = 0;
}

SPRITEACTION Enemy::Update()
{
    m_iTick++;
    if (m_iTick > 5) // Her 5 frame'de bir y�n�n� g�ncelle
    {
        m_iTick = 0;
        ChasePlayer();
    }

    // Sprite'�n temel Update fonksiyonunu �a��r (hareket ve animasyon i�in)
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
    // Yatayda m� yoksa dikeyde mi daha uzaksak, o y�nde hareket etmeye �ncelik ver.
    if (abs(dx) > abs(dy))
    {
        moveX = (dx > 0) ? m_iSpeed : -m_iSpeed;
    }
    else
    {
        moveY = (dy > 0) ? m_iSpeed : -m_iSpeed;
    }

    // Gitmek istedi�imiz y�n bir duvarsa, di�er y�n� dene.
    RECT currentPos = GetPosition();
    if (m_pMaze->IsWall(currentPos.left + moveX, currentPos.top + moveY))
    {
        // As�l y�n t�kal�, alternatifi dene
        if (moveX != 0) // Yatay hareket t�kal�ysa dikeyi dene
        {
            moveX = 0;
            moveY = (dy > 0) ? m_iSpeed : -m_iSpeed;
        }
        else // Dikey hareket t�kal�ysa yatay� dene
        {
            moveY = 0;
            moveX = (dx > 0) ? m_iSpeed : -m_iSpeed;
        }

        // Alternatif y�n de t�kal�ysa, dur
        if (m_pMaze->IsWall(currentPos.left + moveX, currentPos.top + moveY))
        {
            moveX = 0;
            moveY = 0;
        }
    }

    // H�z� ayarla
    SetVelocity(moveX, moveY);
}