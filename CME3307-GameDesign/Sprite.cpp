//-----------------------------------------------------------------
// Sprite Object
// C++ Source - Sprite.cpp
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "Sprite.h"
#include <stdio.h>
#include <cmath> // cos ve sin fonksiyonları için
//-----------------------------------------------------------------
// Sprite Constructor(s)/Destructor
//-----------------------------------------------------------------
Sprite::Sprite(Bitmap* pBitmap, SpriteType type)
{
    // Initialize the member variables
    m_eSpriteType = type; // Tipi ayarla
    m_pBitmap = pBitmap;
    m_iNumFrames = 1;
    m_iCurFrame = m_iFrameDelay = m_iFrameTrigger = 0;
    SetRect(&m_rcPosition, 0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
    CalcCollisionRect();
    m_ptVelocity.x = m_ptVelocity.y = 0;
    m_iZOrder = 0;
    SetRect(&m_rcBounds, 0, 0, 640, 480);
    m_baBoundsAction = BA_STOP;
    m_bHidden = FALSE;
    m_bDying = FALSE;
    m_bOneCycle = FALSE;
    m_dRotationAngle = 0.0;
}

Sprite::Sprite(Bitmap* pBitmap, RECT& rcBounds, BOUNDSACTION baBoundsAction, SpriteType type)
{
    // Calculate a random position
    int iXPos = rand() % (rcBounds.right - rcBounds.left);
    int iYPos = rand() % (rcBounds.bottom - rcBounds.top);

    // Initialize the member variables
    m_eSpriteType = type; // Tipi ayarla
    m_pBitmap = pBitmap;
    m_iNumFrames = 1;
    m_iCurFrame = m_iFrameDelay = m_iFrameTrigger = 0;
    SetRect(&m_rcPosition, iXPos, iYPos, iXPos + pBitmap->GetWidth(),
        iYPos + pBitmap->GetHeight());
    CalcCollisionRect();
    m_ptVelocity.x = m_ptVelocity.y = 0;
    m_iZOrder = 0;
    CopyRect(&m_rcBounds, &rcBounds);
    m_baBoundsAction = baBoundsAction;
    m_bHidden = FALSE;
    m_bDying = FALSE;
    m_bOneCycle = FALSE;
    m_dRotationAngle = 0.0;
}

Sprite::Sprite(Bitmap* pBitmap, POINT ptPosition, POINT ptVelocity, int iZOrder,
    RECT& rcBounds, BOUNDSACTION baBoundsAction, SpriteType type)
{
    // Initialize the member variables
    m_eSpriteType = type; // Tipi ayarla
    m_pBitmap = pBitmap;
    m_iNumFrames = 1;
    m_iCurFrame = m_iFrameDelay = m_iFrameTrigger = 0;

    // DÜZELTME 1: Konum (m_rcPosition) doðru þekilde ayarlandý.
    SetRect(&m_rcPosition, ptPosition.x, ptPosition.y,
        ptPosition.x + pBitmap->GetWidth(), ptPosition.y + pBitmap->GetHeight());
    CalcCollisionRect();

    // DÜZELTME 2: Hýz (m_ptVelocity) doðru parametreden (ptVelocity) ayarlandý.
    m_ptVelocity = ptVelocity;

    m_iZOrder = iZOrder;
    CopyRect(&m_rcBounds, &rcBounds);
    m_baBoundsAction = baBoundsAction;
    m_bHidden = FALSE;
    m_bDying = FALSE;
    m_bOneCycle = FALSE;
    m_dRotationAngle = 0.0;
}


Sprite::~Sprite()
{
}

//-----------------------------------------------------------------
// Sprite General Methods
//-----------------------------------------------------------------
SPRITEACTION Sprite::Update()
{
  // See if the sprite needs to be killed
  if (m_bDying)
    return SA_KILL;

  // Update the frame
  UpdateFrame();

  // Update the position
  POINT ptNewPosition, ptSpriteSize, ptBoundsSize;
  ptNewPosition.x = m_rcPosition.left + m_ptVelocity.x;
  ptNewPosition.y = m_rcPosition.top + m_ptVelocity.y;
  ptSpriteSize.x = m_rcPosition.right - m_rcPosition.left;
  ptSpriteSize.y = m_rcPosition.bottom - m_rcPosition.top;
  ptBoundsSize.x = m_rcBounds.right - m_rcBounds.left;
  ptBoundsSize.y = m_rcBounds.bottom - m_rcBounds.top;

  // Check the bounds
  // Wrap?
  if (m_baBoundsAction == BA_WRAP)
  {
    if ((ptNewPosition.x + ptSpriteSize.x) < m_rcBounds.left)
      ptNewPosition.x = m_rcBounds.right;
    else if (ptNewPosition.x > m_rcBounds.right)
      ptNewPosition.x = m_rcBounds.left - ptSpriteSize.x;
    if ((ptNewPosition.y + ptSpriteSize.y) < m_rcBounds.top)
      ptNewPosition.y = m_rcBounds.bottom;
    else if (ptNewPosition.y > m_rcBounds.bottom)
      ptNewPosition.y = m_rcBounds.top - ptSpriteSize.y;
  }
  // Bounce?
  else if (m_baBoundsAction == BA_BOUNCE)
  {
    BOOL bBounce = FALSE;
    POINT ptNewVelocity = m_ptVelocity;
    if (ptNewPosition.x < m_rcBounds.left)
    {
      bBounce = TRUE;
      ptNewPosition.x = m_rcBounds.left;
      ptNewVelocity.x = -ptNewVelocity.x;
    }
    else if ((ptNewPosition.x + ptSpriteSize.x) > m_rcBounds.right)
    {
      bBounce = TRUE;
      ptNewPosition.x = m_rcBounds.right - ptSpriteSize.x;
      ptNewVelocity.x = -ptNewVelocity.x;
    }
    if (ptNewPosition.y < m_rcBounds.top)
    {
      bBounce = TRUE;
      ptNewPosition.y = m_rcBounds.top;
      ptNewVelocity.y = -ptNewVelocity.y;
    }
    else if ((ptNewPosition.y + ptSpriteSize.y) > m_rcBounds.bottom)
    {
      bBounce = TRUE;
      ptNewPosition.y = m_rcBounds.bottom - ptSpriteSize.y;
      ptNewVelocity.y = -ptNewVelocity.y;
    }
    if (bBounce)
      SetVelocity(ptNewVelocity);
  }
  // Die?
  else if (m_baBoundsAction == BA_DIE)
  {
    if ((ptNewPosition.x + ptSpriteSize.x) < m_rcBounds.left ||
      ptNewPosition.x > m_rcBounds.right ||
      (ptNewPosition.y + ptSpriteSize.y) < m_rcBounds.top ||
      ptNewPosition.y > m_rcBounds.bottom)
      return SA_KILL;
  }
  // Stop (default)
  else
  {
    if (ptNewPosition.x  < m_rcBounds.left ||
      ptNewPosition.x > (m_rcBounds.right - ptSpriteSize.x))
    {
      ptNewPosition.x = max(m_rcBounds.left, min(ptNewPosition.x,
        m_rcBounds.right - ptSpriteSize.x));
      SetVelocity(0, 0);
    }
    if (ptNewPosition.y  < m_rcBounds.top ||
      ptNewPosition.y > (m_rcBounds.bottom - ptSpriteSize.y))
    {
      ptNewPosition.y = max(m_rcBounds.top, min(ptNewPosition.y,
        m_rcBounds.bottom - ptSpriteSize.y));
      SetVelocity(0, 0);
    }
  }
  SetPosition(ptNewPosition);

  return SA_NONE;
}

Sprite* Sprite::AddSprite()
{
  return NULL;
}

// BU FONKSİYONU TAMAMEN DEĞİŞTİRİN
void Sprite::Draw(HDC hDC, int cameraX, int cameraY)
{
    if (m_pBitmap == NULL || m_bHidden || m_pBitmap->GetWidth() <= 0)
        return;

    int width = GetWidth();
    int height = GetHeight();
    int drawX = m_rcPosition.left - cameraX;
    int drawY = m_rcPosition.top - cameraY;

    if (fabs(m_dRotationAngle) < 0.001) // m_dRotationAngle == 0.0 yerine bunu kullanın.
    {
        if (m_iNumFrames == 1)
            m_pBitmap->Draw(hDC, drawX, drawY, TRUE);
        else
            m_pBitmap->DrawPart(hDC, drawX, drawY,
                m_iCurFrame * width, 0, width, height, TRUE);
        return;
    }


    // --- YENİ: Standart GDI ile Döndürerek Çizim ---

    // 1. Gerekli Değişkenleri ve Döndürme Matrisini Ayarla
    double angle = -m_dRotationAngle; // PlgBlt için açıyı ters çevirmek daha sezgisel sonuçlar verebilir. Deneyerek bulun.
    double cosine = cos(angle);
    double sine = sin(angle);

    // Şeffaflık için kullanılacak renk
    COLORREF crTransColor = RGB(255, 0, 255);

    // 2. Köşe Noktalarını Hesapla
    POINT aptCorners[3];
    int halfWidth = width / 2;
    int halfHeight = height / 2;

    // Sol-üst köşe
    aptCorners[0].x = (long)(drawX + halfWidth + (-halfWidth * cosine) - (-halfHeight * sine));
    aptCorners[0].y = (long)(drawY + halfHeight + (-halfWidth * sine) + (-halfHeight * cosine));

    // Sağ-üst köşe
    aptCorners[1].x = (long)(drawX + halfWidth + (halfWidth * cosine) - (-halfHeight * sine));
    aptCorners[1].y = (long)(drawY + halfHeight + (halfWidth * sine) + (-halfHeight * cosine));

    // Sol-alt köşe
    aptCorners[2].x = (long)(drawX + halfWidth + (-halfWidth * cosine) - (halfHeight * sine));
    aptCorners[2].y = (long)(drawY + halfHeight + (-halfWidth * sine) + (halfHeight * cosine));

    // 3. Geçici Bellek DC'si ve Bitmap Oluştur
    // Döndürülmüş resmin sığacağı kadar bir alan gerekir. Köşegen en uzun mesafedir.
    int iDiag = (int)sqrt(width * width + height * height) + 1;
    RECT rcTemp = { 0, 0, iDiag, iDiag };

    HDC hTempDC = CreateCompatibleDC(hDC);
    HBITMAP hTempBitmap = CreateCompatibleBitmap(hDC, iDiag, iDiag);
    HBITMAP hOldTempBitmap = (HBITMAP)SelectObject(hTempDC, hTempBitmap);

    // 4. Geçici Bitmap'in Arka Planını Şeffaf Renkle Doldur
    HBRUSH hTransBrush = CreateSolidBrush(crTransColor);
    FillRect(hTempDC, &rcTemp, hTransBrush);
    DeleteObject(hTransBrush);

    // Döndürülmüş resmi geçici DC'nin ortasına çizmek için köşe noktalarını ayarla
    int iOffsetX = (iDiag - width) / 2;
    int iOffsetY = (iDiag - height) / 2;
    for (int i = 0; i < 3; i++) {
        aptCorners[i].x -= (drawX - iOffsetX);
        aptCorners[i].y -= (drawY - iOffsetY);
    }

    // 5. Orijinal Bitmap için Bellek DC'si Oluştur ve PlgBlt ile Döndür
    HDC hSpriteDC = CreateCompatibleDC(hDC);
    HBITMAP hOldSpriteBitmap = (HBITMAP)SelectObject(hSpriteDC, m_pBitmap->GetHBITMAP());

    SetStretchBltMode(hTempDC, COLORONCOLOR); // Daha iyi kalite için
    PlgBlt(hTempDC, aptCorners, hSpriteDC,
        m_iCurFrame * width, 0, width, height, NULL, 0, 0);

    // 6. Döndürülmüş Geçici Bitmap'i Asıl Ekrana Şeffaf Olarak Çiz
    TransparentBlt(hDC, drawX - (iDiag - width) / 2, drawY - (iDiag - height) / 2,
        iDiag, iDiag, hTempDC, 0, 0, iDiag, iDiag, crTransColor);

    // 7. Temizlik! Oluşturulan tüm GDI nesnelerini sil
    SelectObject(hSpriteDC, hOldSpriteBitmap);
    DeleteDC(hSpriteDC);
    SelectObject(hTempDC, hOldTempBitmap);
    DeleteDC(hTempDC);
    DeleteObject(hTempBitmap);
}

// Draw'ın bu versiyonunu da güncelleyelim ki diğeri üzerinden çalışsın
void Sprite::Draw(HDC hDC)
{
    Draw(hDC, 0, 0);
}