#include "MazeGenerator.h"
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
        std::uniform_int_distribution<int> widthDist(3, 3);
        std::uniform_int_distribution<int> heightDist(3, 3);
        int roomW = widthDist(rng);
        int roomH = heightDist(rng);
        std::uniform_int_distribution<int> xDist(1, width - roomW - 1);
        std::uniform_int_distribution<int> yDist(1, height - roomH - 1);
        int roomX = 1 + 2 * xDist(rng);
        int roomY = 1 + 2 * yDist(rng);

        bool overlap = false;
        for (const auto& r : rooms) {

            if (roomX < r.x + r.w * 2 + 1 && roomX + roomW * 2 + 1 > r.x &&
                roomY < r.y + r.h * 2 + 1 && roomY + roomH * 2 + 1 > r.y) {
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
    int tileValue = maze[y][x];
    return tileValue == static_cast<int>(TileType::WALL) || tileValue == static_cast<int>(TileType::SPECIAL_WALL);
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
    m_keyWallPositions.clear();
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
    int numKeys = std::min(level, 4);
    if (numKeys == 0) return;

    int numSpecialWalls = 1;
    int mazeW = width * 2 + 1;
    int mazeH = height * 2 + 1;

    // 1. Değiştirilebilecek tüm normal duvarları bul (kenarda olmayanlar)
    std::vector<std::pair<int, int>> replaceableWalls;
    for (int y = 1; y < mazeH - 1; ++y) {
        for (int x = 1; x < mazeW - 1; ++x) {
            if (maze[y][x] == (int)TileType::WALL) {
                // Sadece etrafında yol olan duvarları seçelim ki kırılabilir olsunlar
                int pathNeighbors = 0;
                if (isValid(x, y - 1) && maze[y - 1][x] != (int)TileType::WALL) pathNeighbors++;
                if (isValid(x, y + 1) && maze[y + 1][x] != (int)TileType::WALL) pathNeighbors++;
                if (isValid(x - 1, y) && maze[y][x - 1] != (int)TileType::WALL) pathNeighbors++;
                if (isValid(x + 1, y) && maze[y][x + 1] != (int)TileType::WALL) pathNeighbors++;

                if (pathNeighbors > 0) {
                    replaceableWalls.push_back({ x, y });
                }
            }
        }
    }

    // 2. Rastgele pozisyonlar elde etmek için duvar listesini karıştır
    std::shuffle(replaceableWalls.begin(), replaceableWalls.end(), rng);

    // 3. Özel duvarları yerleştir
    int wallsToPlace = std::min((int)replaceableWalls.size(), numSpecialWalls);
    std::vector<std::pair<int, int>> placedSpecialWalls;

    for (int i = 0; i < wallsToPlace; ++i) {
        std::pair<int, int> pos = replaceableWalls[i];
        maze[pos.second][pos.first] = (int)TileType::SPECIAL_WALL;
        placedSpecialWalls.push_back(pos);
    }

    // 4. Yerleştirilen özel duvarlardan bazılarını anahtar konumu olarak belirle
    int keysToDesignate = std::min(numKeys, (int)placedSpecialWalls.size());
    for (int i = 0; i < keysToDesignate; ++i) {
        m_keyWallPositions.push_back(placedSpecialWalls[i]);
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

    // DEĞİŞTİRİLDİ: Item listesine yeni silah eklendi
    TileType items[] = {
        TileType::HEALTH_PACK,
        TileType::ARMOR_PACK,
        TileType::WEAPON_MELTER, // Yeni silah
        TileType::WEAPON_AMMO,   // Bu artık Melter mermisi verebilir (Game.cpp'de ayarlanacak)
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

void MazeGenerator::RemoveKeyWallPosition(int x, int y)
{
    // Verilen koordinatla eşleşen öğeyi vektörden kaldır
    auto it = std::remove_if(m_keyWallPositions.begin(), m_keyWallPositions.end(),
        [x, y](const std::pair<int, int>& pos) {
            return pos.first == x && pos.second == y;
        });

    if (it != m_keyWallPositions.end()) {
        m_keyWallPositions.erase(it, m_keyWallPositions.end());
    }
}