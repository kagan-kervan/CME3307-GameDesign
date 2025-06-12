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
//Global Variables
//--------------------------------------------------


HINSTANCE   instance;
GameEngine* game_engine;
HDC         offscreenDC;
HBITMAP     offscreenBitmap;
Bitmap* backgroundBitmap;
Bitmap* charBitmap;
Sprite* charSprite;
Background* background;
Bitmap* wallBitmap;
MazeGenerator* mazeGenerator;
Sprite* wallSpriteList;
Bitmap* _pWallBitmap;
Bitmap* _pFloorBitmap;
Bitmap* _pPlayerBitmap;
Bitmap* _pEnemyBitmap;
Bitmap* _pMissileBitmap; // Mermi için Player.cpp'de extern ile eriþiliyor
FOVBackground* fovEffect;
int window_X, window_Y;
int MAZE_WIDTH, MAZE_HEIGHT = 20;
int TILE_SIZE;
Camera* camera = camera;
extern std::vector<Tile> nonCollidableTiles; // Add this line
class Game {
    Game() = default;
};

//Functions to be used

void GenerateMaze(Bitmap* tileBit);

void AddNonCollidableTile(int x, int y, Bitmap* bitmap); // Add this line

void CenterCameraOnSprite(Sprite* sprite);

#endif //GAME_H
