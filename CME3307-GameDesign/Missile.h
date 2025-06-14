// Missile.h
#pragma once
#include "Sprite.h"

class Missile : public Sprite
{
public:
    // Constructor artýk hýz için POINT yerine float vektör alacak
    Missile(Bitmap* pBitmap, RECT& rcBounds, POINT ptPosition, float fVelocityX, float fVelocityY);

    // Update fonksiyonunu override ederek kendi akýcý hareket mantýðýmýzý ekleyeceðiz
    virtual SPRITEACTION Update();

private:
    // Akýcý hareket için float konum ve hýz deðiþkenleri
    float m_fPositionX;
    float m_fPositionY;
    float m_fVelocityX;
    float m_fVelocityY;
};