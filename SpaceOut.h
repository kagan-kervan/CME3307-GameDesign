// MazeGame.h
#pragma once

#include <Windows.h>
#include "resource.h"
#include "GameEngine.h"
#include "Bitmap.h"
#include "Sprite.h"
#include "Background.h"
#include "Maze.h"
#include "Player.h"
#include "Enemy.h"

// Global Deðiþkenler
extern HINSTANCE   _hInstance;
extern GameEngine* _pGame;

// Bitmap'ler
extern Bitmap* _pWallBitmap;
extern Bitmap* _pFloorBitmap;
extern Bitmap* _pPlayerBitmap;
extern Bitmap* _pEnemyBitmap;
extern Bitmap* _pMissileBitmap;

// Oyun Nesneleri
extern Maze* _pMaze;
extern Player* _pPlayerSprite;