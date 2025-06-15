#include "Enemy.h"
#include "Game.h"       // _pEnemyMissileBitmap ve game_engine için
#include "GameEngine.h" // game_engine için
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <algorithm> // std::min, std::max için

// A* Pathfinding (Bu kýsým ayný kalýyor)
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

    switch (m_type)
    {
    case EnemyType::CHASER:
        m_iHealth = 1;
        Sprite::SetNumFrames(4);
        Sprite::SetFrameDelay(8);
        break;
    case EnemyType::TURRET:
        m_iHealth = 1;
        Sprite::SetNumFrames(4);
        Sprite::SetFrameDelay(8);
        break;
    case EnemyType::ROBOT_TURRET:
        m_iHealth = 3;
        Sprite::SetNumFrames(8);
        Sprite::SetFrameDelay(6);
        break;
    default:
        m_iHealth = 1;
        Sprite::SetNumFrames(4);
        Sprite::SetFrameDelay(8);
        break;
    }
}

void Enemy::TakeDamage(int amount)
{
    if (IsDead()) return;

    m_iHealth -= amount;
    if (m_iHealth <= 0)
    {
        m_iHealth = 0;
        Kill();
    }
}

bool Enemy::IsDead() const
{
    return m_iHealth <= 0;
}


void Enemy::ResolveWallCollisions(POINT& desiredVelocity)
{
    // DÜZELTME: Sadece ROBOT_TURRET hareket etmiyorsa çarpýþma çözmeye gerek yok.
    // TURRET hareket ediyorsa (pathfinding ile) çarpýþma çözümü gerekir.
    if (m_type == EnemyType::ROBOT_TURRET) {
        desiredVelocity = { 0,0 };
        return;
    }

    if (!m_pMaze || TILE_SIZE == 0) return;

    RECT currentSpritePos = GetPosition();
    int spriteWidth = GetWidth();
    int spriteHeight = GetHeight();

    if (desiredVelocity.x != 0) {
        RECT nextXPos = currentSpritePos;
        nextXPos.left += desiredVelocity.x;
        nextXPos.right += desiredVelocity.x;

        int testTileYTop = nextXPos.top / TILE_SIZE;
        int testTileYBottom = (nextXPos.bottom - 1) / TILE_SIZE;
        int testTileYMid = (nextXPos.top + spriteHeight / 2) / TILE_SIZE;

        if (desiredVelocity.x > 0) {
            int testTileX = (nextXPos.right - 1) / TILE_SIZE;
            if (m_pMaze->IsWall(testTileX, testTileYTop) ||
                m_pMaze->IsWall(testTileX, testTileYBottom) ||
                m_pMaze->IsWall(testTileX, testTileYMid)) {
                SetPosition((testTileX * TILE_SIZE) - spriteWidth, currentSpritePos.top);
                desiredVelocity.x = 0;
            }
        }
        else {
            int testTileX = nextXPos.left / TILE_SIZE;
            if (m_pMaze->IsWall(testTileX, testTileYTop) ||
                m_pMaze->IsWall(testTileX, testTileYBottom) ||
                m_pMaze->IsWall(testTileX, testTileYMid)) {
                SetPosition((testTileX + 1) * TILE_SIZE, currentSpritePos.top);
                desiredVelocity.x = 0;
            }
        }
    }

    currentSpritePos = GetPosition();

    if (desiredVelocity.y != 0) {
        RECT nextYPos = currentSpritePos;
        nextYPos.top += desiredVelocity.y;
        nextYPos.bottom += desiredVelocity.y;

        int testTileXLeft = nextYPos.left / TILE_SIZE;
        int testTileXRight = (nextYPos.right - 1) / TILE_SIZE;
        int testTileXMid = (nextYPos.left + spriteWidth / 2) / TILE_SIZE;

        if (desiredVelocity.y > 0) {
            int testTileY = (nextYPos.bottom - 1) / TILE_SIZE;
            if (m_pMaze->IsWall(testTileXLeft, testTileY) ||
                m_pMaze->IsWall(testTileXRight, testTileY) ||
                m_pMaze->IsWall(testTileXMid, testTileY)) {
                SetPosition(currentSpritePos.left, (testTileY * TILE_SIZE) - spriteHeight);
                desiredVelocity.y = 0;
            }
        }
        else {
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
    // YENÝ: Eðer ölme durumundaysak, AI veya hareketi güncelleme.
    // Bunun yerine ölüm animasyonunu oluþturma ve kendini öldürme eylemlerini döndür.
    if (m_bDying)
    {
        return SA_ADDSPRITE | SA_KILL;
    }

    // Eðer canlýysak, normal AI'yý çalýþtýr
    UpdateAI();

    POINT currentVelocity = GetVelocity();
    ResolveWallCollisions(currentVelocity);
    SetVelocity(currentVelocity.x, currentVelocity.y);

    // Temel Sprite::Update'i çaðýr (animasyon karesi güncellemesi vb. için)
    return Sprite::Update();
}

// YENÝ: BU FONKSÝYONU Enemy.cpp DOSYASINA EKLEYÝN
Sprite* Enemy::AddSprite()
{
    // Bu fonksiyon sadece düþman ölürken çaðrýlýr.
    // Ölüm animasyonu için yeni bir sprite oluþturur ve döndürür.

    // Gerekli global bitmap'in yüklendiðinden emin ol
    if (!_pDeathEffectBitmap) return NULL;

    // Ölen düþmanýn merkez pozisyonunu al
    RECT rcPos = GetPosition();
    POINT ptCenter = { rcPos.left + GetWidth() / 2, rcPos.top + GetHeight() / 2 };

    // Ölüm efekti için yeni bir sprite oluþtur
    Sprite* pDeathSprite = new Sprite(_pDeathEffectBitmap);

    // Animasyonu ayarla.
    // NOT: IDB_TIMMY bitmap'inizdeki kare sayýsýný buraya doðru girin!
    // Örneðin 5 kare varsa:
    pDeathSprite->SetNumFrames(5, TRUE); // TRUE -> animasyon tek sefer oynasýn ve bitsin
    pDeathSprite->SetFrameDelay(5);      // Animasyon hýzý (düþük deðer = hýzlý)

    // Efektin pozisyonunu, ölen düþmanýn merkezine gelecek þekilde ayarla
    int frameWidth = pDeathSprite->GetWidth(); // Bir animasyon karesinin geniþliði
    int frameHeight = pDeathSprite->GetHeight();
    pDeathSprite->SetPosition(ptCenter.x - frameWidth / 2, ptCenter.y - frameHeight / 2);

    // Efektin hareket etmediðinden emin ol
    pDeathSprite->SetVelocity(0, 0);

    // Yeni oluþturulan ölüm efekti sprite'ýný oyun motoruna döndür
    return pDeathSprite;
}

void Enemy::UpdateAI()
{
    if (m_attackCooldown > 0) m_attackCooldown--;
    if (m_pathfindingCooldown > 0) m_pathfindingCooldown--;

    if (!m_pPlayer || !game_engine) {
        SetVelocity(0, 0);
        return;
    }
    if (TILE_SIZE == 0) {
        SetVelocity(0, 0);
        return;
    }

    float playerDistance = sqrt(pow(static_cast<float>(m_pPlayer->GetPosition().left - m_rcPosition.left), 2.0f) +
        pow(static_cast<float>(m_pPlayer->GetPosition().top - m_rcPosition.top), 2.0f));

    if (playerDistance > TILE_SIZE * 25) { // Genel görüþ mesafesi
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
        else if (m_type == EnemyType::TURRET || m_type == EnemyType::ROBOT_TURRET) {
            m_state = AIState::ATTACKING;
        }
    }
    else { // Görüþ hattý yoksa
        // DÜZELTME: Sadece ROBOT_TURRET IDLE kalýr, CHASER ve TURRET CHASING'e (yol bulma) geçer
        if (m_type == EnemyType::ROBOT_TURRET) {
            m_state = AIState::IDLE;
            SetVelocity(0, 0); // Robot turret LOS yoksa tamamen durur
        }
        else { // CHASER veya TURRET
            m_state = AIState::CHASING;
        }
    }

    switch (m_state)
    {
    case AIState::IDLE:
        SetVelocity(0, 0);
        // DÜZELTME: ROBOT_TURRET IDLE durumunda yol bulmaya çalýþmaz.
        // CHASER ve TURRET, LOS yoksa ve oyuncu yakýnsa yol bulmaya çalýþýr.
        if (m_type == EnemyType::CHASER || m_type == EnemyType::TURRET) {
            if (m_pathfindingCooldown <= 0 && playerDistance <= TILE_SIZE * 15) {
                if (FindPath()) {
                    m_state = AIState::CHASING;
                }
                m_pathfindingCooldown = 30;
            }
        }
        break;

    case AIState::CHASING:
        // DÜZELTME: CHASER ve TURRET hareket eder ve yol takip eder. ROBOT_TURRET bu duruma girmemeli.
        if (m_type == EnemyType::CHASER || m_type == EnemyType::TURRET) {
            if (!m_path.empty() && m_pathIndex < m_path.size()) {
                FollowPath();
            }
            else { // Yol yok veya bittiyse (veya TURRET için direkt LOS yoksa ve yol bulamadýysa)
                float dirX = static_cast<float>(m_pPlayer->GetPosition().left - m_rcPosition.left);
                float dirY = static_cast<float>(m_pPlayer->GetPosition().top - m_rcPosition.top);
                float len = sqrt(dirX * dirX + dirY * dirY);
                if (len > 0) { dirX /= len; dirY /= len; }
                SetVelocity(static_cast<int>(dirX * 8), static_cast<int>(dirY * 8));

                if (m_pathfindingCooldown <= 0) {
                    FindPath();
                    m_pathfindingCooldown = 30;
                }
            }
        }
        else if (m_type == EnemyType::ROBOT_TURRET) { // Güvenlik önlemi, ROBOT_TURRET CHASING'de olmamalý
            m_state = AIState::IDLE;
            SetVelocity(0, 0);
        }
        break;

    case AIState::ATTACKING:
        SetVelocity(0, 0); // Saldýrýrken tüm düþmanlar sabit durur
        if (m_attackCooldown <= 0) {
            AttackPlayer();
            if (m_type == EnemyType::TURRET) {
                m_attackCooldown = 35;
            }
            else if (m_type == EnemyType::ROBOT_TURRET) {
                m_attackCooldown = 25;
            }
            else { // CHASER
                m_attackCooldown = 50;
            }
        }
        break;
    }
}

bool Enemy::FindPath()
{
    // DÜZELTME: Sadece ROBOT_TURRET yol bulmaya çalýþmaz.
    if (m_type == EnemyType::ROBOT_TURRET) return false;

    if (!m_pPlayer || !m_pMaze || TILE_SIZE == 0 || m_pMaze->GetMaze().empty() || m_pMaze->GetMaze()[0].empty()) return false;

    POINT startTile = { (m_rcPosition.left + GetWidth() / 2) / TILE_SIZE, (m_rcPosition.top + GetHeight() / 2) / TILE_SIZE };
    POINT endTile = { (m_pPlayer->GetPosition().left + m_pPlayer->GetWidth() / 2) / TILE_SIZE, (m_pPlayer->GetPosition().top + m_pPlayer->GetHeight() / 2) / TILE_SIZE };

    int mazeWidth = m_pMaze->GetMaze()[0].size();
    int mazeHeight = m_pMaze->GetMaze().size();

    if (startTile.x < 0 || startTile.x >= mazeWidth || startTile.y < 0 || startTile.y >= mazeHeight ||
        endTile.x < 0 || endTile.x >= mazeWidth || endTile.y < 0 || endTile.y >= mazeHeight) {
        return false;
    }
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
    // DÜZELTME: Sadece ROBOT_TURRET yol takip etmez.
    if (m_type == EnemyType::ROBOT_TURRET) {
        SetVelocity(0, 0);
        m_path.clear();
        return;
    }

    if (m_path.empty() || m_pathIndex >= m_path.size() || TILE_SIZE == 0) {
        SetVelocity(0, 0);
        m_path.clear();
        // DÜZELTME: Eðer yol bittiyse ve TURRET ise, IDLE'a deðil, tekrar path bulmaya veya oyuncuya yönelmeye çalýþsýn (CHASING durumu yönetecek)
        if (m_type == EnemyType::TURRET || m_type == EnemyType::CHASER) {
            m_state = AIState::CHASING; // Tekrar yol bulmaya zorla
        }
        return;
    }

    POINT targetTile = m_path[m_pathIndex];
    float targetX = static_cast<float>(targetTile.x * TILE_SIZE) + (TILE_SIZE / 2.0f);
    float targetY = static_cast<float>(targetTile.y * TILE_SIZE) + (TILE_SIZE / 2.0f);

    float currentX = static_cast<float>(m_rcPosition.left + GetWidth() / 2.0f);
    float currentY = static_cast<float>(m_rcPosition.top + GetHeight() / 2.0f);

    float distanceToTargetTileCenter = sqrt(pow(targetX - currentX, 2.0f) + pow(targetY - currentY, 2.0f));

    if (distanceToTargetTileCenter < TILE_SIZE / 1.5f) {
        m_pathIndex++;
        if (m_pathIndex >= m_path.size()) {
            m_path.clear();
            SetVelocity(0, 0);
            // DÜZELTME: Yol bittiðinde TURRET ve CHASER CHASING durumuna geri dönmeli
            if (m_type == EnemyType::TURRET || m_type == EnemyType::CHASER) {
                m_state = AIState::CHASING;
            }
            return;
        }
        // Bir sonraki hedef tile'ý ayarla, UpdateAI'daki CHASING durumu yeni hedef için hýzý belirleyecek
        // Bu yüzden burada tekrar hýz ayarlamaya gerek yok, sadece indeksi ilerlet.
    }

    // Hedefe doðru yönelme (bu kýsým hala gerekli, çünkü bir sonraki tile'a ulaþana kadar hareket etmeli)
    float dirX = targetX - currentX;
    float dirY = targetY - currentY;
    float len = sqrt(dirX * dirX + dirY * dirY);
    if (len > 0) { dirX /= len; dirY /= len; }

    SetVelocity(static_cast<int>(dirX * 10), static_cast<int>(dirY * 10)); // Hýz sabit kalabilir
}

bool Enemy::HasLineOfSightToPlayer()
{
    if (!m_pPlayer || !m_pMaze || TILE_SIZE == 0 || m_pMaze->GetMaze().empty() || m_pMaze->GetMaze()[0].empty()) return false;

    int x0 = (m_rcPosition.left + GetWidth() / 2) / TILE_SIZE;
    int y0 = (m_rcPosition.top + GetHeight() / 2) / TILE_SIZE;
    int x1 = (m_pPlayer->GetPosition().left + m_pPlayer->GetWidth() / 2) / TILE_SIZE;
    int y1 = (m_pPlayer->GetPosition().top + m_pPlayer->GetHeight() / 2) / TILE_SIZE;

    int mazeWidth = m_pMaze->GetMaze()[0].size();
    int mazeHeight = m_pMaze->GetMaze().size();

    if (x0 < 0 || x0 >= mazeWidth || y0 < 0 || y0 >= mazeHeight ||
        x1 < 0 || x1 >= mazeWidth || y1 < 0 || y1 >= mazeHeight) {
        return false;
    }

    // Görüþ hattý mesafesini tile bazýnda sýnýrla (bu kalabilir)
    if (abs(x0 - x1) > 15 || abs(y0 - y1) > 15) return false;

    int dx_abs = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy_abs = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx_abs - dy_abs; // Bresenham için dy'nin negatif olmasý gerekiyordu, bu düzeltilmiþ versiyon.
    // dy_bresenham = -dy_abs; err = dx + dy_bresenham;

// Düzeltilmiþ Bresenham (pozitif dy ile)
    int x = x0;
    int y = y0;

    POINT startTile = { x0, y0 };
    POINT endTile = { x1, y1 };

    while (true) {
        if (!(x == startTile.x && y == startTile.y) && !(x == endTile.x && y == endTile.y)) {
            if (x < 0 || x >= mazeWidth || y < 0 || y >= mazeHeight) return false; // Sýnýr kontrolü
            if (m_pMaze->IsWall(x, y)) return false;
        }
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy_abs) { // Eðim > -1
            err -= dy_abs;
            x += sx;
        }
        if (e2 < dx_abs) { // Eðim < 1
            err += dx_abs;
            y += sy;
        }
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
        rcMissileBounds = { 0, 0, 4000, 4000 };
    }

    Sprite* pMissile = new Sprite(_pEnemyMissileBitmap, rcMissileBounds, BA_DIE, SPRITE_TYPE_ENEMY_MISSILE);
    pMissile->SetPosition(static_cast<int>(enemyCenterX - pMissile->GetWidth() / 2.0f),
        static_cast<int>(enemyCenterY - pMissile->GetHeight() / 2.0f));

    int missileSpeed = 12;
    pMissile->SetVelocity(static_cast<int>(dirX * missileSpeed), static_cast<int>(dirY * missileSpeed));

    game_engine->AddSprite(pMissile);
}