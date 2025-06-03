#pragma once
#include <vector>
#include <random>
#include <stack>

class MazeGenerator {
private:
    int width, height;
    std::vector<std::vector<int>> maze; // -1 for walls, 0 for paths
    std::mt19937 rng;

    // Directions: North, East, South, West
    const int dx[4] = { 0, 1, 0, -1 };
    const int dy[4] = { -1, 0, 1, 0 };

    bool isValid(int x, int y) const;
    std::vector<int> getUnvisitedNeighbors(int x, int y);
    void carvePassage(int x1, int y1, int x2, int y2);

public:
    MazeGenerator(int width, int height);
    void generateMaze();
    const std::vector<std::vector<int>>& getMaze() const;
    void printMaze() const;
};