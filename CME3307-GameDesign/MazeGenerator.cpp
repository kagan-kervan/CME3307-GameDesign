﻿#include "MazeGenerator.h"
#include <iostream>
#include <algorithm>
#include <stack>


MazeGenerator::MazeGenerator(int width, int height)
    : width(width), height(height) {
    // Initialize maze with all walls (-1)
    std::random_device rd;
    rng = std::mt19937(rd());
    // Boyutları burada ayarlıyoruz, ClearMaze içinde tekrar yapacağız
    ClearMaze();
}
void MazeGenerator::generateMaze() {
    // Step 1: Generate rooms
    std::vector<Room> rooms;
    int numRooms = (width * height) / 20;
    for (int i = 0; i < numRooms; i++) {
        // ... (room size and position logic remains the same)
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
                        // CHANGED: Use the enum for a path
                        maze[y][x] = (int)TileType::PATH;
                        isRoomCell[y][x] = true;
                    }
                }
            }
            rooms.push_back(Room(roomX, roomY, roomW, roomH));
        }
    }

    // Step 2: Connect rooms and fill maze using Randomized DFS
    std::stack<std::pair<int, int>> stack;
    if (!rooms.empty()) {
        // Start from a random point in the first room
        int startX = rooms[0].x + 2 * (rng() % rooms[0].w);
        int startY = rooms[0].y + 2 * (rng() % rooms[0].h);

        // CHANGED: Use the enum for a path
        maze[startY][startX] = (int)TileType::PATH;
        stack.push({ startX, startY });
    }
    else {
        // Fallback if no rooms were generated
        // CHANGED: Use the enum for a path
        maze[1][1] = (int)TileType::PATH;
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
            carvePassage(currentX, currentY, nextX, nextY); // This function also needs updating
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
    // CHANGED: Carve the new cell as a path
    maze[y2][x2] = (int)TileType::PATH;

    // Get the wall between the two cells
    int wallX = (x1 + x2) / 2;
    int wallY = (y1 + y2) / 2;

    // CHANGED: Carve the wall as a path
    maze[wallY][wallX] = (int)TileType::PATH;
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

void MazeGenerator::SetupLevel(int level) {
    // 1. Her şeyi sıfırla
    ClearMaze();

    // 2. Labirenti oluştur (temel yollar ve duvarlar)
    generateMaze(); // Bu fonksiyon artık sadece duvar ve yol (-1 ve 0) oluşturuyor

    // 3. Başlangıç ve bitiş noktalarını yerleştir
    PlaceStartEndPoints();

    // 4. Seviyeye göre anahtarları yerleştir
    PlaceKeysForLevel(level);

    // 5. Seviyeye göre ekstra item'ları yerleştir
    PlaceItemsForLevel(level);
}
/**
 * @brief Labirent verilerini temizler ve yeniden oluşturulmaya hazırlar.
 */
void MazeGenerator::ClearMaze() {
    int mazeWidth = width * 2 + 1;
    int mazeHeight = height * 2 + 1;
    maze.assign(mazeHeight, std::vector<int>(mazeWidth, (int)TileType::WALL));
    isRoomCell.assign(mazeHeight, std::vector<bool>(mazeWidth, false));
    startPosition = { -1, -1 };
    endPosition = { -1, -1 };
}

/**
 * @brief Başlangıç (4,4) ve Bitiş (sağ-alt köşe) noktalarını ayarlar.
 */
void MazeGenerator::PlaceStartEndPoints() {
    // Başlangıç Noktası (4,4)
    if (isValid(4, 4) && maze[4][4] == (int)TileType::WALL) {
        // Eğer (4,4) bir duvar ise, etrafındaki en yakın boşluğu bul
        // Basitlik adına, şimdilik (4,4)'ü direkt yol yapıyoruz.
        // Daha gelişmiş bir mantık gerekebilir.
        maze[3][4] = (int)TileType::PATH; // Etrafını aç
        maze[4][3] = (int)TileType::PATH;
        maze[5][4] = (int)TileType::PATH;
        maze[4][5] = (int)TileType::PATH;
    }
    maze[4][4] = (int)TileType::START_POINT;
    startPosition = { 4, 4 };

    // Bitiş Noktası (sağ-alt köşe)
    int endX = width * 2 - 1;
    int endY = height * 2 - 1;
    if (isValid(endX, endY)) {
        maze[endY][endX] = (int)TileType::END_POINT;
        // Bitiş noktasına erişim olduğundan emin olmak için etrafını aç
        if (isValid(endX - 1, endY)) maze[endY][endX - 1] = (int)TileType::PATH;
        if (isValid(endX, endY - 1)) maze[endY - 1][endX] = (int)TileType::PATH;
        endPosition = { endX, endY };
    }
}

/**
 * @brief Belirtilen alanda rastgele boş bir hücre (TileType::PATH) bulur.
 * @param area Arama yapılacak alan (Room struct).
 * @return Boş bir hücrenin koordinatları. Bulunamazsa {-1, -1}.
 */
std::pair<int, int> MazeGenerator::FindRandomEmptyCellInArea(const Room& area) {
    std::vector<std::pair<int, int>> emptyCells;
    for (int y = area.y; y < area.y + area.h; ++y) {
        for (int x = area.x; x < area.x + area.w; ++x) {
            if (isValid(x, y) && maze[y][x] == (int)TileType::PATH) {
                emptyCells.push_back({ x, y });
            }
        }
    }

    if (emptyCells.empty()) {
        return { -1, -1 }; // Boş hücre bulunamadı
    }

    std::uniform_int_distribution<int> dist(0, emptyCells.size() - 1);
    return emptyCells[dist(rng)];
}

/**
 * @brief Seviyeye göre anahtarları labirentin çeyreklerine yerleştirir.
 */
void MazeGenerator::PlaceKeysForLevel(int level) {
    int mazeW = width * 2 + 1;
    int mazeH = height * 2 + 1;
    int halfW = mazeW / 2;
    int halfH = mazeH / 2;

    // Çeyrekleri tanımla
    Room bottomLeft(1, halfH, halfW - 1, halfH - 1);
    Room topRight(halfW, 1, halfW - 1, halfH - 1);
    Room bottomRight(halfW, halfH, halfW - 1, halfH - 1);
    Room center(halfW / 2, halfH / 2, halfW, halfH);

    std::pair<int, int> pos;

    // Seviye 1: Sol-Alt
    if (level >= 1) {
        pos = FindRandomEmptyCellInArea(bottomLeft);
        if (pos.first != -1) maze[pos.second][pos.first] = (int)TileType::KEY;
    }
    // Seviye 2: Sağ-Üst
    if (level >= 2) {
        pos = FindRandomEmptyCellInArea(topRight);
        if (pos.first != -1) maze[pos.second][pos.first] = (int)TileType::KEY;
    }
    // Seviye 3: Sağ-Alt
    if (level >= 3) {
        pos = FindRandomEmptyCellInArea(bottomRight);
        if (pos.first != -1) maze[pos.second][pos.first] = (int)TileType::KEY;
    }
    // Seviye 4: Orta
    if (level >= 4) {
        pos = FindRandomEmptyCellInArea(center);
        if (pos.first != -1) maze[pos.second][pos.first] = (int)TileType::KEY;
    }
}

/**
 * @brief Seviyeye göre item'ları (can, zırh vb.) yerleştirir.
 */
void MazeGenerator::PlaceItemsForLevel(int level) {
    int mazeW = width * 2 + 1;
    int mazeH = height * 2 + 1;
    int halfW = mazeW / 2;
    int halfH = mazeH / 2;

    // İzin verilen alanlar (sol-üst hariç hepsi)
    std::vector<Room> allowedAreas;
    allowedAreas.push_back(Room(halfW, 1, halfW - 1, halfH - 1));     // Top-Right
    allowedAreas.push_back(Room(1, halfH, halfW - 1, halfH - 1));     // Bottom-Left
    allowedAreas.push_back(Room(halfW, halfH, halfW - 1, halfH - 1)); // Bottom-Right

    TileType items[] = {
        TileType::HEALTH_PACK,
        TileType::ARMOR_PACK,
        TileType::SECOND_WEAPON,
        TileType::WEAPON_AMMO,
        TileType::EXTRA_SCORE
    };

    // Her level için her item'dan +1 tane ekle
    for (int i = 0; i < level; ++i) {
        for (const auto& itemType : items) {
            // Rastgele bir izin verilen alan seç
            std::uniform_int_distribution<int> areaDist(0, allowedAreas.size() - 1);
            const Room& randomArea = allowedAreas[areaDist(rng)];

            // O alanda boş bir yer bul ve item'ı yerleştir
            std::pair<int, int> pos = FindRandomEmptyCellInArea(randomArea);
            if (pos.first != -1) {
                maze[pos.second][pos.first] = (int)itemType;
            }
        }
    }
}