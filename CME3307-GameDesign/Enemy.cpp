// Enemy.cpp

#include "Enemy.h"
#include "Game.h"       // _pEnemyMissileBitmap ve game_engine için
#include "GameEngine.h" // game_engine için (Game.h zaten bunu içeriyor olabilir ama doðrudan eklemek daha güvenli)
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>

// A* Pathfinding (Bu kýsým ayný kalýyor)
namespace Pathfinder {
    struct Node { int x, y, gCost, hCost; Node* parent; Node(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {} int fCost() const { return gCost + hCost; } };
    struct CompareNode { bool operator()(const Node* a, const Node* b) const { return a->fCost() > b->fCost(); } };
    int calculateHCost(int x1, int y1, int x2, int y2) { return abs(x1 - x2) + abs(y1 - y2); }
    std::vector<POINT> reconstructPath(Node* endNode) { std::vector<POINT> p; Node* c = endNode; while (c != nullptr) { p.push_back({ c->x, c->y }); c = c->parent; } std::reverse(p.begin(), p.end()); return p; }
    std::vector<POINT> FindPath(MazeGenerator* maze, POINT start, POINT end) {
        if (!maze) return {}; // maze null ise boþ yol döndür
        std::priority_queue<Node*, std::vector<Node*>, CompareNode> o; std::map<std::pair<int, int>, Node*> a;
        Node* s = new Node(start.x, start.y); s->hCost = calculateHCost(start.x, start.y, end.x, end.y); o.push(s); a[{start.x, start.y}] = s;
        int dx[] = { 0, 0, 1, -1 }, dy[] = { 1, -1, 0, 0 };
        while (!o.empty()) {
            Node* c = o.top(); o.pop();
            if (c->x == end.x && c->y == end.y) { std::vector<POINT> p = reconstructPath(c); for (auto i = a.begin(); i != a.end(); ++i) delete i->second; a.clear(); return p; }
            for (int i = 0; i < 4; ++i) {
                int nX = c->x + dx[i], nY = c->y + dy[i];
                // IsWall çaðrýsýndan önce maze null kontrolü zaten FindPath baþýnda yapýldý.
                if (maze->IsWall(nX, nY)) continue;
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


Enemy::Enemy(Bitmap* pBitmap, RECT& rcBounds, BOUNDSACTION baBoundsAction,
    MazeGenerator* pMaze, Sprite* pPlayer, EnemyType type)
    : Sprite(pBitmap, rcBounds, baBoundsAction, SPRITE_TYPE_ENEMY), m_pMaze(pMaze), m_pPlayer(pPlayer), m_type(type)
{
    m_state = AIState::IDLE;
    m_pathIndex = 0;
    m_attackCooldown = 0;
    m_pathfindingCooldown = 0;

    Sprite::SetNumFrames(4); // Düþman animasyonu için frame sayýsý (varsa)
    Sprite::SetFrameDelay(8); // Animasyon hýzý
}

SPRITEACTION Enemy::Update()
{
    UpdateAI();
    return Sprite::Update(); // Sprite'ýn temel Update'ini çaðýr (hareket, animasyon, sýnýr kontrolü)
}

void Enemy::UpdateAI()
{
    if (m_attackCooldown > 0) m_attackCooldown--;
    if (m_pathfindingCooldown > 0) m_pathfindingCooldown--;

    if (!m_pPlayer || !game_engine) { // game_engine null kontrolü eklendi
        SetVelocity(0, 0);
        return;
    }

    // TILE_SIZE global deðiþkenini kullanýyoruz, null/sýfýr olmamalý.
    // Bu kontrolü Game.cpp'de TILE_SIZE'ýn atandýðý yerde yapmak daha mantýklý olabilir.
    // Þimdilik burada bir güvenlik önlemi olarak býrakýyorum.
    if (TILE_SIZE == 0) {
        SetVelocity(0, 0);
        return;
    }


    float playerDistance = sqrt(pow(static_cast<float>(m_pPlayer->GetPosition().left - m_rcPosition.left), 2.0f) +
        pow(static_cast<float>(m_pPlayer->GetPosition().top - m_rcPosition.top), 2.0f));

    // Görüþ mesafesini biraz daha artýralým (örneðin 25 tile)
    if (playerDistance > TILE_SIZE * 25) { // Önceki 20 tile idi
        m_state = AIState::IDLE;
        SetVelocity(0, 0);
        return;
    }

    bool hasLOS = HasLineOfSightToPlayer();

    if (hasLOS) {
        m_path.clear(); // Görüþ hattý varsa önceki yolu temizle
        m_pathIndex = 0;
        if (m_type == EnemyType::CHASER) {
            m_state = AIState::CHASING; // Chaser direkt kovalasýn
        }
        else { // TURRET tipi
            m_state = AIState::ATTACKING; // Turret direkt ateþ etsin
        }
    }
    else {
        // Görüþ hattý yoksa her zaman CHASER gibi davranýp yol bulmaya çalýþsýn
        m_state = AIState::CHASING;
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
        else { // Yol yoksa veya bittiyse
            // Direkt oyuncuya doðru basit bir yönelim (eðer pathfinding baþarýsýz olursa veya cooldown'daysa)
            // Bu kýsým aslýnda pathfinding ile daha iyi yönetilir, ama bir fallback olarak kalabilir.
            float dirX = static_cast<float>(m_pPlayer->GetPosition().left - m_rcPosition.left);
            float dirY = static_cast<float>(m_pPlayer->GetPosition().top - m_rcPosition.top);
            float len = sqrt(dirX * dirX + dirY * dirY);
            if (len > 0) { dirX /= len; dirY /= len; }

            // HAREKET HIZI ARTIRIMI (CHASING - DIRECT)
            // Önceki hýz: * 5 idi, þimdi * 8 yapalým (yaklaþýk %60 artýþ)
            SetVelocity(static_cast<int>(dirX * 8), static_cast<int>(dirY * 8));

            if (m_pathfindingCooldown <= 0) {
                FindPath(); // Yeni yol bulmayý dene
                m_pathfindingCooldown = 30; // Pathfinding cooldown'unu biraz azaltalým (önceki 45 idi)
                // Daha sýk yol bulmaya çalýþýr.
            }
        }
        break;

    case AIState::ATTACKING:
        SetVelocity(0, 0); // Saldýrýrken sabit dur
        if (m_attackCooldown <= 0) {
            AttackPlayer();
            // ATEÞ ETME HIZI ARTIRIMI (COOLDOWN AZALTMA)
            // Önceki cooldown: 60 idi, þimdi 35 yapalým (yaklaþýk %40 daha hýzlý ateþ)
            // TURRET tipi için
            m_attackCooldown = (m_type == EnemyType::TURRET) ? 35 : 50; // Chaser biraz daha yavaþ ateþ edebilir.
        }
        break;
    }
}

bool Enemy::FindPath()
{
    if (!m_pPlayer || !m_pMaze || TILE_SIZE == 0) return false;

    // Hedef tile oyuncunun tam ortasý deðil, en yakýn grid hücresi olmalý
    POINT startTile = { m_rcPosition.left / TILE_SIZE, m_rcPosition.top / TILE_SIZE };
    POINT endTile = { m_pPlayer->GetPosition().left / TILE_SIZE, m_pPlayer->GetPosition().top / TILE_SIZE };

    // Bitiþ noktasý bir duvar ise yol bulmaya çalýþma.
    // Ancak bazen oyuncu çok hýzlý hareket ederken anlýk olarak duvar içinde görünebilir.
    // Bu durumda, oyuncunun hemen yanýndaki geçerli bir tile'ý hedeflemek daha iyi olabilir.
    // Þimdilik basit tutalým:
    if (m_pMaze->IsWall(endTile.x, endTile.y)) {
        // Hedef duvar ise, etrafýndaki boþ bir tile'ý dene (basit bir deneme)
        int dx[] = { 0, 0, 1, -1, 1, 1, -1, -1 };
        int dy[] = { 1, -1, 0, 0, 1, -1, 1, -1 };
        bool foundAlternative = false;
        for (int i = 0; i < 8; ++i) {
            int altX = endTile.x + dx[i];
            int altY = endTile.y + dy[i];
            if (!m_pMaze->IsWall(altX, altY)) {
                endTile = { altX, altY };
                foundAlternative = true;
                break;
            }
        }
        if (!foundAlternative) return false; // Alternatif de bulunamazsa çýk
    }


    m_path = Pathfinder::FindPath(m_pMaze, startTile, endTile);
    m_pathIndex = 0;
    return !m_path.empty();
}

void Enemy::FollowPath()
{
    if (m_path.empty() || m_pathIndex >= m_path.size() || TILE_SIZE == 0) {
        SetVelocity(0, 0);
        m_path.clear(); // Yol bittiyse veya geçersizse temizle
        return;
    }

    POINT targetTile = m_path[m_pathIndex];
    // Hedef konumu tile'ýn merkezi yap
    float targetX = static_cast<float>(targetTile.x * TILE_SIZE) + (TILE_SIZE / 2.0f);
    float targetY = static_cast<float>(targetTile.y * TILE_SIZE) + (TILE_SIZE / 2.0f);

    // Mevcut konum düþmanýn merkezi
    float currentX = static_cast<float>(m_rcPosition.left + GetWidth() / 2.0f);
    float currentY = static_cast<float>(m_rcPosition.top + GetHeight() / 2.0f);

    float distanceToTargetTileCenter = sqrt(pow(targetX - currentX, 2.0f) + pow(targetY - currentY, 2.0f));

    // Hedef tile'a yeterince yaklaþýldýysa bir sonraki tile'a geç
    if (distanceToTargetTileCenter < TILE_SIZE / 1.5f) { // Biraz daha toleranslý olabilir (önceki TILE_SIZE / 2.0f idi)
        m_pathIndex++;
        if (m_pathIndex >= m_path.size()) {
            m_path.clear(); // Yolun sonuna gelindi
            SetVelocity(0, 0);
            return;
        }
        // Bir sonraki hedef tile'ý hemen alalým, böylece yönelim daha erken güncellenir.
        targetTile = m_path[m_pathIndex];
        targetX = static_cast<float>(targetTile.x * TILE_SIZE) + (TILE_SIZE / 2.0f);
        targetY = static_cast<float>(targetTile.y * TILE_SIZE) + (TILE_SIZE / 2.0f);
    }

    float dirX = targetX - currentX;
    float dirY = targetY - currentY;
    float len = sqrt(dirX * dirX + dirY * dirY);
    if (len > 0) { dirX /= len; dirY /= len; }

    // HAREKET HIZI ARTIRIMI (FOLLOW_PATH)
    // Önceki hýz: * 7 idi, þimdi * 10 yapalým (yaklaþýk %40 artýþ)
    SetVelocity(static_cast<int>(dirX * 10), static_cast<int>(dirY * 10));
}

bool Enemy::HasLineOfSightToPlayer()
{
    if (!m_pPlayer || !m_pMaze || TILE_SIZE == 0) return false;

    // Bresenham çizgi algoritmasý ile görüþ hattý kontrolü
    int x0 = m_rcPosition.left / TILE_SIZE;
    int y0 = m_rcPosition.top / TILE_SIZE;
    int x1 = m_pPlayer->GetPosition().left / TILE_SIZE;
    int y1 = m_pPlayer->GetPosition().top / TILE_SIZE;

    // Çok uzaksa direkt false dön (performans için)
    // Bu mesafe, UpdateAI içindeki genel detect mesafesinden küçük olmalý
    if (abs(x0 - x1) > 15 || abs(y0 - y1) > 15) return false; // Önceki 20 idi

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        // Baþlangýç noktasýndaki duvar kontrolünü atla (düþman kendi içindeyse takýlmasýn)
        if (!(x0 == (m_rcPosition.left / TILE_SIZE) && y0 == (m_rcPosition.top / TILE_SIZE))) {
            if (m_pMaze->IsWall(x0, y0)) return false; // Arada duvar var
        }
        if (x0 == x1 && y0 == y1) break; // Hedefe ulaþýldý
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return true; // Engel yok, görüþ hattý var
}

void Enemy::AttackPlayer()
{
    // _pEnemyMissileBitmap ve game_engine global deðiþkenlerine Game.h üzerinden eriþiliyor.
    // Bu deðiþkenlerin null olup olmadýðýný kontrol etmek iyi bir pratiktir.
    if (!m_pPlayer || !game_engine || !_pEnemyMissileBitmap || TILE_SIZE == 0) return;

    // Oyuncunun ve düþmanýn merkez noktalarýný hesapla
    float playerCenterX = static_cast<float>(m_pPlayer->GetPosition().left + m_pPlayer->GetWidth() / 2.0f);
    float playerCenterY = static_cast<float>(m_pPlayer->GetPosition().top + m_pPlayer->GetHeight() / 2.0f);
    float enemyCenterX = static_cast<float>(m_rcPosition.left + GetWidth() / 2.0f);
    float enemyCenterY = static_cast<float>(m_rcPosition.top + GetHeight() / 2.0f);

    // Ateþ yönünü hesapla
    float dirX = playerCenterX - enemyCenterX;
    float dirY = playerCenterY - enemyCenterY;
    float length = sqrt(dirX * dirX + dirY * dirY);
    if (length > 0) { dirX /= length; dirY /= length; }
    else { return; } // Hedef tam üstündeyse ateþ etme (bölme sýfýr hatasý önlemi)

    // Mermi için sýnýrlar (labirentin tamamý)
    // globalBounds Game.cpp'den geliyor, burada doðrudan eriþimi yok.
    // Merminin hareket edeceði genel alaný tanýmlayan bir RECT gerekli.
    // Þimdilik MazeGenerator'dan labirent boyutlarýný alalým.
    // Bu, merminin Sprite kurucusuna verilecek rcBounds.
    RECT rcMissileBounds;
    if (m_pMaze) {
        const auto& mazeData = m_pMaze->GetMaze();
        if (!mazeData.empty() && !mazeData[0].empty()) {
            rcMissileBounds = { 0, 0,
                                static_cast<long>(mazeData[0].size() * TILE_SIZE),
                                static_cast<long>(mazeData.size() * TILE_SIZE) };
        }
        else { // Labirent verisi yoksa varsayýlan büyük bir alan
            rcMissileBounds = { 0, 0, 4000, 4000 };
        }
    }
    else { // m_pMaze null ise
        rcMissileBounds = { 0, 0, 4000, 4000 };
    }


    // Mermi oluþtur
    // Yeni Missile sýnýfýný kullanýyorsak:
    // Missile* pMissile = new Missile(_pEnemyMissileBitmap, rcMissileBounds,
    //                                {(int)enemyCenterX, (int)enemyCenterY},
    //                                dirX * missileSpeed, dirY * missileSpeed);

    // Eðer hala eski Sprite tabanlý mermi kullanýlýyorsa:
    Sprite* pMissile = new Sprite(_pEnemyMissileBitmap, rcMissileBounds, BA_DIE, SPRITE_TYPE_ENEMY_MISSILE);
    pMissile->SetPosition(static_cast<int>(enemyCenterX - pMissile->GetWidth() / 2.0f),
        static_cast<int>(enemyCenterY - pMissile->GetHeight() / 2.0f));

    // MERMÝ HIZI ARTIRIMI
    // Önceki hýz: 8 idi, þimdi 12 yapalým (yaklaþýk %50 artýþ)
    int missileSpeed = 12;
    pMissile->SetVelocity(static_cast<int>(dirX * missileSpeed), static_cast<int>(dirY * missileSpeed));

    game_engine->AddSprite(pMissile);
}