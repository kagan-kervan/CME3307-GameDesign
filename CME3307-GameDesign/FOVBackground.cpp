#include "FOVBackground.h"
#include <cmath>

#define PI 3.14159265358979323846

FOVBackground::FOVBackground(Sprite* pPlayer, int iFOVDegrees, int iFOVDistance)
    : m_pPlayer(pPlayer), m_iFOVDegrees(iFOVDegrees), m_iFOVDistance(iFOVDistance), m_dDirection(0.0)
{
    m_ptMouse.x = 0;
    m_ptMouse.y = 0;
}

// MOUSE BUG FIX: Update uses the camera coordinates to correctly calculate the direction.
SPRITEACTION FOVBackground::Update(int cameraX, int cameraY)
{
    if (m_pPlayer)
    {
        // Player's center in world coordinates
        RECT rcPos = m_pPlayer->GetPosition();
        int playerX = rcPos.left + (rcPos.right - rcPos.left) / 2;
        int playerY = rcPos.top + (rcPos.bottom - rcPos.top) / 2;

        // Convert mouse screen coordinates to world coordinates
        int mouseWorldX = m_ptMouse.x + cameraX;
        int mouseWorldY = m_ptMouse.y + cameraY;

        // Calculate direction vector from player to the mouse in the world
        double dx = mouseWorldX - playerX;
        double dy = mouseWorldY - playerY;
        m_dDirection = atan2(dy, dx);
    }
    return SA_NONE;
}
void FOVBackground::Draw(HDC hDC, int cameraX, int cameraY)
{
    if (m_pPlayer == NULL)
        return;

    // Get the window dimensions from the game engine
    GameEngine* pGame = GameEngine::GetEngine();
    int screenWidth = pGame->GetWidth();
    int screenHeight = pGame->GetHeight();

    // Create a memory DC and bitmap for the darkness overlay
    HDC memDC = CreateCompatibleDC(hDC);
    HBITMAP memBitmap = CreateCompatibleBitmap(hDC, screenWidth, screenHeight);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    // Fill the entire screen with black
    RECT rect = { 0, 0, screenWidth, screenHeight };
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(memDC, &rect, blackBrush);
    DeleteObject(blackBrush);

    // Get player position and adjust for camera
    RECT rcPos = m_pPlayer->GetPosition();
    int playerX = (rcPos.left + (rcPos.right - rcPos.left) / 2) - cameraX;
    int playerY = (rcPos.top + (rcPos.bottom - rcPos.top) / 2) - cameraY;

    // Create the FOV cone mask
    HBRUSH transparent = CreateSolidBrush(RGB(255, 255, 255));
    HPEN transparentPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    SelectObject(memDC, transparent);
    SelectObject(memDC, transparentPen);

    // Calculate FOV points using camera-adjusted position
    float halfFOV = m_iFOVDegrees * 3.14159f / 360.0f;
    float startAngle = m_dDirection - halfFOV;
    float endAngle = m_dDirection + halfFOV;

    // Create FOV polygon points with camera offset
    POINT points[3];
    points[0].x = playerX;
    points[0].y = playerY;
    points[1].x = playerX + (int)(cos(startAngle) * m_iFOVDistance);
    points[1].y = playerY + (int)(sin(startAngle) * m_iFOVDistance);
    points[2].x = playerX + (int)(cos(endAngle) * m_iFOVDistance);
    points[2].y = playerY + (int)(sin(endAngle) * m_iFOVDistance);

    // Draw the FOV cone
    Polygon(memDC, points, 3);

    // Create a blend function for transparency
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 192, 0 }; // 192 is 75% opacity

    // Blend the darkness overlay with the main screen
    AlphaBlend(hDC, 0, 0, screenWidth, screenHeight,
        memDC, 0, 0, screenWidth, screenHeight,
        blend);

    // Clean up
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteObject(transparent);
    DeleteObject(transparentPen);
    DeleteDC(memDC);
}

// Keep the original Draw method for backward compatibility
void FOVBackground::Draw(HDC hDC)
{
    Draw(hDC, 0, 0); // Call the camera-aware version with no camera offset
}