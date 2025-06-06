#pragma once
// Player.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);

    virtual SPRITEACTION Update();
    void HandleInput(); // Klavye girdilerini burada iþleyeceðiz

private:
    MazeGenerator* m_pMaze; // Çarpýþma kontrolü için labirent referansý
    int m_iSpeed;
};