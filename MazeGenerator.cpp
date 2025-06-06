#include "MazeGenerator.h"
#include <iostream>
#include <algorithm>

MazeGenerator::MazeGenerator(int width, int height)
    : width(width), height(height) {
    // Initialize maze with all walls (-1)
    // We need odd dimensions for proper maze generation
    int mazeWidth = width * 2 + 1;
    int mazeHeight = height * 2 + 1;
    maze = std::vector<std::vector<int>>(mazeHeight, std::vector<int>(mazeWidth, -1));

    std::random_device rd;
    rng = std::mt19937(rd());
}

void MazeGenerator::generateMaze() {
    // Start from position (1,1) - first cell position
    std::stack<std::pair<int, int>> stack;

    // Mark starting cell as path
    maze[1][1] = 0;
    stack.push({ 1, 1 });

    while (!stack.empty()) {
        std::pair<int, int> current = stack.top();
        int currentX = current.first;
        int currentY = current.second;

        // Get unvisited neighbors (2 cells away to account for walls)
        std::vector<int> neighbors = getUnvisitedNeighbors(currentX, currentY);

        if (!neighbors.empty()) {
            // Choose random neighbor
            std::uniform_int_distribution<int> dist(0, neighbors.size() - 1);
            int direction = neighbors[dist(rng)];

            // Calculate neighbor position (2 cells away)
            int nextX = currentX + dx[direction] * 2;
            int nextY = currentY + dy[direction] * 2;

            // Carve passage to neighbor
            carvePassage(currentX, currentY, nextX, nextY);

            // Add neighbor to stack
            stack.push({ nextX, nextY });
        }
        else {
            // Backtrack
            stack.pop();
        }
    }
}

bool MazeGenerator::isValid(int x, int y) const {
    return x >= 0 && x < maze[0].size() && y >= 0 && y < maze.size();
}

std::vector<int> MazeGenerator::getUnvisitedNeighbors(int x, int y) {
    std::vector<int> neighbors;

    for (int i = 0; i < 4; i++) {
        int nextX = x + dx[i] * 2; // 2 cells away
        int nextY = y + dy[i] * 2;

        if (isValid(nextX, nextY) && maze[nextY][nextX] == -1) {
            neighbors.push_back(i);
        }
    }

    return neighbors;
}

void MazeGenerator::carvePassage(int x1, int y1, int x2, int y2) {
    // Mark destination cell as path
    maze[y2][x2] = 0;

    // Carve the wall between current and next cell
    int wallX = (x1 + x2) / 2;
    int wallY = (y1 + y2) / 2;
    maze[wallY][wallX] = 0;
}

const std::vector<std::vector<int>>& MazeGenerator::getMaze() const {
    return maze;
}

void MazeGenerator::printMaze() const {
    for (const auto& row : maze) {
        for (int cell : row) {
            std::cout << (cell == -1 ? "x" : " ");
        }
        std::cout << "\n";
    }
}

// Example usage
int main() {
    MazeGenerator maze(2, 2);  // This will create a 51x25 actual maze array
    maze.generateMaze();
    maze.printMaze();

    // Access the maze array
    const auto& mazeArray = maze.getMaze();
    std::cout << "\nMaze dimensions: " << mazeArray[0].size() << "x" << mazeArray.size() << std::endl;
    std::cout << "Walls are -1, paths are 0" << std::endl;

    return 0;
}