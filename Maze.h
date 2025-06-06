// Maze.h
#pragma once
#include "Bitmap.h"
#include <Windows.h>

// Labirentimizin ve karolar�m�z�n boyutlar�n� tan�mlayal�m
const int TILE_SIZE = 32; // Bitmap'lerinizin boyutu (32x32 piksel varsay�yoruz)
const int MAZE_WIDTH = 50; // Labirentin geni�li�i (karo say�s�)
const int MAZE_HEIGHT = 40; // Labirentin y�ksekli�i (karo say�s�)

class Maze
{
public:
    Maze(Bitmap* pWallBitmap, Bitmap* pFloorBitmap);
    ~Maze();

    // Labirenti �izer (kamera pozisyonuna g�re)
    void Draw(HDC hDC, RECT& camera);

    // Verilen koordinat bir duvar m�?
    bool IsWall(int x, int y);

    // Verilen karo koordinat� bir duvar m�?
    bool IsWallAtTile(int tileX, int tileY);

private:
    void Generate(); // �rnek bir labirent olu�turur

    Bitmap* m_pWallBitmap;
    Bitmap* m_pFloorBitmap;
    int m_data[MAZE_HEIGHT][MAZE_WIDTH]; // Labirent verisi (0 = zemin, 1 = duvar)
};