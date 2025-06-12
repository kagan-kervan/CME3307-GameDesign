#pragma once

#include "GameEngine.h"
#include "Sprite.h"

class FOVBackground
{
protected:
    Sprite* m_pPlayer;       // Reference to the player sprite
    int m_iFOVDegrees;       // Field of view in degrees
    int m_iFOVDistance;      // How far the FOV extends
    double m_dDirection;     // Direction angle in radians
    POINT m_ptMouse;        // Current mouse position

public:
    FOVBackground(Sprite* pPlayer, int iFOVDegrees = 90, int iFOVDistance = 200);
    virtual ~FOVBackground() {}

    virtual void Draw(HDC hDC);
    virtual void Draw(HDC hDC, int cameraX, int cameraY); // Add camera-aware drawing
    virtual SPRITEACTION Update();

    // Handle mouse position updates
    void UpdateMousePos(int x, int y) { m_ptMouse.x = x; m_ptMouse.y = y; }
};