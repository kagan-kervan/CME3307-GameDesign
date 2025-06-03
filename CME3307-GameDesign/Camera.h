#pragma once
#include <windows.h>

class Camera
{
public:
    int x, y; // Top-left of the camera in world coordinates
    int width, height; // Size of the camera (viewport)

    Camera(int x = 0, int y = 0, int width = 640, int height = 480)
        : x(x), y(y), width(width), height(height) {
    }

    // Move camera by delta
    void Move(int dx, int dy) { x += dx; y += dy; }

    // Set camera position
    void SetPosition(int newX, int newY) { x = newX; y = newY; }

    // Get camera RECT
    RECT GetRect() const { return { x, y, x + width, y + height }; }
};