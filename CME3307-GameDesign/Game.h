//
// Created by ahmet on 3.06.2025.
//

#ifndef GAME_H
#define GAME_H
#include <windows.h>
#include "Resource.h"
#include "GameEngine.h"
#include "Bitmap.h"
#include "Sprite.h"
#include "Background.h"
#include  "MazeGenerator.h"
#include "Camera.h"
#include "Player.h"
#include "Enemy.h"
#include "FOVBackground.h"
#include <vector>

//Structures to be used
struct Tile {
    int x, y;
    Bitmap* bitmap;
};


//--------------------------------------------------
//Global Variables (Declarations)
//--------------------------------------------------

extern HINSTANCE   instance;
extern GameEngine* game_engine;
extern Player* charSprite;
extern MazeGenerator* mazeGenerator;
extern Bitmap* _pEnemyMissileBitmap;
extern int TILE_SIZE;
extern FOVBackground* fovEffect;
extern Camera* camera;
extern std::vector<Tile> nonCollidableTiles;
extern Bitmap* healthPWBitmap;
extern Bitmap* ammoPWBitmap;
extern Bitmap* armorPWBitmap;
extern Bitmap* pointPWBitmap;
extern Bitmap* wallBitmap; // Assuming you have a specific wall bitmap
extern Bitmap* floorBitmap; // Assuming you have a specific floor bitmap
extern Bitmap* keyBitmap;
extern Bitmap* endPointBitmap;
extern Bitmap* secondWeaponBitmap;


extern HFONT       g_hUIFont;
extern HFONT       g_hBigFont; // For "Level Complete" text
extern BOOL        g_bInLevelTransition;
extern DWORD       g_dwTransitionStartTime;

extern bool isLevelFinished;

extern int  currentLevel; // Add a global variable for the current level

// Add declarations for window dimensions
extern int window_X, window_Y;

//Functions to be used

void GenerateLevel(int level); // Renamed for clarity, as it does more than generate a maze
void GenerateMaze(Bitmap* tileBit);
void AddNonCollidableTile(int x, int y, Bitmap* bitmap);
void CenterCameraOnSprite(Sprite* sprite);
void CleanupLevel(); // Good practice to have a function to clear old sprites
void LoadBitmaps(HDC hDC);
void DrawUI(HDC hDC);

// Yeni seviye oluþturma fonksiyonu
void OnLevelComplete();

#endif //GAME_H