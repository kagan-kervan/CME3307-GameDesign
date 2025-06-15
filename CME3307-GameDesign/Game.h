// Game.h
#ifndef GAME_H
#define GAME_H
#pragma once
#include <windows.h>
#include "Resource.h" 
#include "GameEngine.h"
#include "Bitmap.h"
#include "Sprite.h"
#include "Background.h"
#include "MazeGenerator.h"
#include "Camera.h"
#include "Player.h"
#include "Enemy.h"
#include "FOVBackground.h"
#include <vector>

struct Tile {
    int x, y;
    Bitmap* bitmap;
};
struct HighScoreEntry {
    int score;
    std::string timestamp;
    bool operator<(const HighScoreEntry& other) const {
        return score < other.score;
    }
};

//--------------------------------------------------
//Global Variables (Declarations)
//--------------------------------------------------

extern HINSTANCE   instance;
extern GameEngine* game_engine;
extern Player* charSprite;
extern MazeGenerator* mazeGenerator;
extern Bitmap* _pEnemyBitmap;
extern Bitmap* _pTurretEnemyBitmap;
extern Bitmap* _pRobotTurretEnemyBitmap; // YENÝ: Robot Turret düþmaný için bitmap
extern Bitmap* _pEnemyMissileBitmap;
extern int TILE_SIZE;
extern FOVBackground* fovEffect;
extern Camera* camera;
extern std::vector<Tile> nonCollidableTiles;
extern Bitmap* healthPWBitmap;
extern Bitmap* ammoPWBitmap;
extern Bitmap* armorPWBitmap;
extern Bitmap* pointPWBitmap;
extern Bitmap* wallBitmap;
extern Bitmap* floorBitmap;
extern Bitmap* keyBitmap;
extern Bitmap* endPointBitmap;
extern Bitmap* secondWeaponBitmap;
extern Bitmap* _pPlayerMissileBitmap;

extern DWORD g_dwLastSpawnTime;
extern DWORD g_dwLastClosestEnemySpawnTime;
extern DWORD g_dwLastRobotTurretSpawnTime; // YENÝ: Robot Turret spawn zamanlayýcýsý
extern bool isLevelFinished;

extern int  currentLevel;
extern HCURSOR g_hCrosshairCursor;
extern int window_X, window_Y;

void GenerateLevel(int level);
void GenerateMaze(Bitmap* tileBit);
void AddNonCollidableTile(int x, int y, Bitmap* bitmap);
void CleanupLevel();
void LoadBitmaps(HDC hDC);
void OnLevelComplete();

void SpawnEnemyNearPlayer();
void SpawnEnemyNearClosest();
void SpawnRobotTurretEnemy(); // YENÝ: Robot Turret spawn fonksiyonu

void DrawUI(HDC hDC);
void LoadHighScores();
void SaveHighScores();
void CheckAndSaveScore(int finalScore);
void RestartGame();
std::string GetCurrentTimestamp();
// YENÝ: Yardýmcý fonksiyonun deklarasyonu (bildirimi)
double LerpAngle(double a, double b, float t);

#endif //GAME_H