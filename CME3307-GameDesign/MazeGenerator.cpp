#include "MazeGenerator.h"
#include <iostream>
#include <algorithm>
#include <stack>


MazeGenerator::MazeGenerator(int width, int height)
    : width(width), height(height) {
    // Initialize maze with all walls (-1)
    int mazeWidth = width * 2 + 1;
    int mazeHeight = height * 2 + 1;
    maze = std::vector<std::vector<int>>(mazeHeight, std::vector<int>(mazeWidth, -1));
    isRoomCell = std::vector<std::vector<bool>>(mazeHeight, std::vector<bool>(mazeWidth, false));
    std::random_device rd;
    rng = std::mt19937(rd());
}
void MazeGenerator::generateMaze() {
    // Step 1: Generate rooms
    std::vector<Room> rooms;
    int numRooms = (width * height) / 20;
    for (int i = 0; i < numRooms; i++) {
        std::uniform_int_distribution<int> widthDist(2, 5);
        std::uniform_int_distribution<int> heightDist(2, 5);
        int roomW = widthDist(rng);
        int roomH = heightDist(rng);
        std::uniform_int_distribution<int> xDist(1, width - roomW - 1);
        std::uniform_int_distribution<int> yDist(1, height - roomH - 1);
        int roomX = 1 + 2 * xDist(rng);
        int roomY = 1 + 2 * yDist(rng);

        bool overlap = false;
        for (const auto& r : rooms) {
            if (roomX < r.x + r.w * 2 + 2 && roomX + roomW * 2 > r.x - 2 &&
                roomY < r.y + r.h * 2 + 2 && roomY + roomH * 2 > r.y - 2) {
                overlap = true;
                break;
            }
        }
        if (!overlap) {
            for (int y = roomY; y < roomY + roomH * 2; y++) {
                for (int x = roomX; x < roomX + roomW * 2; x++) {
                    if (isValid(x, y)) {
                        maze[y][x] = 0;
                        isRoomCell[y][x] = true;
                    }
                }
            }
            rooms.push_back(Room(roomX, roomY, roomW, roomH));
        }
    }

    // Step 2: Connect rooms and fill maze
    std::stack<std::pair<int, int>> stack;
    if (!rooms.empty()) {
        int startX = rooms[0].x + 2 * (rng() % rooms[0].w);
        int startY = rooms[0].y + 2 * (rng() % rooms[0].h);
        maze[startY][startX] = 0;
        stack.push({ startX, startY });
    }
    else {
        maze[1][1] = 0;
        stack.push({ 1, 1 });
    }

    while (!stack.empty()) {
        std::pair<int, int> current = stack.top();
        int currentX = current.first;
        int currentY = current.second;
        std::vector<int> neighbors = getUnvisitedNeighbors(currentX, currentY);

        if (!neighbors.empty()) {
            std::uniform_int_distribution<int> dist(0, neighbors.size() - 1);
            int direction = neighbors[dist(rng)];
            int nextX = currentX + dx[direction] * 2;
            int nextY = currentY + dy[direction] * 2;
            carvePassage(currentX, currentY, nextX, nextY);
            stack.push({ nextX, nextY });
        }
        else {
            stack.pop();
        }
    }
}
bool MazeGenerator::isValid(int x, int y) const {
    return y >= 0 && y < maze.size() && x >= 0 && x < maze[0].size();
}

std::vector<int> MazeGenerator::getUnvisitedNeighbors(int x, int y) {
    std::vector<int> neighbors;
    for (int i = 0; i < 4; i++) {
        int nextX = x + dx[i] * 2;
        int nextY = y + dy[i] * 2;
        if (isValid(nextX, nextY) && maze[nextY][nextX] == -1) {
            neighbors.push_back(i);
        }
    }
    return neighbors;
}

void MazeGenerator::carvePassage(int x1, int y1, int x2, int y2) {
    maze[y2][x2] = 0;
    int wallX = (x1 + x2) / 2;
    int wallY = (y1 + y2) / 2;
    maze[wallY][wallX] = 0;
}

const std::vector<std::vector<int>>& MazeGenerator::GetMaze() const {
    return maze;
}

bool MazeGenerator::IsWall(int x, int y) const {
    if (!isValid(x, y)) return true; // Treat out-of-bounds as a wall
    return maze[y][x] == -1; // Correct access: [y][x]
}

// CRITICAL FIX: The original code used maze[x][y], which was inconsistent.
// Now it uses maze[y][x] to match IsWall and other functions.
void MazeGenerator::setValue(int x, int y, int value) {
    if (isValid(x, y)) {
        maze[y][x] = value; // Correct access: [y][x]
    }
}