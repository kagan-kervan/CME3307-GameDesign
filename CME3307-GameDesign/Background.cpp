//-----------------------------------------------------------------
// Background Object
// C++ Source - Background.cpp
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "Background.h"

//-----------------------------------------------------------------
// Background Constructor(s)/Destructor
//-----------------------------------------------------------------
Background::Background(int iWidth, int iHeight, COLORREF crColor)
{
  // Initialize the member variables
  m_iWidth = iWidth;
  m_iHeight = iHeight;
  m_crColor = crColor;
  m_pBitmap = NULL;
}

Background::Background(Bitmap* pBitmap)
{
  // Initialize the member variables
  m_crColor = 0;
  m_pBitmap = pBitmap;
  m_iWidth = pBitmap->GetWidth();
  m_iHeight = pBitmap->GetHeight();
}

Background::~Background()
{
}

//-----------------------------------------------------------------
// Background General Methods
//-----------------------------------------------------------------
void Background::Update()
{
  // Do nothing since the basic background is not animated
}

void Background::Draw(HDC hDC)
{
  // Draw the background
  if (m_pBitmap != NULL)
    m_pBitmap->Draw(hDC, 0, 0);
  else
  {
    RECT    rect = { 0, 0, m_iWidth, m_iHeight };
    HBRUSH  hBrush = CreateSolidBrush(m_crColor);
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);
  }
}

void Background::Draw(HDC hDC, int camX, int camY)
{
    // Draw the background
    if (m_pBitmap != NULL)
        m_pBitmap->Draw(hDC, camX, camY);
    else
    {
        RECT    rect = { camX, camY, m_iWidth+camX, m_iHeight+camY };
        HBRUSH  hBrush = CreateSolidBrush(m_crColor);
        FillRect(hDC, &rect, hBrush);
        DeleteObject(hBrush);
    }
}

//-----------------------------------------------------------------
// StarryBackground Constructor
//-----------------------------------------------------------------
StarryBackground::StarryBackground(int iWidth, int iHeight, int iNumStars,
  int iTwinkleDelay) : Background(iWidth, iHeight, 0)
{
  // Initialize the member variables
  m_iNumStars = min(iNumStars, 100);
  m_iTwinkleDelay = iTwinkleDelay;

  // Create the stars
  for (int i = 0; i < iNumStars; i++)
  {
    m_ptStars[i].x = rand() % iWidth;
    m_ptStars[i].y = rand() % iHeight;
    m_crStarColors[i] = RGB(128, 128, 128);
  }
}

StarryBackground::~StarryBackground()
{
}

//-----------------------------------------------------------------
// StarryBackground General Methods
//-----------------------------------------------------------------
void StarryBackground::Update()
{
  // Randomly change the shade of the stars so that they twinkle
  int iRGB;
  for (int i = 0; i < m_iNumStars; i++)
    if ((rand() % m_iTwinkleDelay) == 0)
    {
      iRGB = rand() % 256;
      m_crStarColors[i] = RGB(iRGB, iRGB, iRGB);
    }
}

void StarryBackground::Draw(HDC hDC)
{
  // Draw the solid black background
  RECT    rect = { 0, 0, m_iWidth, m_iHeight };
  HBRUSH  hBrush = CreateSolidBrush(RGB(0, 0, 0));
  FillRect(hDC, &rect, hBrush);
  DeleteObject(hBrush);

  // Draw the stars
  for (int i = 0; i < m_iNumStars; i++)
    SetPixel(hDC, m_ptStars[i].x, m_ptStars[i].y, m_crStarColors[i]);
}


//Scrolling Background
ScrollingBackground::ScrollingBackground(Bitmap* pBitmap, int iScrollSpeedX, int iScrollSpeedY)
    : Background(pBitmap)
{
    m_iScrollX = 0;
    m_iScrollY = 0;
    m_iScrollSpeedX = iScrollSpeedX;
    m_iScrollSpeedY = iScrollSpeedY;
    m_iTileWidth = pBitmap->GetWidth();
    m_iTileHeight = pBitmap->GetHeight();
}

void ScrollingBackground::Update()
{
    // Move the background
    m_iScrollX += m_iScrollSpeedX;
    m_iScrollY += m_iScrollSpeedY;

    // Wrap around when we've scrolled a full tile
    if (m_iScrollX >= m_iTileWidth) m_iScrollX = 0;
    if (m_iScrollX < 0) m_iScrollX = m_iTileWidth - 1;
    if (m_iScrollY >= m_iTileHeight) m_iScrollY = 0;
    if (m_iScrollY < 0) m_iScrollY = m_iTileHeight - 1;
}

void ScrollingBackground::Draw(HDC hDC)
{
    if (m_pBitmap == NULL) return;

    // Calculate how many tiles we need to fill the screen
    int tilesX = (m_iWidth / m_iTileWidth) + 2;  // +2 for partial tiles
    int tilesY = (m_iHeight / m_iTileHeight) + 2;

    // Draw tiles to cover the entire background
    for (int y = -1; y < tilesY; y++)
    {
        for (int x = -1; x < tilesX; x++)
        {
            int drawX = (x * m_iTileWidth) - m_iScrollX;
            int drawY = (y * m_iTileHeight) - m_iScrollY;
            m_pBitmap->Draw(hDC, drawX, drawY);
        }
    }
}
