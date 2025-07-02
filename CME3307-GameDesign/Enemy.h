#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"
#include <vector>
#include <windows.h>

// Forward declaration
class GameEngine;
extern int TILE_SIZE;

// YENÝ: Robot Turret düþman tipi eklendi
enum class EnemyType { CHASER, TURRET, ROBOT_TURRET, RANDOM_WALKER };
enum class AIState { IDLE, CHASING, ATTACKING };

class Enemy : public Sprite
{
public:
    Enemy(Bitmap* pBitmap, RECT& rcBounds, BOUNDSACTION baBoundsAction,
        MazeGenerator* pMaze, Sprite* pPlayer, EnemyType type);
    EnemyType GetEnemyType() const { return m_type; };
    virtual SPRITEACTION Update(); // Override edeceðiz
    virtual Sprite* AddSprite(); // YENÝ: Ölüm sprite'ý yaratmak için override ediyoruz.
    // YENÝ: Düþmanlarýn can yönetimi için fonksiyonlar
    void TakeDamage(int amount);
    bool IsDead() const;
    int GetHealth() const { return m_iHealth; } // Ýsteðe baðlý, debug veya UI için

private:
    void UpdateAI();
    bool FindPath();
    void FollowPath();
    bool HasLineOfSightToPlayer();
    void AttackPlayer();

    void ResolveWallCollisions(POINT& desiredVelocity);

    MazeGenerator* m_pMaze;
    Sprite* m_pPlayer;
    EnemyType m_type;
    AIState m_state;

    std::vector<POINT> m_path;
    int m_pathIndex;
    int m_attackCooldown;
    int m_pathfindingCooldown;

    int m_randomMoveTimer;
    POINT m_randomMoveDirection;

    // YENÝ: Düþman caný
    int m_iHealth;
};