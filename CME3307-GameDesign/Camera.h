// Camera.h

#pragma once
#include <windows.h>
#include "Sprite.h" // Sprite'ý takip edebilmesi için eklendi

class Camera
{
public:
    int x, y;           // Kameranýn dünya koordinatlarýndaki sol üst köþesi
    int width, height;  // Kameranýn boyutu (görüþ alaný)

    // DEÐÝÞÝKLÝK: Kurucu metod artýk takip edilecek bir hedef alýyor.
    Camera(Sprite* target, int w = 640, int h = 480);

    // DEÐÝÞÝKLÝK: Kameranýn yumuþak hareketini saðlayacak Update fonksiyonu
    void Update();

    // Orijinal fonksiyonlar yerinde duruyor
    void Move(int dx, int dy) { x += dx; y += dy; }
    void SetPosition(int newX, int newY) { x = newX; y = newY; }
    RECT GetRect() const { return { x, y, x + width, y + height }; }

private:
    Sprite* m_pTarget;          // Takip edilecek sprite (oyuncu)
    float   m_fLerpFactor;      // Yumuþatma faktörü (0.0 ile 1.0 arasý)
    float   m_fCurrentX, m_fCurrentY; // Akýcý hareket için float pozisyonlar
};