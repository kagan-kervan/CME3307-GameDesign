#pragma once
#include <vector>
#include <random>

#include <utility> // std::pair için


// Labirentteki hücre tiplerini daha okunaklı hale getirmek için enum
enum class TileType {
    WALL = -1,
    PATH = 0,
    START_POINT = 2,
    END_POINT = 3,
    KEY = 4,
    HEALTH_PACK = 5,
    ARMOR_PACK = 6,
    SECOND_WEAPON = 7,
    WEAPON_AMMO = 8,
    EXTRA_SCORE = 9
};

// Quadrant (Çeyrek) veya alan tanımlamak için kullanılacak yapı
struct Room {
    int x, y; // Sol-üst köşe
    int w, h; // Genişlik ve yükseklik
    Room(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
};


class MazeGenerator {
private:
    int width, height;
    std::vector<std::vector<int>> maze; // Instance variable
    std::mt19937 rng;
    std::vector<std::vector<bool>> isRoomCell;
    const int dx[4] = { 0, 1, 0, -1 };
    const int dy[4] = { -1, 0, 1, 0 };

    std::pair<int, int> startPosition;
    std::pair<int, int> endPosition;

    // Labirentteki hücre tiplerini daha okunaklı hale getirmek için enum
    enum class TileType {
        WALL = -1,
        PATH = 0,
        START_POINT = 2,
        END_POINT = 3,
        KEY = 4,
        HEALTH_PACK = 5,
        ARMOR_PACK = 6,
        SECOND_WEAPON = 7,
        WEAPON_AMMO = 8,
        EXTRA_SCORE = 9
    };

    // These can remain private
    std::vector<int> getUnvisitedNeighbors(int x, int y);
    void carvePassage(int x1, int y1, int x2, int y2);

    // Yeni yardımcı fonksiyonlar
    void PlaceStartEndPoints();
    void PlaceKeysForLevel(int level);
    void PlaceItemsForLevel(int level);
    std::pair<int, int> FindRandomEmptyCellInArea(const Room& area);

public:
    MazeGenerator(int width, int height);
    void generateMaze();

    void ClearMaze(); // Labirenti temizleyip yeniden üretime hazırlar
    void printMaze() const;

    // Ana seviye oluşturma fonksiyonu. Tüm mantık burada toplanacak.
    void SetupLevel(int level);

    // MOVED TO PUBLIC: This function needs to be accessible by the enemy's pathfinding.
    bool isValid(int x, int y) const;

    const std::vector<std::vector<int>>& GetMaze() const;
    void setValue(int x, int y, int value);
    bool IsWall(int x, int y) const;


    // Oyunun başlangıç ve bitiş noktalarını bilmesi için public erişimli değişkenler
    std::pair<int, int> GetStartPos() const { return startPosition; }
    std::pair<int, int> GetEndPos() const { return endPosition; }
};
