#pragma once

#include <windows.h>
#include <vector>
#include "Sprite.h"
#include "GameEngine.h"

#include "Bitmap.h"

// Key locations in quadrants (1: top-right, 2: bottom-right, 3: bottom-left, 4: top-left)
enum class Quadrant {
    TOP_RIGHT = 1,
    BOTTOM_RIGHT = 2,
    BOTTOM_LEFT = 3,
    TOP_LEFT = 4,
    CENTER = 5
};

// Item types for collectibles
enum class ItemType {
    HEALTH,      // +20 health
    ARMOR,       // +20 armor
    WEAPON,      // Second weapon
    AMMO,        // Second weapon ammo
    POINTS       // Extra points
}; 
enum MazeElement {
    PATH = 0,
    WALL = 1,
    PLAYER = 2,
    END_POINT = 3,
    KEY = 4,
    HEALTH_ITEM = 5,
    ARMOR_ITEM = 6,
    WEAPON_ITEM = 7,
    AMMO_ITEM = 8,
    POINT_ITEM = 9,
    ENEMY = 10
};

class Level {
protected:
    int currentLevel;
    int timeLimit;        // 5 minutes in milliseconds
    int startTime;        // Level start time
    int** maze;          // 2D maze array
    std::vector<POINT> keys;
    std::vector<std::pair<POINT, ItemType>> items;
    std::vector<POINT> enemies;
    POINT startPos;      // Starting position (1,1)
    POINT endPos;        // End position (size-2, size-2)
    int score;
    int waveInterval;    // 30 seconds in milliseconds
    int lastWaveTime;    // Time of last enemy wave
    int enemiesPerWave;  // 6 + level number
    Bitmap* keyBitmap;
    Bitmap* healthBitmap;
    Bitmap* armorBitmap;
    Bitmap* ammoBitmap;
    Bitmap* weaponBitmap;
    Bitmap* pointBitmap;


public:
    Level(int level);
    ~Level();
    static const int MAZE_SIZE = 31; // Must be odd for proper maze generation
    static const int TILE_SIZE = 50;
    // Maze generation and setup
    void GenerateMaze();
    void SetupLevel();
    void PlaceKeys();
    bool PlaceKeyInQuadrant(Quadrant quadrant);
    void PlaceItems();
    bool ValidatePathToEnd();
    void ClearArea(int x, int y, int size);

    // Enemy management
    void SpawnEnemyWave(int currentTime);
    void SpawnStaticGuardians();

    // Level state checks
    bool IsLevelComplete();
    bool IsTimeExpired(int currentTime);
    int GetTimeRemaining(int currentTime);

    // Scoring
    void AddScore(int points);
    int CalculateFinalScore(int timeUsed, int remainingHp, int keysCollected, int enemiesKilled, int itemsCollected);

    // Accessors
    int GetCurrentLevel() const { return currentLevel; }
    int GetMazeValue(int x, int y) const { return maze[y][x]; }
    const std::vector<POINT>& GetKeys() const { return keys; }
    const std::vector<std::pair<POINT, ItemType>>& GetItems() const { return items; }
    const std::vector<POINT>& GetEnemies() const { return enemies; }
    POINT GetStartPosition() const { return startPos; }
    POINT GetEndPosition() const { return endPos; }
    int GetScore() const { return score; }

private:
    void InitializeMaze();
    bool IsValidKeyLocation(POINT p, int minDistance);
    Quadrant GetQuadrant(POINT p) const;
    bool IsInClearArea(int x, int y) const;
    void RecursiveBacktracking(int x, int y);
    bool HasValidPath(POINT start, POINT end);
};