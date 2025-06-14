#include "Level.h"
#include <queue>
#include <random>
#include <ctime>

Level::Level(int level) : currentLevel(level), score(0) {
    timeLimit = 5 * 60 * 1000; // 5 minutes in milliseconds
    waveInterval = 30 * 1000;  // 30 seconds in milliseconds
    enemiesPerWave = 6 + level;
    startTime = GetTickCount();
    lastWaveTime = startTime;
    // Initialize maze array
    maze = new int* [MAZE_SIZE];
    for (int i = 0; i < MAZE_SIZE; i++) {
        maze[i] = new int[MAZE_SIZE];
    }

    SetupLevel();
}

Level::~Level() {
    for (int i = 0; i < MAZE_SIZE; i++) {
        delete[] maze[i];
    }
    delete[] maze;
}

void Level::SetupLevel() {
    // 1. Generate maze
    GenerateMaze();

    // 2. Set start and end positions
    startPos = { 1, 1 };
    endPos = { MAZE_SIZE - 2, MAZE_SIZE - 2 };
    ClearArea(startPos.x, startPos.y, 3);
    ClearArea(endPos.x, endPos.y, 3);

    // 3. Place keys based on level
    PlaceKeys();

    // 4. Place items
    PlaceItems();

    // 5. Validate path to end
    if (!ValidatePathToEnd()) {
        // If no valid path exists, regenerate the level
        SetupLevel();
    }

    // 6. Place static guardian enemies near keys
    //SpawnStaticGuardians();
}

void Level::GenerateMaze() {
    InitializeMaze();
    RecursiveBacktracking(1, 1);
}

void Level::PlaceKeys() {
    keys.clear();
    std::random_device rd;
    std::mt19937 gen(rd());

    // Place keys based on level requirements
    switch (currentLevel) {
    case 1: // Bottom-left only
        PlaceKeyInQuadrant(Quadrant::BOTTOM_LEFT);
        break;
    case 2: // Bottom-left + Top-right
        PlaceKeyInQuadrant(Quadrant::BOTTOM_LEFT);
        PlaceKeyInQuadrant(Quadrant::TOP_RIGHT);
        break;
    case 3: // Bottom-left + Top-right + Bottom-right
        PlaceKeyInQuadrant(Quadrant::BOTTOM_LEFT);
        PlaceKeyInQuadrant(Quadrant::TOP_RIGHT);
        PlaceKeyInQuadrant(Quadrant::BOTTOM_RIGHT);
        break;
    case 4: // All quadrants + center
        PlaceKeyInQuadrant(Quadrant::BOTTOM_LEFT);
        PlaceKeyInQuadrant(Quadrant::TOP_RIGHT);
        PlaceKeyInQuadrant(Quadrant::BOTTOM_RIGHT);
        PlaceKeyInQuadrant(Quadrant::TOP_LEFT);
        PlaceKeyInQuadrant(Quadrant::CENTER);
        break;
    }
}

void Level::PlaceItems() {
    int baseItems = 2; // Base number of items
    int totalItems = baseItems + currentLevel;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, MAZE_SIZE - 1);

    for (int i = 0; i < totalItems; i++) {
        ItemType itemType = static_cast<ItemType>(i % 5); // Cycle through item types
        POINT pos;
        do {
            pos = { dis(gen), dis(gen) };
        } while (maze[pos.y][pos.x] != 0 ||
            GetQuadrant(pos) == Quadrant::TOP_LEFT ||
            IsInClearArea(pos.x, pos.y));

        items.push_back({ pos, itemType });
        ClearArea(pos.x, pos.y, 3);
    }
}

void Level::SpawnEnemyWave(int currentTime) {
    if (currentTime - lastWaveTime >= waveInterval) {
        // Spawn enemies around player in one of four directions
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 3); // 0-3 for four directions

        for (int i = 0; i < enemiesPerWave; i++) {
            int direction = dis(gen);
            POINT spawnPos;
            // TODO: Get player position from game state
            POINT playerPos = { MAZE_SIZE / 2, MAZE_SIZE / 2 }; // Example position

            // Calculate spawn position 6 tiles away from player
            switch (direction) {
            case 0: // North
                spawnPos = { playerPos.x, playerPos.y - 6 };
                break;
            case 1: // East
                spawnPos = { playerPos.x + 6, playerPos.y };
                break;
            case 2: // South
                spawnPos = { playerPos.x, playerPos.y + 6 };
                break;
            case 3: // West
                spawnPos = { playerPos.x - 6, playerPos.y };
                break;
            }

            // Ensure spawn position is valid
            if (spawnPos.x >= 0 && spawnPos.x < MAZE_SIZE &&
                spawnPos.y >= 0 && spawnPos.y < MAZE_SIZE &&
                maze[spawnPos.y][spawnPos.x] == 0) {
                enemies.push_back(spawnPos);
            }
        }

        lastWaveTime = currentTime;
    }
}

int Level::CalculateFinalScore(int timeUsed, int remainingHp, int keysCollected,
    int enemiesKilled, int itemsCollected) {
    int score = 0;

    // Base scoring
    score += 1000 * currentLevel;                    // Level completion
    score += (300 - timeUsed / 1000) * 10;            // Time bonus (in seconds)
    score += keysCollected * 500;                    // Key collection
    score += enemiesKilled * 100;                    // Enemy kills
    score += itemsCollected * 200;                   // Item collection
    score += remainingHp * 5;                        // Survival bonus

    return score;
}

bool Level::IsTimeExpired(int currentTime) {
    return (currentTime - startTime) >= timeLimit;
}

int Level::GetTimeRemaining(int currentTime) {
    return timeLimit - (currentTime - startTime);
}

void Level::ClearArea(int x, int y, int size) {
    int radius = size / 2;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            int newX = x + i;
            int newY = y + j;
            if (newX >= 0 && newX < MAZE_SIZE &&
                newY >= 0 && newY < MAZE_SIZE) {
                maze[newY][newX] = 0; // Clear the area
            }
        }
    }
}

// Private helper methods implementation...
void Level::InitializeMaze() {
    // Initialize all cells as walls
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            maze[i][j] = 1; // 1 represents wall
        }
    }
}

bool Level::ValidatePathToEnd() {
    return HasValidPath(startPos, endPos);
}

bool Level::HasValidPath(POINT start, POINT end) {
    // Implementation using BFS
    std::vector<std::vector<bool>> visited(MAZE_SIZE, std::vector<bool>(MAZE_SIZE, false));
    std::queue<POINT> q;

    q.push(start);
    visited[start.y][start.x] = true;

    while (!q.empty()) {
        POINT current = q.front();
        q.pop();

        if (current.x == end.x && current.y == end.y)
            return true;

        // Check all four directions
        POINT dirs[4] = { {0,1}, {1,0}, {0,-1}, {-1,0} };
        for (auto& dir : dirs) {
            POINT next = { current.x + dir.x, current.y + dir.y };
            if (next.x >= 0 && next.x < MAZE_SIZE &&
                next.y >= 0 && next.y < MAZE_SIZE &&
                !visited[next.y][next.x] && maze[next.y][next.x] == 0) {
                visited[next.y][next.x] = true;
                q.push(next);
            }
        }
    }

    return false;
}
void Level::RecursiveBacktracking(int x, int y) {
    // Direction arrays for: Right, Down, Left, Up
    const int dx[] = { 2, 0, -2, 0 };
    const int dy[] = { 0, 2, 0, -2 };

    // Mark current cell as path
    maze[y][x] = 0;

    // Create a random order of directions
    int dirs[4] = { 0, 1, 2, 3 };
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(dirs, dirs + 4, gen);

    // Try each direction in random order
    for (int i = 0; i < 4; i++) {
        int dir = dirs[i];
        int newX = x + dx[dir];
        int newY = y + dy[dir];

        // Check if the new position is within bounds and unvisited
        if (newX > 0 && newX < MAZE_SIZE - 1 &&
            newY > 0 && newY < MAZE_SIZE - 1 &&
            maze[newY][newX] == 1) {
            // Mark the wall between cells as path
            maze[y + dy[dir] / 2][x + dx[dir] / 2] = 0;
            // Recursively visit next cell
            RecursiveBacktracking(newX, newY);
        }
    }
}

bool Level::PlaceKeyInQuadrant(Quadrant quadrant) {
    int quadrantStartX, quadrantStartY, quadrantEndX, quadrantEndY;
    int mid = Level::MAZE_SIZE / 2;

    // Define quadrant boundaries
    switch (quadrant) {
    case Quadrant::TOP_RIGHT:
        quadrantStartX = mid;
        quadrantStartY = 1;
        quadrantEndX = MAZE_SIZE - 2;
        quadrantEndY = mid;
        break;
    case Quadrant::BOTTOM_RIGHT:
        quadrantStartX = mid;
        quadrantStartY = mid;
        quadrantEndX = MAZE_SIZE - 2;
        quadrantEndY = MAZE_SIZE - 2;
        break;
    case Quadrant::BOTTOM_LEFT:
        quadrantStartX = 1;
        quadrantStartY = mid;
        quadrantEndX = mid;
        quadrantEndY = MAZE_SIZE - 2;
        break;
    case Quadrant::TOP_LEFT:
        quadrantStartX = 1;
        quadrantStartY = 1;
        quadrantEndX = mid;
        quadrantEndY = mid;
        break;
    case Quadrant::CENTER:
        quadrantStartX = mid - mid / 2;
        quadrantStartY = mid - mid / 2;
        quadrantEndX = mid + mid / 2;
        quadrantEndY = mid + mid / 2;
        break;
    default:
        return false;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    // Create distributions for x and y coordinates within the quadrant
    std::uniform_int_distribution<> disX(quadrantStartX, quadrantEndX);
    std::uniform_int_distribution<> disY(quadrantStartY, quadrantEndY);

    const int MAX_ATTEMPTS = 50;
    int attempts = 0;

    while (attempts < MAX_ATTEMPTS) {
        // Generate random position within quadrant
        POINT keyPos = { disX(gen), disY(gen) };

        // Check if position is valid (is a path and not near other keys)
        if (maze[keyPos.y][keyPos.x] == 0 && !IsInClearArea(keyPos.x, keyPos.y)) {
            bool validDistance = true;

            // Check minimum distance from other keys
            for (const POINT& existingKey : keys) {
                int dx = keyPos.x - existingKey.x;
                int dy = keyPos.y - existingKey.y;
                int distanceSquared = dx * dx + dy * dy;

                if (distanceSquared < 25) { // Minimum distance of 5 units
                    validDistance = false;
                    break;
                }
            }

            if (validDistance) {
                // Place key and clear surrounding area
                keys.push_back(keyPos);
                ClearArea(keyPos.x, keyPos.y, 3);
                return true;
            }
        }

        attempts++;
    }

    return false;
}

bool Level::IsInClearArea(int x, int y) const {
    // Check if position is in start or end area
    if ((x <= 3 && y <= 3) ||  // Start area
        (x >= MAZE_SIZE - 4 && y >= MAZE_SIZE - 4)) {  // End area
        return true;
    }

    // Check if position is near any existing key or item
    for (const POINT& key : keys) {
        int dx = x - key.x;
        int dy = y - key.y;
        if (dx * dx + dy * dy < 9) { // Within 3 units of a key
            return true;
        }
    }

    for (const auto& item : items) {
        int dx = x - item.first.x;
        int dy = y - item.first.y;
        if (dx * dx + dy * dy < 9) { // Within 3 units of an item
            return true;
        }
    }

    return false;
}
Quadrant Level::GetQuadrant(POINT p) const {
    int mid = MAZE_SIZE / 2;

    // Check center first (if point is in the middle region)
    int centerStart = mid - mid / 2;
    int centerEnd = mid + mid / 2;
    if (p.x >= centerStart && p.x <= centerEnd &&
        p.y >= centerStart && p.y <= centerEnd) {
        return Quadrant::CENTER;
    }

    // Check other quadrants
    if (p.x >= mid) {
        if (p.y < mid) {
            return Quadrant::TOP_RIGHT;
        }
        else {
            return Quadrant::BOTTOM_RIGHT;
        }
    }
    else {
        if (p.y < mid) {
            return Quadrant::TOP_LEFT;
        }
        else {
            return Quadrant::BOTTOM_LEFT;
        }
    }
}

