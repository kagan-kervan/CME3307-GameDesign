#include "Enemy.h"
#include "Game.h"
#include "GameEngine.h"
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>

// A* Pathfinding (Bu kýsým doðru, deðiþtirilmedi)
namespace Pathfinder {
    // ... (Önceki doðru versiyondaki A* kodu burada yer alýyor)
    struct Node { int x, y, gCost, hCost; Node* parent; Node(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {} int fCost() const { return gCost + hCost; } };
    struct CompareNode { bool operator()(const Node* a, const Node* b) const { return a->fCost() > b->fCost(); } };
    int calculateHCost(int x1, int y1, int x2, int y2) { return abs(x1 - x2) + abs(y1 - y2); }
    std::vector<POINT> reconstructPath(Node* endNode) { std::vector<POINT> p; Node* c = endNode; while (c != nullptr) { p.push_back({ c->x, c->y }); c = c->parent; } std::reverse(p.begin(), p.end()); return p; }
    std::vector<POINT> FindPath(MazeGenerator* maze, POINT start, POINT end) {
        std::priority_queue<Node*, std::vector<Node*>, CompareNode> o; std::map<std::pair<int, int>, Node*> a;
        Node* s = new Node(start.x, start.y); s->hCost = calculateHCost(start.x, start.y, end.x, end.y); o.push(s); a[{start.x, start.y}] = s;
        int dx[] = { 0, 0, 1, -1 }, dy[] = { 1, -1, 0, 0 };
        while (!o.empty()) {
            Node* c = o.top(); o.pop();
            if (c->x == end.x && c->y == end.y) { std::vector<POINT> p = reconstructPath(c); for (auto i = a.begin(); i != a.end(); ++i) delete i->second; a.clear(); return p; }
            for (int i = 0; i < 4; ++i) {
                int nX = c->x + dx[i], nY = c->y + dy[i]; if (maze->IsWall(nX, nY)) continue;
                int nG = c->gCost + 1; auto it = a.find({ nX, nY });
                if (it == a.end() || nG < it->second->gCost) {
                    Node* nN; if (it == a.end()) { nN = new Node(nX, nY); a[{nX, nY}] = nN; }
                    else { nN = it->second; }
                    nN->parent = c; nN->gCost = nG; nN->hCost = calculateHCost(nX, nY, end.x, end.y); o.push(nN);
                }
            }
        }
        for (auto i = a.begin(); i != a.end(); ++i) delete i->second; a.clear(); return {};
    }
}

// HAREKET HATASI DÜZELTÝLDÝ: Kurucu metod artýk base class'ý doðru harita sýnýrlarýyla çaðýrýyor.
Enemy::Enemy(Bitmap* pBitmap, RECT& rcBounds, BOUNDSACTION baBoundsAction,
    MazeGenerator* pMaze, Sprite* pPlayer, EnemyType type)
    : Sprite(pBitmap, rcBounds, baBoundsAction), m_pMaze(pMaze), m_pPlayer(pPlayer), m_type(type)
{
    m_state = AIState::IDLE;
    m_pathIndex = 0;
    m_attackCooldown = 0;
    m_pathfindingCooldown = 0; // Sayaç sýfýrdan baþlýyor

    Sprite::SetNumFrames(4);
    Sprite::SetFrameDelay(8);
}

SPRITEACTION Enemy::Update()
{
    UpdateAI();
    return Sprite::Update(); // Hýzý ayarladýktan sonra hareket için base class'ý kullan
}

void Enemy::UpdateAI()
{
    if (m_attackCooldown > 0) m_attackCooldown--;
    if (m_pathfindingCooldown > 0) m_pathfindingCooldown--;
    if (!m_pPlayer) { SetVelocity(0, 0); return; }

    float playerDistance = sqrt(pow(m_pPlayer->GetPosition().left - m_rcPosition.left, 2) +
        pow(m_pPlayer->GetPosition().top - m_rcPosition.top, 2));

    // PERFORMANS OPTÝMÝZASYONU: Oyuncu çok uzaktaysa, AI'ý beklemeye al.
    if (playerDistance > TILE_SIZE * 20) {
        m_state = AIState::IDLE;
        SetVelocity(0, 0);
        return;
    }

    bool hasLOS = HasLineOfSightToPlayer();

    // Akýllý Strateji Belirleme
    if (hasLOS) {
        // PERFORMANS OPTÝMÝZASYONU: Görüþ varsa, pahalý A* yolunu temizle ve direkt saldýr.
        m_path.clear();
        if (m_type == EnemyType::CHASER) {
            m_state = AIState::CHASING;
        }
        else { // Turret
            m_state = AIState::ATTACKING;
        }
    }
    else {
        // Görüþ yoksa, yol bulmasý gerekiyor.
        m_state = AIState::CHASING;
    }

    // Stratejiyi Uygulama
    switch (m_state)
    {
    case AIState::IDLE:
        SetVelocity(0, 0);
        break;

    case AIState::CHASING:
        if (!m_path.empty()) {
            // Eðer takip edilecek bir yol varsa, onu takip et.
            FollowPath();
        }
        else {
            // PERFORMANS OPTÝMÝZASYONU: Yol yoksa ve görüþ de yoksa,
            // direkt oyuncuya doðru gitmeye çalýþ (köþelere takýlabilir).
            float dirX = (float)m_pPlayer->GetPosition().left - m_rcPosition.left;
            float dirY = (float)m_pPlayer->GetPosition().top - m_rcPosition.top;
            float len = sqrt(dirX * dirX + dirY * dirY);
            if (len > 0) { dirX /= len; dirY /= len; }
            SetVelocity((int)(dirX * 3), (int)(dirY * 3));

            // Ve periyodik olarak A* ile yeni bir yol bulmaya çalýþ.
            if (m_pathfindingCooldown <= 0) {
                FindPath();
                m_pathfindingCooldown = 45; // Her 1.5 saniyede bir yol ara
            }
        }
        break;

    case AIState::ATTACKING:
        SetVelocity(0, 0); // Ateþ etmek için dur
        if (m_attackCooldown <= 0) {
            AttackPlayer();
            m_attackCooldown = 60; // 2 saniyede bir ateþ et
        }
        break;
    }
}

bool Enemy::FindPath()
{
    if (!m_pPlayer || !m_pMaze) return false;
    POINT startTile = { GetPosition().left / TILE_SIZE, GetPosition().top / TILE_SIZE };
    POINT endTile = { m_pPlayer->GetPosition().left / TILE_SIZE, m_pPlayer->GetPosition().top / TILE_SIZE };
    if (m_pMaze->IsWall(endTile.x, endTile.y)) return false;
    m_path = Pathfinder::FindPath(m_pMaze, startTile, endTile);
    m_pathIndex = 0;
    return !m_path.empty();
}

void Enemy::FollowPath()
{
    if (m_path.empty() || m_pathIndex >= m_path.size()) {
        SetVelocity(0, 0);
        return;
    }
    POINT targetTile = m_path[m_pathIndex];
    float targetX = (float)targetTile.x * TILE_SIZE + (TILE_SIZE / 2.0f);
    float targetY = (float)targetTile.y * TILE_SIZE + (TILE_SIZE / 2.0f);
    float currentX = (float)GetPosition().left + GetWidth() / 2.0f;
    float currentY = (float)GetPosition().top + GetHeight() / 2.0f;
    float distance = sqrt(pow(targetX - currentX, 2) + pow(targetY - currentY, 2));

    if (distance < TILE_SIZE / 2.0f) {
        m_pathIndex++;
        if (m_pathIndex >= m_path.size()) {
            m_path.clear(); // Yol bitti, yenisi bulunana kadar bekle.
            SetVelocity(0, 0);
            return;
        }
    }
    float dirX = targetX - currentX;
    float dirY = targetY - currentY;
    float len = sqrt(dirX * dirX + dirY * dirY);
    if (len > 0) { dirX /= len; dirY /= len; }
    SetVelocity((int)(dirX * 4), (int)(dirY * 4)); // Biraz daha hýzlý takip
}

// HasLineOfSight and AttackPlayer do not need changes, but are included for completeness.
bool Enemy::HasLineOfSightToPlayer()
{
    if (!m_pPlayer || !m_pMaze) return false;
    int x0 = GetPosition().left / TILE_SIZE, y0 = GetPosition().top / TILE_SIZE;
    int x1 = m_pPlayer->GetPosition().left / TILE_SIZE, y1 = m_pPlayer->GetPosition().top / TILE_SIZE;
    if (abs(x0 - x1) > 20 || abs(y0 - y1) > 20) return false;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        if (m_pMaze->IsWall(x0, y0)) return false;
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return true;
}

void Enemy::AttackPlayer()
{
    if (!m_pPlayer || !game_engine || !_pEnemyMissileBitmap) return;
    float playerCenterX = (float)m_pPlayer->GetPosition().left + (m_pPlayer->GetWidth() / 2.0f);
    float playerCenterY = (float)m_pPlayer->GetPosition().top + (m_pPlayer->GetHeight() / 2.0f);
    float enemyCenterX = (float)GetPosition().left + GetWidth() / 2.0f;
    float enemyCenterY = (float)GetPosition().top + GetHeight() / 2.0f;
    float dirX = playerCenterX - enemyCenterX, dirY = playerCenterY - enemyCenterY;
    float length = sqrt(dirX * dirX + dirY * dirY);
    if (length > 0) { dirX /= length; dirY /= length; }
    RECT rcBounds = { 0, 0, 50 * (25 * 2 + 1), 50 * (25 * 2 + 1) }; // Updated bounds
    Sprite* pMissile = new Sprite(_pEnemyMissileBitmap, rcBounds, BA_DIE);
    pMissile->SetPosition((int)enemyCenterX - pMissile->GetWidth() / 2, (int)enemyCenterY - pMissile->GetHeight() / 2);
    int missileSpeed = 8;
    pMissile->SetVelocity(static_cast<int>(dirX * missileSpeed), static_cast<int>(dirY * missileSpeed));
    game_engine->AddSprite(pMissile);
}