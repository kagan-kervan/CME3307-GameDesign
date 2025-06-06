#pragma once
// Player.h
#pragma once
#include "Sprite.h"
#include "Maze.h"

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, Maze* pMaze);

    virtual SPRITEACTION Update();
    void HandleInput(); // Klavye girdilerini burada i�leyece�iz

private:
    Maze* m_pMaze; // �arp��ma kontrol� i�in labirent referans�
    int m_iSpeed;
};