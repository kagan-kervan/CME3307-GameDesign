#include "Enemy.h"
#include "Game.h"       // _pEnemyMissileBitmap ve game_engine için
#include "GameEngine.h" // game_engine için
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <algorithm> // std::min, std::max için

// A* Pathfinding (Bu kýsým ayný kalýyor, ancak sýnýr kontrolleri eklenebilir)
namespace Pathfinder {
    struct Node { int x, y, gCost, hCost; Node* parent; Node(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {} int fCost() const { return gCost + hCost; } };
    struct CompareNode { bool operator()(const Node* a, const Node* b) const { return a->fCost() > b->fCost(); } };
    int calculateHCost(int x1, int y1, int x2, int y2) { return abs(x1 - x2) + abs(y1 - y2); }
    std::vector<POINT> reconstructPath(Node* endNode) { std::vector<POINT> p; Node* c = endNode; while (c != nullptr) { p.push_back({ c->x, c->y }); c = c->parent; } std::reverse(p.begin(), p.end()); return p; }
    std::vector<POINT> FindPath(MazeGenerator* maze, POINT start, POINT end) {
        if (!maze || maze->GetMaze().empty() || maze->GetMaze()[0].empty()) return {};
        int mazeWidth = maze->GetMaze()[0].size();
        int mazeHeight = maze->GetMaze().size();

        std::priority_queue<Node*, std::vector<Node*>, CompareNode> o; std::map<std::pair<int, int>, Node*> a;
        Node* s = new Node(start.x, start.y); s->hCost = calculateHCost(start.x, start.y, end.x, end.y); o.push(s); a[{start.x, start.y}] = s;
        int dx[] = { 0, 0, 1, -1 }, dy[] = { 1, -1, 0, 0 };
        while (!o.empty()) {
            Node* c = o.top(); o.pop();
            if (c->x == end.x && c->y == end.y) { std::vector<POINT> p = reconstructPath(c); for (auto const& pair : a) { auto key = pair.first; auto val = pair.second; delete val; } a.clear(); return p; }
            for (int i = 0; i < 4; ++i) {
                int nX = c->x + dx[i], nY = c->y + dy[i];
                // Sýnýr ve duvar kontrolü
                if (nX < 0 || nX >= mazeWidth || nY < 0 || nY >= mazeHeight || maze->IsWall(nX, nY)) continue;
                int nG = c->gCost + 1; auto it = a.find({ nX, nY });
                if (it == a.end() || nG < it->second->gCost) {
                    Node* nN; if (it == a.end()) { nN = new Node(nX, nY); a[{nX, nY}] = nN; }
                    else { nN = it->second; }
                    nN->parent = c; nN->gCost = nG; nN->hCost = calculateHCost(nX, nY, end.x, end.y); o.push(nN);
                }
            }
        }
        for (auto const& pair : a) { auto key = pair.first; auto val = pair.second; delete val; } a.clear(); return {};
    }
}


Enemy::Enemy(Bitmap* pBitmap, RECT& rcBounds, BOUNDSACTION baBoundsAction,
    MazeGenerator* pMaze, Sprite* pPlayer, EnemyType type)
    : Sprite(pBitmap, rcBounds, baBoundsAction, SPRITE_TYPE_ENEMY), m_pMaze(pMaze), m_pPlayer(pPlayer), m_type(type)
{
    m_state = AIState::IDLE;
    m_pathIndex = 0;
    m_attackCooldown = 0;
    m_pathfindingCooldown = 0;

    Sprite::SetNumFrames(4);
    Sprite::SetFrameDelay(8);
}

// YENÝ YARDIMCI FONKSÝYON: Duvar çarpýþmalarýný çözer ve gerekirse hýzý ayarlar.
// Bu fonksiyon, Sprite'ýn pozisyonunu doðrudan deðiþtirebilir ve verilen hýzý (referans) güncelleyebilir.
void Enemy::ResolveWallCollisions(POINT& desiredVelocity)
{
    if (!m_pMaze || TILE_SIZE == 0) return;

    RECT currentSpritePos = GetPosition();
    int spriteWidth = GetWidth();
    int spriteHeight = GetHeight();

    // 1. X ekseninde hareket ve çarpýþma kontrolü
    if (desiredVelocity.x != 0) {
        RECT nextXPos = currentSpritePos;
        nextXPos.left += desiredVelocity.x;
        nextXPos.right += desiredVelocity.x;

        // X ekseninde hareket ettiðinde hangi tile'lara deðeceðini kontrol et
        int testTileYTop = nextXPos.top / TILE_SIZE;
        int testTileYBottom = (nextXPos.bottom - 1) / TILE_SIZE; // -1 kenar durumlarý için
        int testTileYMid = (nextXPos.top + spriteHeight / 2) / TILE_SIZE;

        if (desiredVelocity.x > 0) { // Saða hareket
            int testTileX = (nextXPos.right - 1) / TILE_SIZE;
            if (m_pMaze->IsWall(testTileX, testTileYTop) ||
                m_pMaze->IsWall(testTileX, testTileYBottom) ||
                m_pMaze->IsWall(testTileX, testTileYMid)) {
                // Duvara çarptý, pozisyonu duvarýn soluna ayarla ve X hýzýný sýfýrla
                SetPosition((testTileX * TILE_SIZE) - spriteWidth, currentSpritePos.top);
                desiredVelocity.x = 0;
            }
        }
        else { // Sola hareket
            int testTileX = nextXPos.left / TILE_SIZE;
            if (m_pMaze->IsWall(testTileX, testTileYTop) ||
                m_pMaze->IsWall(testTileX, testTileYBottom) ||
                m_pMaze->IsWall(testTileX, testTileYMid)) {
                // Duvara çarptý, pozisyonu duvarýn saðýna ayarla ve X hýzýný sýfýrla
                SetPosition((testTileX + 1) * TILE_SIZE, currentSpritePos.top);
                desiredVelocity.x = 0;
            }
        }
    }

    // X ekseni düzeltmesinden sonra mevcut pozisyonu tekrar al
    currentSpritePos = GetPosition();

    // 2. Y ekseninde hareket ve çarpýþma kontrolü
    if (desiredVelocity.y != 0) {
        RECT nextYPos = currentSpritePos; // X düzeltilmiþ pozisyondan baþla
        nextYPos.top += desiredVelocity.y;
        nextYPos.bottom += desiredVelocity.y;

        int testTileXLeft = nextYPos.left / TILE_SIZE;
        int testTileXRight = (nextYPos.right - 1) / TILE_SIZE;
        int testTileXMid = (nextYPos.left + spriteWidth / 2) / TILE_SIZE;

        if (desiredVelocity.y > 0) { // Aþaðý hareket
            int testTileY = (nextYPos.bottom - 1) / TILE_SIZE;
            if (m_pMaze->IsWall(testTileXLeft, testTileY) ||
                m_pMaze->IsWall(testTileXRight, testTileY) ||
                m_pMaze->IsWall(testTileXMid, testTileY)) {
                SetPosition(currentSpritePos.left, (testTileY * TILE_SIZE) - spriteHeight);
                desiredVelocity.y = 0;
            }
        }
        else { // Yukarý hareket
            int testTileY = nextYPos.top / TILE_SIZE;
            if (m_pMaze->IsWall(testTileXLeft, testTileY) ||
                m_pMaze->IsWall(testTileXRight, testTileY) ||
                m_pMaze->IsWall(testTileXMid, testTileY)) {
                SetPosition(currentSpritePos.left, (testTileY + 1) * TILE_SIZE);
                desiredVelocity.y = 0;
            }
        }
    }
}


SPRITEACTION Enemy::Update()
{
    UpdateAI(); // Bu, Sprite::m_ptVelocity'yi ayarlar.

    POINT currentVelocity = GetVelocity(); // AI tarafýndan belirlenen istenen hýz.

    // Duvar çarpýþmalarýný çöz. Bu fonksiyon, eðer çarpýþma varsa
    // Sprite'ýn pozisyonunu düzeltebilir ve currentVelocity'yi (referansla) deðiþtirebilir.
    ResolveWallCollisions(currentVelocity);

    // Düzeltilmiþ veya orijinal hýz ile Sprite'ýn kendi SetVelocity'sini çaðýr.
    SetVelocity(currentVelocity.x, currentVelocity.y);

    // Þimdi Sprite'ýn temel Update'ini çaðýr. Bu, ayarlanmýþ hýzý kullanarak
    // konumu güncelleyecek, animasyonu ilerletecek ve BA_ (BoundsAction) sýnýr kontrollerini yapacak.
    // ResolveWallCollisions zaten pozisyonu duvara yasladýðý için,
    // Sprite::Update'ýn BA_STOP davranýþý büyük ihtimalle ekstra bir þey yapmayacak
    // (veya çok küçük bir düzeltme yapacak).
    SPRITEACTION action = Sprite::Update();


    // Sprite::Update'dan sonra son bir güvenlik kontrolü ve pozisyon düzeltmesi.
    // Bu, BA_BOUNCE gibi durumlarýn veya ResolveWallCollisions'ýn
    // mükemmel olmadýðý nadir durumlarýn ele alýnmasýna yardýmcý olabilir.
    // Genellikle ResolveWallCollisions yeterli olmalý.
    // Bu kýsým isteðe baðlýdýr ve ince ayar gerektirebilir.
    // Þimdilik daha basit tutmak için bu adýmý atlayabiliriz, ResolveWallCollisions'ýn
    // ana iþi yapmasýný bekleyebiliriz. Eðer hala sorunlar varsa bu eklenebilir.
    /*
    RECT finalPos = GetPosition();
    int spriteWidth = GetWidth();
    int spriteHeight = GetHeight();

    // X ekseni son kontrolü
    if (m_pMaze->IsRectCollidingWithWall(finalPos, spriteWidth, spriteHeight)) { // Böyle bir fonksiyon yazmak gerekebilir
        // Pozisyonu bir önceki geçerli pozisyona geri al veya duvara yasla
        // ... karmaþýklaþabilir ...
    }
    */

    return action;
}

void Enemy::UpdateAI()
{
    if (m_attackCooldown > 0) m_attackCooldown--;
    if (m_pathfindingCooldown > 0) m_pathfindingCooldown--;

    if (!m_pPlayer || !game_engine) {
        SetVelocity(0, 0);
        return;
    }
    if (TILE_SIZE == 0) { // TILE_SIZE tanýmlý olmalý
        SetVelocity(0, 0);
        return;
    }

    float playerDistance = sqrt(pow(static_cast<float>(m_pPlayer->GetPosition().left - m_rcPosition.left), 2.0f) +
        pow(static_cast<float>(m_pPlayer->GetPosition().top - m_rcPosition.top), 2.0f));

    if (playerDistance > TILE_SIZE * 25) { // Görüþ mesafesi
        m_state = AIState::IDLE;
        SetVelocity(0, 0);
        return;
    }

    bool hasLOS = HasLineOfSightToPlayer();

    if (hasLOS) {
        m_path.clear();
        m_pathIndex = 0;
        if (m_type == EnemyType::CHASER) {
            m_state = AIState::CHASING;
        }
        else { // TURRET
            m_state = AIState::ATTACKING;
        }
    }
    else { // Görüþ hattý yoksa
        m_state = AIState::CHASING; // Her zaman yol bulmaya çalýþ
    }

    switch (m_state)
    {
    case AIState::IDLE:
        SetVelocity(0, 0);
        break;

    case AIState::CHASING:
        if (!m_path.empty() && m_pathIndex < m_path.size()) {
            FollowPath();
        }
        else { // Yol yok veya bittiyse
            float dirX = static_cast<float>(m_pPlayer->GetPosition().left - m_rcPosition.left);
            float dirY = static_cast<float>(m_pPlayer->GetPosition().top - m_rcPosition.top);
            float len = sqrt(dirX * dirX + dirY * dirY);
            if (len > 0) { dirX /= len; dirY /= len; }
            SetVelocity(static_cast<int>(dirX * 8), static_cast<int>(dirY * 8)); // Hýz arttýrýldý

            if (m_pathfindingCooldown <= 0) {
                FindPath();
                m_pathfindingCooldown = 30; // Cooldown azaltýldý
            }
        }
        break;

    case AIState::ATTACKING:
        SetVelocity(0, 0); // Saldýrýrken sabit dur
        if (m_attackCooldown <= 0) {
            AttackPlayer();
            m_attackCooldown = (m_type == EnemyType::TURRET) ? 35 : 50; // Ateþ hýzý arttýrýldý
        }
        break;
    }
}

bool Enemy::FindPath()
{
    if (!m_pPlayer || !m_pMaze || TILE_SIZE == 0 || m_pMaze->GetMaze().empty() || m_pMaze->GetMaze()[0].empty()) return false;

    // Düþmanýn ve oyuncunun merkez tile'larýný kullanmak daha iyi sonuç verebilir
    POINT startTile = { (m_rcPosition.left + GetWidth() / 2) / TILE_SIZE, (m_rcPosition.top + GetHeight() / 2) / TILE_SIZE };
    POINT endTile = { (m_pPlayer->GetPosition().left + m_pPlayer->GetWidth() / 2) / TILE_SIZE, (m_pPlayer->GetPosition().top + m_pPlayer->GetHeight() / 2) / TILE_SIZE };

    int mazeWidth = m_pMaze->GetMaze()[0].size();
    int mazeHeight = m_pMaze->GetMaze().size();

    // Baþlangýç ve bitiþ tile'larýnýn sýnýrlar içinde olduðundan emin ol
    if (startTile.x < 0 || startTile.x >= mazeWidth || startTile.y < 0 || startTile.y >= mazeHeight ||
        endTile.x < 0 || endTile.x >= mazeWidth || endTile.y < 0 || endTile.y >= mazeHeight) {
        return false;
    }
    // Baþlangýç tile'ý duvar olmamalý
    if (m_pMaze->IsWall(startTile.x, startTile.y)) return false;


    if (m_pMaze->IsWall(endTile.x, endTile.y)) {
        int dx[] = { 0, 0, 1, -1, 1, 1, -1, -1 };
        int dy[] = { 1, -1, 0, 0, 1, -1, 1, -1 };
        bool foundAlternative = false;
        for (int i = 0; i < 8; ++i) {
            int altX = endTile.x + dx[i];
            int altY = endTile.y + dy[i];
            if (altX >= 0 && altX < mazeWidth && altY >= 0 && altY < mazeHeight && !m_pMaze->IsWall(altX, altY)) {
                endTile = { altX, altY };
                foundAlternative = true;
                break;
            }
        }
        if (!foundAlternative) return false;
    }

    m_path = Pathfinder::FindPath(m_pMaze, startTile, endTile);
    m_pathIndex = 0;
    return !m_path.empty();
}

void Enemy::FollowPath()
{
    if (m_path.empty() || m_pathIndex >= m_path.size() || TILE_SIZE == 0) {
        SetVelocity(0, 0);
        m_path.clear();
        return;
    }

    POINT targetTile = m_path[m_pathIndex];
    float targetX = static_cast<float>(targetTile.x * TILE_SIZE) + (TILE_SIZE / 2.0f);
    float targetY = static_cast<float>(targetTile.y * TILE_SIZE) + (TILE_SIZE / 2.0f);

    float currentX = static_cast<float>(m_rcPosition.left + GetWidth() / 2.0f);
    float currentY = static_cast<float>(m_rcPosition.top + GetHeight() / 2.0f);

    float distanceToTargetTileCenter = sqrt(pow(targetX - currentX, 2.0f) + pow(targetY - currentY, 2.0f));

    if (distanceToTargetTileCenter < TILE_SIZE / 1.5f) { // Tolerans arttýrýldý
        m_pathIndex++;
        if (m_pathIndex >= m_path.size()) {
            m_path.clear();
            SetVelocity(0, 0);
            return;
        }
        targetTile = m_path[m_pathIndex]; // Yeni hedef tile
        targetX = static_cast<float>(targetTile.x * TILE_SIZE) + (TILE_SIZE / 2.0f);
        targetY = static_cast<float>(targetTile.y * TILE_SIZE) + (TILE_SIZE / 2.0f);
    }

    float dirX = targetX - currentX;
    float dirY = targetY - currentY;
    float len = sqrt(dirX * dirX + dirY * dirY);
    if (len > 0) { dirX /= len; dirY /= len; }

    SetVelocity(static_cast<int>(dirX * 10), static_cast<int>(dirY * 10)); // Hýz arttýrýldý
}

bool Enemy::HasLineOfSightToPlayer()
{
    if (!m_pPlayer || !m_pMaze || TILE_SIZE == 0 || m_pMaze->GetMaze().empty() || m_pMaze->GetMaze()[0].empty()) return false;

    // Düþmanýn ve oyuncunun merkez tile'larýný kullan
    int x0 = (m_rcPosition.left + GetWidth() / 2) / TILE_SIZE;
    int y0 = (m_rcPosition.top + GetHeight() / 2) / TILE_SIZE;
    int x1 = (m_pPlayer->GetPosition().left + m_pPlayer->GetWidth() / 2) / TILE_SIZE;
    int y1 = (m_pPlayer->GetPosition().top + m_pPlayer->GetHeight() / 2) / TILE_SIZE;

    int mazeWidth = m_pMaze->GetMaze()[0].size();
    int mazeHeight = m_pMaze->GetMaze().size();

    // Tile koordinatlarýnýn geçerli olduðundan emin ol
    if (x0 < 0 || x0 >= mazeWidth || y0 < 0 || y0 >= mazeHeight ||
        x1 < 0 || x1 >= mazeWidth || y1 < 0 || y1 >= mazeHeight) {
        return false; // Birisi sýnýr dýþýndaysa LOS yok
    }

    // Görüþ hattý mesafesini tile bazýnda sýnýrla
    if (abs(x0 - x1) > 15 || abs(y0 - y1) > 15) return false;

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy_abs = abs(y1 - y0); // dy'nin pozitif olmasý için _abs eklendi
    int dy = -dy_abs, sy = y0 < y1 ? 1 : -1; // Bresenham için dy negatif olmalý
    int err = dx + dy, e2;

    // Baþlangýç ve hedef tile'lar Bresenham'da duvar olarak deðerlendirilmemeli
    POINT startTile = { x0, y0 };
    POINT endTile = { x1, y1 };

    while (true) {
        // Mevcut tile (x0, y0) baþlangýç veya hedef tile deðilse duvar kontrolü yap
        if (!(x0 == startTile.x && y0 == startTile.y) && !(x0 == endTile.x && y0 == endTile.y)) {
            // Sýnýr kontrolü (genelde gereksiz çünkü döngü x1,y1'e ulaþýnca durur ama güvenlik için)
            if (x0 < 0 || x0 >= mazeWidth || y0 < 0 || y0 >= mazeHeight) return false;
            if (m_pMaze->IsWall(x0, y0)) return false;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return true;
}

void Enemy::AttackPlayer()
{
    if (!m_pPlayer || !game_engine || !_pEnemyMissileBitmap || TILE_SIZE == 0) return;

    float playerCenterX = static_cast<float>(m_pPlayer->GetPosition().left + m_pPlayer->GetWidth() / 2.0f);
    float playerCenterY = static_cast<float>(m_pPlayer->GetPosition().top + m_pPlayer->GetHeight() / 2.0f);
    float enemyCenterX = static_cast<float>(m_rcPosition.left + GetWidth() / 2.0f);
    float enemyCenterY = static_cast<float>(m_rcPosition.top + GetHeight() / 2.0f);

    float dirX = playerCenterX - enemyCenterX;
    float dirY = playerCenterY - enemyCenterY;
    float length = sqrt(dirX * dirX + dirY * dirY);
    if (length > 0) { dirX /= length; dirY /= length; }
    else { return; }

    RECT rcMissileBounds;
    if (m_pMaze && !m_pMaze->GetMaze().empty() && !m_pMaze->GetMaze()[0].empty()) {
        const auto& mazeData = m_pMaze->GetMaze();
        rcMissileBounds = { 0, 0,
                            static_cast<long>(mazeData[0].size() * TILE_SIZE),
                            static_cast<long>(mazeData.size() * TILE_SIZE) };
    }
    else {
        rcMissileBounds = { 0, 0, 4000, 4000 }; // Varsayýlan
    }

    Sprite* pMissile = new Sprite(_pEnemyMissileBitmap, rcMissileBounds, BA_DIE, SPRITE_TYPE_ENEMY_MISSILE);
    pMissile->SetPosition(static_cast<int>(enemyCenterX - pMissile->GetWidth() / 2.0f),
        static_cast<int>(enemyCenterY - pMissile->GetHeight() / 2.0f));

    int missileSpeed = 12; // Mermi hýzý arttýrýldý
    pMissile->SetVelocity(static_cast<int>(dirX * missileSpeed), static_cast<int>(dirY * missileSpeed));

    game_engine->AddSprite(pMissile);
}