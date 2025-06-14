#pragma once

#include "GameEngine.h"
#include "Sprite.h"

class FOVBackground
{
protected:
    Sprite* m_pPlayer;
    int m_iFOVDegrees;
    int m_iFOVDistance;
    double m_dDirection;
    POINT m_ptMouse;

public:
    FOVBackground(Sprite* pPlayer, int iFOVDegrees = 90, int iFOVDistance = 200);
    virtual ~FOVBackground() {}

    virtual void Draw(HDC hDC);
    virtual void Draw(HDC hDC, int cameraX, int cameraY);

    // MOUSE BUG FIX: Update now takes camera coordinates
    virtual SPRITEACTION Update(int cameraX, int cameraY);

    void UpdateMousePos(int x, int y) { m_ptMouse.x = x; m_ptMouse.y = y; }
};