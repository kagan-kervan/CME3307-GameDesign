#pragma once
#include <vector>
#include <random>

class MazeGenerator {
private:
    int width, height;
    std::vector<std::vector<int>> maze; // Instance variable
    std::mt19937 rng;
    std::vector<std::vector<bool>> isRoomCell;
    const int dx[4] = { 0, 1, 0, -1 };
    const int dy[4] = { -1, 0, 1, 0 };

    bool isValid(int x, int y) const;
    std::vector<int> getUnvisitedNeighbors(int x, int y);
    void carvePassage(int x1, int y1, int x2, int y2);

public:
    MazeGenerator(int width, int height);
    void generateMaze();
    void printMaze() const;

    // ✅ Non-static function to access instance maze
    const std::vector<std::vector<int>>& GetMaze() const;

    void setValue(int x,int y,int value);

    // ✅ Non-static IsWall function
    bool IsWall(int x, int y) const;
};
struct Room {
    int x,y; // Top-left corner
    int w,h; // Width and height
    Room(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
};