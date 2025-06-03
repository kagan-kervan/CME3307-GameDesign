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

//--------------------------------------------------
//Global Variables
//--------------------------------------------------


HINSTANCE   instance;
GameEngine* game_engine;
HDC         offscreenDC;
HBITMAP     offscreenBitmap;
Bitmap* backgroundBitmap;
Background* background;
Bitmap* wallBitmap;
MazeGenerator* mazeGenerator;
Sprite* wallSpriteList;
int window_X, window_Y;
class Game {
    Game() = default;
};

//Functions to be used

void GenerateMaze();


#endif //GAME_H
