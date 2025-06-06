#pragma once
// Enemy.h
#pragma once
#include "Sprite.h"
#include "Maze.h"

class Enemy : public Sprite
{
public:
    Enemy(Bitmap* pBitmap, Maze* pMaze, Sprite* pPlayer);

    virtual SPRITEACTION Update();

private:
    void ChasePlayer(); // Oyuncuyu takip etme mant���

    Maze* m_pMaze;
    Sprite* m_pPlayer; // Hedef oyuncu
    int m_iSpeed;
    int m_iTick; // AI'�n ne s�kl�kla �al��aca��n� kontrol etmek i�in
};