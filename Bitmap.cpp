// Bitmap.cpp (TAM VE DÜZELTÝLMÝÞ HALÝ)

#include "Bitmap.h"

//=============================================================
//Bitmap Constructors and Destructor
//=============================================================
Bitmap::Bitmap()
    : m_hBitmap(NULL), m_iWidth(0), m_iHeight(0)
{
}

Bitmap::Bitmap(HDC hDC, LPTSTR szFileName)
    : m_hBitmap(NULL), m_iWidth(0), m_iHeight(0)
{
    Create(hDC, szFileName);
}

Bitmap::Bitmap(HDC hDC, UINT uiResID, HINSTANCE hInstance)
    : m_hBitmap(NULL), m_iWidth(0), m_iHeight(0)
{
    Create(hDC, uiResID, hInstance);
}

Bitmap::Bitmap(HDC hDC, int iWidth, int iHeight, COLORREF crColor)
    : m_hBitmap(NULL), m_iWidth(0), m_iHeight(0)
{
    Create(hDC, iWidth, iHeight, crColor);
}

Bitmap::~Bitmap()
{
    Free();
}

//=============================================================
//Bitmap Helper Methods
//=============================================================
void Bitmap::Free()
{
    if (m_hBitmap != NULL)
    {
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
}

//=============================================================
//Bitmap General Methods
//=============================================================
BOOL Bitmap::Create(HDC hDC, LPTSTR szFileName)
{
    Free();

    // Dosyadan yükleme daha basittir ve genellikle daha iyi çalýþýr.
    // LoadImage fonksiyonunu kullanalým, bu daha modern ve güvenlidir.
    m_hBitmap = (HBITMAP)LoadImage(NULL, szFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

    if (m_hBitmap == NULL)
    {
        // Eski yöntemle tekrar dene
        HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return FALSE;
        // ... (Eski dosya okuma kodu buraya gelebilir ama LoadImage genellikle yeterlidir)
        return FALSE;
    }

    // Bitmap boyutlarýný al
    BITMAP bm;
    GetObject(m_hBitmap, sizeof(BITMAP), &bm);
    m_iWidth = bm.bmWidth;
    m_iHeight = bm.bmHeight;

    return TRUE;
}

BOOL Bitmap::Create(HDC hDC, UINT uiResID, HINSTANCE hInstance)
{
    Free();

    // Modern ve güvenli LoadImage fonksiyonunu kullanalým.
    // Bu fonksiyon, kaynaktan bitmap yüklemek için en iyi yoldur.
    m_hBitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(uiResID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    if (m_hBitmap == NULL)
    {
        return FALSE; // Yükleme baþarýsýz
    }

    // Bitmap boyutlarýný al
    BITMAP bm;
    GetObject(m_hBitmap, sizeof(BITMAP), &bm);
    m_iWidth = bm.bmWidth;
    m_iHeight = bm.bmHeight;

    return TRUE;
}

BOOL Bitmap::Create(HDC hDC, int iWidth, int iHeight, COLORREF crColor)
{
    m_hBitmap = CreateCompatibleBitmap(hDC, iWidth, iHeight);
    if (m_hBitmap == NULL)
        return FALSE;

    m_iWidth = iWidth;
    m_iHeight = iHeight;

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBRUSH hBrush = CreateSolidBrush(crColor);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, m_hBitmap);

    RECT rcBitmap = { 0, 0, m_iWidth, m_iHeight };
    FillRect(hMemDC, &rcBitmap, hBrush);

    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
    DeleteObject(hBrush);

    return TRUE;
}

void Bitmap::Draw(HDC hDC, int x, int y, BOOL bTrans, COLORREF crTransColor)
{
    if (m_hBitmap != NULL)
    {
        HDC hMemDC = CreateCompatibleDC(hDC);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, m_hBitmap);

        // GDI TransparentBlt fonksiyonu msimg32.lib kütüphanesini gerektirir.
        // Proje ayarlarýna eklenmiþ olmalý.
        if (bTrans)
            TransparentBlt(hDC, x, y, m_iWidth, m_iHeight, hMemDC, 0, 0,
                m_iWidth, m_iHeight, crTransColor);
        else
            BitBlt(hDC, x, y, m_iWidth, m_iHeight, hMemDC, 0, 0, SRCCOPY);

        SelectObject(hMemDC, hOldBitmap);
        DeleteDC(hMemDC);
    }
}

void Bitmap::DrawPart(HDC hDC, int x, int y, int xPart, int yPart,
    int wPart, int hPart, BOOL bTrans, COLORREF crTransColor)
{
    if (m_hBitmap != NULL)
    {
        HDC hMemDC = CreateCompatibleDC(hDC);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, m_hBitmap);

        if (bTrans)
            TransparentBlt(hDC, x, y, wPart, hPart, hMemDC, xPart, yPart,
                wPart, hPart, crTransColor);
        else
            BitBlt(hDC, x, y, wPart, hPart, hMemDC, xPart, yPart, SRCCOPY);

        SelectObject(hMemDC, hOldBitmap);
        DeleteDC(hMemDC);
    }
}