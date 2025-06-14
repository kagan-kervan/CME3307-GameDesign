// FOVBackground.cpp

#include "FOVBackground.h"
#include "GameEngine.h" // Ekran boyutlarýný almak için eklendi
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

// YENÝ: Açýlarý yumuþak bir þekilde birleþtirmek için yardýmcý fonksiyon (dönme takýlmasýný çözer)
// Ýki açý arasýndaki en kýsa yolu bularak interpolasyon yapar.
double LerpAngle(double a, double b, float t)
{
    double diff = b - a;
    // -PI ve PI aralýðýný aþan dönüþleri düzeltir
    if (diff > PI) diff -= 2 * PI;
    if (diff < -PI) diff += 2 * PI;
    return a + diff * t;
}


FOVBackground::FOVBackground(Sprite* pPlayer, int iFOVDegrees, int iFOVDistance)
    : m_pPlayer(pPlayer), m_iFOVDegrees(iFOVDegrees), m_iFOVDistance(iFOVDistance), m_dDirection(0.0)
{
    m_ptMouse.x = 0;
    m_ptMouse.y = 0;
}

// DEÐÝÞÝKLÝK: Fare hareketlerini ve fener açýsýný yumuþatmak için Update güncellendi
SPRITEACTION FOVBackground::Update(int cameraX, int cameraY)
{
    if (m_pPlayer)
    {
        // Oyuncunun dünya koordinatlarýndaki merkezi
        RECT rcPos = m_pPlayer->GetPosition();
        int playerX = rcPos.left + (rcPos.right - rcPos.left) / 2;
        int playerY = rcPos.top + (rcPos.bottom - rcPos.top) / 2;

        // Fare pozisyonunu dünya koordinatlarýna çevir
        int mouseWorldX = m_ptMouse.x + cameraX;
        int mouseWorldY = m_ptMouse.y + cameraY;

        // Hedef açýyý hesapla
        double targetDirection = atan2((double)mouseWorldY - playerY, (double)mouseWorldX - playerX);

        // YENÝ: Açýyý anýnda deðiþtirmek yerine yumuþak geçiþ yap (fare takýlmasýný önler)
        m_dDirection = LerpAngle(m_dDirection, targetDirection, 0.25f);
    }
    return SA_NONE;
}

// DEÐÝÞÝKLÝK: Draw metodu tamamen yeniden yazýldý. Artýk beyaz bir üçgen çizmek yerine,
// fenerin dýþýnda kalan alaný karartan bir katman çiziyor.
void FOVBackground::Draw(HDC hDC, int cameraX, int cameraY)
{
    if (m_pPlayer == NULL)
        return;

    GameEngine* pGame = GameEngine::GetEngine();
    int screenWidth = pGame->GetWidth();
    int screenHeight = pGame->GetHeight();

    // 1. Geçici bir hafýza DC'si ve karanlýk katmaný için bitmap oluþtur
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
    int playerScreenX = (rcPos.left + m_pPlayer->GetWidth() / 2) - cameraX;
    int playerScreenY = (rcPos.top + m_pPlayer->GetHeight() / 2) - cameraY;

    // 4. Fener konisinin silineceði bir "þeffaf" fýrça oluþtur
    // Bu fýrça, siyah katman üzerinde "delik açmak" için kullanýlacak.
    HBRUSH transparentBrush = (HBRUSH)GetStockObject(NULL_BRUSH); // Veya özel bir desenli fýrça
    HPEN   noPen = CreatePen(PS_NULL, 0, 0); // Kenar çizgisi olmasýn
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, transparentBrush);
    HPEN   oldPen = (HPEN)SelectObject(memDC, noPen);

    // 5. Daha gerçekçi bir fener konisi için poligon noktalarýný hesapla
    // YAKINDA DAR, UZAKTA GENÝÞ BÝR Koni
    const int numPoints = 20; // Koninin kenarlarýný daha pürüzsüz yapmak için nokta sayýsý
    POINT points[numPoints + 2];
    points[0] = { playerScreenX, playerScreenY }; // Ýlk nokta her zaman oyuncunun merkezi

    double halfFOV = (m_iFOVDegrees * PI / 180.0) / 2.0; // Radyan cinsinden
    double startAngle = m_dDirection - halfFOV;
    double endAngle = m_dDirection + halfFOV;
    double angleStep = (endAngle - startAngle) / (numPoints - 1);

    for (int i = 0; i < numPoints; ++i)
    {
        double currentAngle = startAngle + i * angleStep;
        points[i + 1].x = playerScreenX + (int)(cos(currentAngle) * m_iFOVDistance);
        points[i + 1].y = playerScreenY + (int)(sin(currentAngle) * m_iFOVDistance);
    }

    // 6. Bu poligonu kullanarak siyah katman üzerinde bir "bölge" oluþturup silelim.
    // GDI'da transparanlýk karmaþýk olduðundan, en kolay yol, bu alaný "beyaz" ile doldurup
    // sonra tüm katmaný özel bir ROP kodu ile birleþtirmektir.
    // VEYA DAHA BASÝTÝ: Bu alaný baþka bir renkle boyayýp TransparentBlt kullanmak.
    // ÞÝMDÝLÝK EN BASÝT YÖNTEM: AlphaBlend.

    // Polygon'u "þeffaf" (siyah) olmayan bir renkle (örn: beyaz) çizelim.
    // Ardýndan, bu katmaný ekrana AlphaBlend ile çizerken,
    // kaynak ve hedef pikselleri arasýnda özel bir iþlem yapacaðýz.
    // Bu, GDI'nin en zorlu kýsýmlarýndan biridir.
    // EN TEMÝZ ÇÖZÜM:
    // a) Siyah katmaný çiz.
    // b) Fener poligonunu bu katmandan "sil". Bunun için bir RGN (bölge) kullanacaðýz.

    HRGN hRgn = CreatePolygonRgn(points, numPoints + 1, WINDING);
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255)); // Geçici olarak beyaz fýrça
    FillRgn(memDC, hRgn, whiteBrush); // Bölgeyi beyaza boyayarak "delik" açýyoruz.
    DeleteObject(whiteBrush);
    DeleteObject(hRgn);


    // 7. Karanlýk katmaný, beyaz kýsýmlarý (yani fenerin olduðu yer) þeffaf olacak þekilde
    // ana ekrana (hDC) çiz.
    // TransparentBlt, beyaz rengi þeffaf yapar ve kalan siyahý ekrana çizer.
    // Bu, siyah bir maske üzerine delik açma efekti verir.
    TransparentBlt(hDC, 0, 0, screenWidth, screenHeight,
        memDC, 0, 0, screenWidth, screenHeight, RGB(255, 255, 255));

    // Alternatif ve daha iyi görünen yöntem: AlphaBlend
    // Bu yöntem, siyah katmaný yarý saydam olarak çizer ve fener efekti verir.
    // BLENDFUNCTION blend = { AC_SRC_OVER, 0, 192, 0 }; // 192 = %75 opaklýk
    // AlphaBlend(hDC, 0, 0, screenWidth, screenHeight, memDC, 0, 0, screenWidth, screenHeight, blend);

    // 8. Temizlik
    SelectObject(memDC, oldBitmap);
    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(noPen);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}


void FOVBackground::Draw(HDC hDC)
{
    Draw(hDC, 0, 0);
}