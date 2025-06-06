#pragma once
// Enemy.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"

class Enemy : public Sprite
{
public:
    Enemy(Bitmap* pBitmap, MazeGenerator* pMaze, Sprite* pPlayer);

    virtual SPRITEACTION Update();

private:
    void ChasePlayer(); // Oyuncuyu takip etme mantýðý

    MazeGenerator* m_pMaze;
    Sprite* m_pPlayer; // Hedef oyuncu
    int m_iSpeed;
    int m_iTick; // AI'ýn ne sýklýkla çalýþacaðýný kontrol etmek için
};