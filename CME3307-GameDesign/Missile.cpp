// Missile.cpp
#include "Missile.h"

// Constructor: Baþlangýç pozisyonunu ve float hýz vektörünü alýr
Missile::Missile(Bitmap* pBitmap, RECT& rcBounds, POINT ptPosition, float fVelocityX, float fVelocityY)
    : Sprite(pBitmap, ptPosition, { 0, 0 }, 0, rcBounds, BA_DIE, SPRITE_TYPE_PLAYER_MISSILE)
{
    // Sprite'ýn kendi hýzýný sýfýrlýyoruz, çünkü kendi hýz mantýðýmýzý kullanacaðýz.
    SetVelocity(0, 0);

    // Float konum ve hýz deðiþkenlerini baþlat
    m_fPositionX = (float)ptPosition.x;
    m_fPositionY = (float)ptPosition.y;
    m_fVelocityX = fVelocityX;
    m_fVelocityY = fVelocityY;
}

Missile::Missile(Bitmap* pBitmap, RECT& rcBounds, POINT ptPosition, float fVelocityX, float fVelocityY, SpriteType type)
    : Sprite(pBitmap, ptPosition, { 0, 0 }, 0, rcBounds, BA_DIE, type)
{
    // Sprite'ın kendi hızını sıfırlıyoruz, çünkü kendi hız mantığımızı kullanacağız.
    SetVelocity(0, 0);

    // Float konum ve hız değişkenlerini başlat
    m_fPositionX = (float)ptPosition.x;
    m_fPositionY = (float)ptPosition.y;
    m_fVelocityX = fVelocityX;
    m_fVelocityY = fVelocityY;
}

// Kendi akýcý hareket mantýðýmýzý içeren Update metodu
SPRITEACTION Missile::Update()
{
    // Delta time'ý varsayalým (1.0f / 60.0f).
    // Bu deðerin oyun motorundan gelmesi en idealidir.
    float fDeltaTime = 1.0f / 60.0f;

    // Yeni pozisyonu hesapla: Pozisyon += Hýz * Zaman
    m_fPositionX += m_fVelocityX * fDeltaTime;
    m_fPositionY += m_fVelocityY * fDeltaTime;

    // Sprite'ýn tamsayý pozisyonunu güncelle (çizim ve çarpýþma için)
    SetPosition((int)m_fPositionX, (int)m_fPositionY);

    // Sprite'ýn normal update'ini çaðýr (sýnýr kontrolü ve ölme durumu için)
    // Ama kendi pozisyon güncellememizi yaptýðýmýz için, base class'ýnkini
    // direkt çaðýrmak yerine sadece sýnýr kontrolünü yapabiliriz.
    // Þimdilik, Sprite::Update() pozisyonu tekrar deðiþtireceði için bunu çaðýrmayalým
    // ve sýnýr kontrolünü manuel yapalým.

    // Sýnýr kontrolü (BA_DIE için)
    if ((m_rcPosition.right < m_rcBounds.left ||
        m_rcPosition.left > m_rcBounds.right ||
        m_rcPosition.bottom < m_rcBounds.top ||
        m_rcPosition.top > m_rcBounds.bottom))
    {
        return SA_KILL;
    }

    // Animasyon ve ölme durumunu kontrol et
    UpdateFrame();
    if (m_bDying)
        return SA_KILL;

    return SA_NONE;
}