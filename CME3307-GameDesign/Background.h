//-----------------------------------------------------------------
// Background Object
// C++ Header - Background.h
//-----------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include <windows.h>
#include "Bitmap.h"

//-----------------------------------------------------------------
// Background Class
//-----------------------------------------------------------------
class Background
{
protected:
  // Member Variables
  int       m_iWidth, m_iHeight;
  COLORREF  m_crColor;
  Bitmap*   m_pBitmap;

public:
  // Constructor(s)/Destructor
          Background(int iWidth, int iHeight, COLORREF crColor);
          Background(Bitmap* pBitmap);
  virtual ~Background();

  // General Methods
  virtual void  Update();
  virtual void  Draw(HDC hDC);

  // Accessor Methods
  int GetWidth()  { return m_iWidth; };
  int GetHeight() { return m_iHeight; };
};

//-----------------------------------------------------------------
// Starry Background Class
//-----------------------------------------------------------------
class StarryBackground : Background
{
protected:
  // Member Variables
  int       m_iNumStars;
  int       m_iTwinkleDelay;
  POINT     m_ptStars[100];
  COLORREF  m_crStarColors[100];

public:
  // Constructor(s)/Destructor
          StarryBackground(int iWidth, int iHeight, int iNumStars = 100,
            int iTwinkleDelay = 50);
  virtual ~StarryBackground();

  // General Methods
  virtual void  Update();
  virtual void  Draw(HDC hDC);
};

//Scrolling Background Class
class ScrollingBackground : public Background
{
protected:
    int m_iScrollX, m_iScrollY;     // Current scroll position
    int m_iScrollSpeedX, m_iScrollSpeedY; // Pixels per frame
    int m_iTileWidth, m_iTileHeight; // Size of repeating tile

public:
    ScrollingBackground(Bitmap* pBitmap, int iScrollSpeedX, int iScrollSpeedY);
    virtual void Update();
    virtual void Draw(HDC hDC);

    // Methods to control scrolling
    void SetScrollSpeed(int iSpeedX, int iSpeedY);
    void SetScrollPosition(int iX, int iY);
};
