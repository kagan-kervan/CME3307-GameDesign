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
    void HandleInput(); // Klavye girdilerini burada iþleyeceðiz

private:
    Maze* m_pMaze; // Çarpýþma kontrolü için labirent referansý
    int m_iSpeed;
};