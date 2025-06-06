#include "Maze.h"

Maze::Maze(Bitmap* pWallBitmap, Bitmap* pFloorBitmap)
{
    m_pWallBitmap = pWallBitmap;
    m_pFloorBitmap = pFloorBitmap;
    Generate();
}

Maze::~Maze() {}

void Maze::Generate()
{
    for (int y = 0; y < MAZE_HEIGHT; ++y)
    {
        for (int x = 0; x < MAZE_WIDTH; ++x)
        {
            if (y == 0 || y == MAZE_HEIGHT - 1 || x == 0 || x == MAZE_WIDTH - 1 || (y == 10 && x > 9 && x < 25) || (x == 20 && y > 14 && y < 30))
            {
                m_data[y][x] = 1;
            }
            else
            {
                m_data[y][x] = 0;
            }
        }
    }
}

void Maze::Draw(HDC hDC, RECT& camera)
{
    int startTileX = camera.left / TILE_SIZE;
    int startTileY = camera.top / TILE_SIZE;
    int endTileX = (camera.right / TILE_SIZE) + 2;
    int endTileY = (camera.bottom / TILE_SIZE) + 2;

    for (int y = startTileY; y < endTileY; ++y)
    {
        for (int x = startTileX; x < endTileX; ++x)
        {
            if (x < 0 || x >= MAZE_WIDTH || y < 0 || y >= MAZE_HEIGHT) continue;

            int drawX = (x * TILE_SIZE) - camera.left;
            int drawY = (y * TILE_SIZE) - camera.top;

            if (m_data[y][x] == 1)
            {
                if (m_pWallBitmap) m_pWallBitmap->Draw(hDC, drawX, drawY);
            }
            else
            {
                if (m_pFloorBitmap) m_pFloorBitmap->Draw(hDC, drawX, drawY);
            }
        }
    }
}

bool Maze::IsWall(int x, int y)
{
    if (x < 0 || y < 0) return true;
    int tileX = x / TILE_SIZE;
    int tileY = y / TILE_SIZE;
    return IsWallAtTile(tileX, tileY);
}

bool Maze::IsWallAtTile(int tileX, int tileY)
{
    if (tileX < 0 || tileX >= MAZE_WIDTH || tileY < 0 || tileY >= MAZE_HEIGHT)
    {
        return true;
    }
    return m_data[tileY][tileX] == 1;
}