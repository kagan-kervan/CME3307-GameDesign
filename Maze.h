// Maze.h
#pragma once
#include "Bitmap.h"
#include <Windows.h>

// Labirentimizin ve karolarýmýzýn boyutlarýný tanýmlayalým
const int TILE_SIZE = 32; // Bitmap'lerinizin boyutu (32x32 piksel varsayýyoruz)
const int MAZE_WIDTH = 50; // Labirentin geniþliði (karo sayýsý)
const int MAZE_HEIGHT = 40; // Labirentin yüksekliði (karo sayýsý)

class Maze
{
public:
    Maze(Bitmap* pWallBitmap, Bitmap* pFloorBitmap);
    ~Maze();

    // Labirenti çizer (kamera pozisyonuna göre)
    void Draw(HDC hDC, RECT& camera);

    // Verilen koordinat bir duvar mý?
    bool IsWall(int x, int y);

    // Verilen karo koordinatý bir duvar mý?
    bool IsWallAtTile(int tileX, int tileY);

private:
    void Generate(); // Örnek bir labirent oluþturur

    Bitmap* m_pWallBitmap;
    Bitmap* m_pFloorBitmap;
    int m_data[MAZE_HEIGHT][MAZE_WIDTH]; // Labirent verisi (0 = zemin, 1 = duvar)
};