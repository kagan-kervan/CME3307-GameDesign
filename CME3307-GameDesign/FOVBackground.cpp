// FOVBackground.cpp

#include "FOVBackground.h"
#include "GameEngine.h"
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

double LerpAngle(double a, double b, float t)
{
    double diff = b - a;
    if (diff > PI) diff -= 2 * PI;
    if (diff < -PI) diff += 2 * PI;
    return a + diff * t;
}

// YENÝ: Kurucu metot güncellendi
FOVBackground::FOVBackground(Sprite* pPlayer, int iFOVDegrees, int iFOVDistance, int iPlayerLightRadius)
    : m_pPlayer(pPlayer),
    m_iFOVDegrees(iFOVDegrees),
    m_iFOVDistance(iFOVDistance),
    m_dDirection(0.0),
    m_iPlayerLightRadius(iPlayerLightRadius) // YENÝ: Üye deðiþkeni baþlat
{
    m_ptMouse.x = 0;
    m_ptMouse.y = 0;
}

SPRITEACTION FOVBackground::Update(int cameraX, int cameraY)
{
    if (m_pPlayer)
    {
        RECT rcPos = m_pPlayer->GetPosition();
        int playerX = rcPos.left + (rcPos.right - rcPos.left) / 2;
        int playerY = rcPos.top + (rcPos.bottom - rcPos.top) / 2;

        int mouseWorldX = m_ptMouse.x + cameraX;
        int mouseWorldY = m_ptMouse.y + cameraY;

        double targetDirection = atan2((double)mouseWorldY - playerY, (double)mouseWorldX - playerX);
        m_dDirection = LerpAngle(m_dDirection, targetDirection, 0.25f);
    }
    return SA_NONE;
}

void FOVBackground::Draw(HDC hDC, int cameraX, int cameraY)
{
    if (m_pPlayer == NULL)
        return;

    GameEngine* pGame = GameEngine::GetEngine();
    if (!pGame) return; // pGame null ise çýk
    int screenWidth = pGame->GetWidth();
    int screenHeight = pGame->GetHeight();

    // 1. Geçici hafýza DC'si ve karanlýk katmaný için bitmap oluþtur
    HDC memDC = CreateCompatibleDC(hDC);
    HBITMAP memBitmap = CreateCompatibleBitmap(hDC, screenWidth, screenHeight);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    // 2. Bu geçici katmaný tamamen siyahla doldur
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    RECT rect = { 0, 0, screenWidth, screenHeight };
    FillRect(memDC, &rect, blackBrush);
    DeleteObject(blackBrush);

    // 3. Oyuncunun ekrandaki pozisyonunu hesapla
    RECT rcPos = m_pPlayer->GetPosition();
    // Oyuncu sprite'ýnýn geniþlik ve yüksekliðini almamýz gerek.
    // Sprite sýnýfýnda GetWidth() ve GetHeight() zaten var.
    int playerSpriteWidth = 0;
    int playerSpriteHeight = 0;
    if (m_pPlayer->GetBitmap()) { // Bitmap null deðilse boyutlarý al
        playerSpriteWidth = m_pPlayer->GetWidth();
        playerSpriteHeight = m_pPlayer->GetHeight();
    }

    int playerScreenX = (rcPos.left + playerSpriteWidth / 2) - cameraX;
    int playerScreenY = (rcPos.top + playerSpriteHeight / 2) - cameraY;


    // 4. Þeffaflýk için kullanýlacak beyaz fýrça (TransparentBlt bu rengi þeffaf yapacak)
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, whiteBrush); // Beyaz fýrçayý seç
    HPEN   noPen = CreatePen(PS_NULL, 0, 0); // Kenar çizgisi olmasýn
    HPEN   oldPen = (HPEN)SelectObject(memDC, noPen);


    // 5. FENER KONÝSÝ BÖLGESÝNÝ OLUÞTUR VE BEYAZA BOYA (ÞEFFAF YAP)
    const int numConePoints = 20;
    POINT conePoints[numConePoints + 2]; // Polygon için nokta dizisi (merkez + kenar noktalarý)
    conePoints[0] = { playerScreenX, playerScreenY };

    double halfFOV = (m_iFOVDegrees * PI / 180.0) / 2.0;
    double startAngle = m_dDirection - halfFOV;
    // double endAngle = m_dDirection + halfFOV; // Bu kullanýlmýyor, angleStep için yeterli
    double angleStep = (2.0 * halfFOV) / (numConePoints - 1); // Toplam FOV açýsý / (nokta sayýsý - 1)

    for (int i = 0; i < numConePoints; ++i)
    {
        double currentAngle = startAngle + i * angleStep;
        conePoints[i + 1].x = playerScreenX + (int)(cos(currentAngle) * m_iFOVDistance);
        conePoints[i + 1].y = playerScreenY + (int)(sin(currentAngle) * m_iFOVDistance);
    }

    // Fener konisi için bir Region oluþtur
    HRGN hConeRgn = CreatePolygonRgn(conePoints, numConePoints + 1, WINDING);

    // 6. YENÝ: OYUNCU ETRAFINDAKÝ DAÝRESEL IÞIK ALANI ÝÇÝN BÖLGE OLUÞTUR
    HRGN hCircleRgn = CreateEllipticRgn(
        playerScreenX - m_iPlayerLightRadius,
        playerScreenY - m_iPlayerLightRadius,
        playerScreenX + m_iPlayerLightRadius,
        playerScreenY + m_iPlayerLightRadius
    );

    // 7. YENÝ: Ýki bölgeyi birleþtir (OR iþlemi ile)
    // Önce birleþik bölgeyi tutacak boþ bir RGN oluþturmak iyi bir pratiktir.
    HRGN hCombinedRgn = CreateRectRgn(0, 0, 1, 1); // Geçici küçük bir bölge
    int combineResult = CombineRgn(hCombinedRgn, hConeRgn, hCircleRgn, RGN_OR);

    // 8. BÝRLEÞÝK BÖLGEYÝ BEYAZA BOYA (ÞEFFAF YAP)
    if (combineResult != ERROR && combineResult != NULLREGION) {
        FillRgn(memDC, hCombinedRgn, whiteBrush);
    }
    else { // Eðer combine baþarýsýz olursa veya boþ bölge dönerse, ayrý ayrý boya
        FillRgn(memDC, hConeRgn, whiteBrush);
        FillRgn(memDC, hCircleRgn, whiteBrush); // Daireyi de beyazla doldur
    }

    // Oluþturulan GDI nesnelerini sil
    DeleteObject(hConeRgn);
    DeleteObject(hCircleRgn);
    DeleteObject(hCombinedRgn); // Birleþik bölgeyi de sil

    // 9. Karanlýk katmaný, beyaz kýsýmlarý (fener ve oyuncu ýþýðý) þeffaf olacak þekilde
    // ana ekrana (hDC) çiz.
    TransparentBlt(hDC, 0, 0, screenWidth, screenHeight,
        memDC, 0, 0, screenWidth, screenHeight, RGB(255, 255, 255));

    // 10. Temizlik
    SelectObject(memDC, oldBitmap);
    SelectObject(memDC, oldBrush); // Eski fýrçayý geri yükle
    SelectObject(memDC, oldPen);   // Eski kalemi geri yükle
    DeleteObject(whiteBrush);      // Oluþturulan beyaz fýrçayý sil
    DeleteObject(noPen);           // Oluþturulan kalemi sil
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// Bu Draw metodu genellikle kamera olmadan çaðrýlmaz ama yine de býrakýyorum.
void FOVBackground::Draw(HDC hDC)
{
    Draw(hDC, 0, 0);
}