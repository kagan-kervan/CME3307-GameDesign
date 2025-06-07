#include "MazeGenerator.h"
#include <iostream>
#include <algorithm>
#include <stack>


MazeGenerator::MazeGenerator(int width, int height)
    : width(width), height(height) {
    // Initialize maze with all walls (-1)
    // We need odd dimensions for proper maze generation
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
    int numRooms = (width * height) / 20; // Rough estimate: e.g., 1 room per 20 cells
    for (int i = 0; i < numRooms; i++) {
        // Random room size (e.g., 2 to 5 cells wide and tall)
        std::uniform_int_distribution<int> widthDist(2, 5);
        std::uniform_int_distribution<int> heightDist(2, 5);
        int roomW = widthDist(rng);
        int roomH = heightDist(rng);
        // Random top-left position, scaled for maze grid, with padding
        std::uniform_int_distribution<int> xDist(1, width - roomW - 1);
        std::uniform_int_distribution<int> yDist(1, height - roomH - 1);
        int roomX = 1 + 2 * xDist(rng); // Scale to maze grid
        int roomY = 1 + 2 * yDist(rng);

        // Check for overlap with existing rooms (with 1-cell buffer)
        bool overlap = false;
        for (const auto& r : rooms) {
            if (roomX < r.x + r.w * 2 + 2 && roomX + roomW * 2 > r.x - 2 &&
                roomY < r.y + r.h * 2 + 2 && roomY + roomH * 2 > r.y - 2) {
                overlap = true;
                break;
            }
        }
        if (!overlap) {
            // Carve the room and mark cells as part of a room
            for (int y = roomY; y < roomY + roomH * 2; y++) { // No step, fill all cells
                for (int x = roomX; x < roomX + roomW * 2; x++) {
                    if (isValid(x, y)) {
                        maze[y][x] = 0; // Set all cells in room to path
                        isRoomCell[y][x] = true; // Mark as room cell
                    }
                }
            }
            rooms.push_back(Room(roomX, roomY, roomW, roomH));
        }
    }

    // Step 2: Connect rooms and fill maze
    std::stack<std::pair<int, int>> stack;
    // Start from a cell in the first room
    if (!rooms.empty()) {
        int startX = rooms[0].x + 2 * (rng() % rooms[0].w);
        int startY = rooms[0].y + 2 * (rng() % rooms[0].h);
        maze[startY][startX] = 0;
        stack.push({ startX, startY });
    }
    else {
        // Fallback if no rooms
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
            std::uniform_int_distribution<int> chance(0, 4);
            if (chance(rng) != 0) {
                stack.pop();
            }
        }

        // Occasionally carve extra passages for loops
        std::uniform_int_distribution<int> loopChance(0, 20); // 5% chance
        if (loopChance(rng) == 0) {
            int x = 1 + 2 * (rng() % width);
            int y = 1 + 2 * (rng() % height);
            std::vector<int> loopDirs = getUnvisitedNeighbors(x, y);
            if (!loopDirs.empty()) {
                int dir = loopDirs[rng() % loopDirs.size()];
                int nx = x + dx[dir] * 2;
                int ny = y + dy[dir] * 2;
                if (isValid(nx, ny)) {
                    carvePassage(x, y, nx, ny);
                }
            }
        }
    }

    // Step 3: Ensure rooms are connected
    for (size_t i = 0; i < rooms.size(); i++) {
        int x = rooms[i].x + 2 * (rng() % rooms[i].w);
        int y = rooms[i].y + 2 * (rng() % rooms[i].h);
        std::vector<int> neighbors = getUnvisitedNeighbors(x, y);
        if (!neighbors.empty()) {
            int dir = neighbors[rng() % neighbors.size()];
            int nx = x + dx[dir] * 2;
            int ny = y + dy[dir] * 2;
            if (isValid(nx, ny)) {
                carvePassage(x, y, nx, ny);
            }
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


void MazeGenerator::printMaze() const {
    for (const auto& row : maze) {
        for (int cell : row) {
            std::cout << (cell == -1 ? "x" : " ");
        }
        std::cout << "\n";
    }
}

// Getter for maze
const std::vector<std::vector<int>>& MazeGenerator::GetMaze() const {
    return maze;
}

// Checks if a cell is a wall
bool MazeGenerator::IsWall(int x, int y) const {
    if (x < 0 || y < 0 || x >= 2*width || y >= 2*height)
        return true;
    return maze[y][x] == -1;
}

void MazeGenerator::setValue(int x, int y, int value) {
    if (x >= 0 && x < maze.size() && y >= 0 && y < maze[x].size()) {
        maze[x][y] = value;
    }
}