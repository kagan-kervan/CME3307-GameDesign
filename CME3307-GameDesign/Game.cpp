//
// Created by ahmet on 3.06.2025.
//

#include "Game.h"
#include <vector>

//--------------------------------------------------
//Global Variable Definitions
//--------------------------------------------------
std::vector<Tile> nonCollidableTiles;
RECT globalBounds = { 0,0,4000,4000 };

GameEngine* game_engine;
Player* charSprite;
MazeGenerator* mazeGenerator;
Bitmap* _pEnemyMissileBitmap;
FOVBackground* fovEffect;
int TILE_SIZE;
Camera* camera;
HDC         offscreenDC;
HBITMAP     offscreenBitmap;
Background* background;
Bitmap* wallBitmap;
Bitmap* charBitmap;
Bitmap* _pEnemyBitmap;
HINSTANCE   instance;
int window_X, window_Y;
extern RECT globalBounds; // Make sure globalBounds is accessible



Bitmap* healthPWBitmap = nullptr;
Bitmap* ammoPWBitmap = nullptr;
Bitmap* armorPWBitmap = nullptr;
Bitmap* pointPWBitmap = nullptr;
Bitmap* floorBitmap = nullptr;
Bitmap* keyBitmap = nullptr;
Bitmap* endPointBitmap = nullptr;
Bitmap* secondWeaponBitmap = nullptr;

bool isLevelFinished = false;
int currentLevel; 

BOOL GameInitialize(HINSTANCE hInst)
{
    window_X = 1500;
    window_Y = 700;
    game_engine = new GameEngine(hInst, TEXT("Maze Game"), TEXT("Maze Game"),
        IDI_SPACEOUT, IDI_SPACEOUT_SM, window_X, window_Y);
    if (game_engine == NULL)
        return FALSE;

    game_engine->SetFrameRate(30);
    instance = hInst;

    return TRUE;
}

void GameStart(HWND hWindow)
{
    srand(GetTickCount());
    offscreenDC = CreateCompatibleDC(GetDC(hWindow));
    offscreenBitmap = CreateCompatibleBitmap(GetDC(hWindow),
        game_engine->GetWidth(), game_engine->GetHeight());
    SelectObject(offscreenDC, offscreenBitmap);

    HDC hDC = GetDC(hWindow);
    LoadBitmaps(hDC);
    TILE_SIZE = wallBitmap->GetHeight();
    background = new Background(window_X, window_Y, RGB(0, 0, 0));
    // DONMAYI AZALTMAK ÝÇÝN: Labirent boyutunu test için makul bir seviyeye getirdim.
    // Performansýn iyi olduðundan emin olunca tekrar büyütebilirsiniz.
    mazeGenerator = new MazeGenerator(15, 15);
    // Player can be created after maze
    charSprite = new Player(charBitmap, mazeGenerator);
    currentLevel = 1;
    GenerateLevel(currentLevel);
    game_engine->AddSprite(charSprite);

    // Initialize Camera and FOV AFTER player is positioned
    camera = new Camera(0, 0, window_X, window_Y);
    CenterCameraOnSprite(charSprite); // Initial camera position
    fovEffect = new FOVBackground(charSprite, 90, 150);


    // Create and add multiple enemies
 // Create and add multiple enemies
    // Düþmanlarý oluþtur
    for (int i = 0; i < 5; i++) // Sayýyý test için azalttým
    {
        EnemyType type = (i < 2) ? EnemyType::TURRET : EnemyType::CHASER;

        // HAREKET HATASI DÜZELTÝLDÝ: Düþmaný tüm harita sýnýrlarýyla (globalBounds) oluþturuyoruz.
        Enemy* pEnemy = new Enemy(_pEnemyBitmap, globalBounds, BA_STOP,
            mazeGenerator, charSprite, type);

        int ex, ey;
        do {
            ex = (rand() % (15 * 2));
            ey = (rand() % (15 * 2));
        } while (mazeGenerator->IsWall(ex, ey));

        pEnemy->SetPosition(ex * TILE_SIZE, ey * TILE_SIZE);
        game_engine->AddSprite(pEnemy);
    }
}

void GameEnd()
{
    // Close the MIDI player for the background music
    game_engine->CloseMIDIPlayer();

    // Cleanup the offscreen device context and bitmap
    DeleteObject(offscreenBitmap);
    DeleteDC(offscreenDC);

    // Cleanup bitmaps
    delete _pEnemyMissileBitmap;

    // Cleanup the background
    delete background;

    // Cleanup the sprites
    game_engine->CleanupSprites();

    // Cleanup the game engine
    delete game_engine;
}

void GameActivate(HWND hWindow)
{
    // Resume the background music
}

void GameDeactivate(HWND hWindow)
{
    // Pause the background music
}

void GamePaint(HDC hDC)
{
    // Draw background with camera offset
    background->Draw(hDC, camera->x, camera->y);

    CenterCameraOnSprite(charSprite);

    // Draw all sprites that are visible in the camera's viewport
    RECT camRect = { camera->x, camera->y, camera->x + camera->width, camera->y + camera->height };



    // Draw all non-collidable tiles
    for (const auto& tile : nonCollidableTiles) {
        // Adjust for camera offset if needed
        tile.bitmap->Draw(hDC, tile.x - camera->x, tile.y - camera->y, TRUE);
    }

    // Draw all sprites with camera offset
    for (Sprite* sprite : game_engine->GetSprites()) {
        sprite->Draw(hDC, camera->x, camera->y);
    }
    fovEffect->Draw(hDC, camera->x, camera->y);
}

void GameCycle()
{
    // Update the background
    background->Update();
    game_engine->UpdateSprites(); // This updates player and enemies

    if (isLevelFinished) {
        OnLevelComplete();
        isLevelFinished = false;
        return;
    }
    // MOUSE BUG FIX: Pass camera coordinates to the FOV update function.
    if (fovEffect && camera) {
        fovEffect->Update(camera->x, camera->y);
    }

    // Obtain a device context for repainting the game
    HWND  hWindow = game_engine->GetWindow();
    HDC   hDC = GetDC(hWindow);

    // Paint the game to the offscreen device context
    // Center camera on sprite
    CenterCameraOnSprite(charSprite);

    GamePaint(offscreenDC);
    // Blit the offscreen bitmap to the game screen
    BitBlt(hDC, 0, 0, game_engine->GetWidth(), game_engine->GetHeight(),
        offscreenDC, 0, 0, SRCCOPY);

    // Cleanup
    ReleaseDC(hWindow, hDC);
}

void HandleKeys()
{

}

void MouseButtonDown(int x, int y, BOOL bLeft)
{

}

void MouseButtonUp(int x, int y, BOOL bLeft)
{

}

void MouseMove(int x, int y)
{
    if (fovEffect)
        fovEffect->UpdateMousePos(x, y);
}

void HandleJoystick(JOYSTATE jsJoystickState) {

}

void GenerateMaze(Bitmap* tileBit) {
    mazeGenerator->generateMaze();
    const std::vector<std::vector<int>>& mazeArray = mazeGenerator->GetMaze();

    int tile_height = wallBitmap->GetHeight();
    int tile_width = wallBitmap->GetWidth();
    RECT rcBounds = { 0, 0, 4000, 4000 }; // or based on maze size

    for (size_t y = 0; y < mazeArray.size(); ++y) {
        for (size_t x = 0; x < mazeArray[y].size(); ++x) {

            int posX = x * tile_width;
            int posY = y * tile_height;
            if (mazeArray[y][x] == -1) { // It's a wall
                POINT pos = { posX, posY };
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
            }
            else {
                AddNonCollidableTile(posX, posY, tileBit); // Instead of creating a Sprite
            }
        }
    }
}

/**
 * @brief Clears old sprites and generates a new level based on the level number.
 * This function builds the visual and physical world from the MazeGenerator data.
 * @param level The level number to generate.
 */
void GenerateLevel(int level) {
    // 1. (Optional but Recommended) Clean up sprites from the previous level.
    // CleanupLevel(); // You would write this function to delete old wall/item sprites.

    // 2. Generate the logical layout for the new level
    mazeGenerator->SetupLevel(level);
    const auto& mazeArray = mazeGenerator->GetMaze();

    // 3. Get tile dimensions (assuming all tiles are the same size)
    if (wallBitmap == nullptr || floorBitmap == nullptr) {
        // Add a safety check to prevent crashes if bitmaps aren't loaded
        game_engine->ErrorQuit(TEXT("Essential bitmaps (wall/floor) are not loaded!"));
        return;
    }
    int tile_width = wallBitmap->GetWidth();
    int tile_height = wallBitmap->GetHeight();
    TILE_SIZE = tile_width; // Update global TILE_SIZE if needed

    // Define the bounding box for all sprites
    // This should be large enough to contain the entire maze
    RECT rcBounds = { 0, 0, mazeArray[0].size() * tile_width, mazeArray.size() * tile_height };

    // 4. Iterate through the maze data and create the game world
    for (size_t y = 0; y < mazeArray.size(); ++y) {
        for (size_t x = 0; x < mazeArray[y].size(); ++x) {
            int posX = x * tile_width;
            int posY = y * tile_height;
            POINT pos = { posX, posY };

            // Use the integer value from the maze array
            int tileValue = mazeArray[y][x];

            // Use a switch to handle each type of tile
            switch (static_cast<TileType>(tileValue)) {
            case TileType::WALL: {
                // Create a collidable wall sprite
                Sprite* wall = new Sprite(wallBitmap, rcBounds, BA_STOP);
                wall->SetPosition(pos);
                game_engine->AddSprite(wall);
                break;
            }

            case TileType::KEY: {
                // Create a collidable key sprite
                Sprite* key = new Sprite(keyBitmap, rcBounds, BA_STOP);
                key->SetPosition(pos);
                // To identify this sprite on collision, you might set a unique ID or use a subclass
                // For now, the game logic can check the bitmap, but that's not ideal.
                game_engine->AddSprite(key);
                // Also draw a floor tile underneath the key
                AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }

            case TileType::HEALTH_PACK: {
                Sprite* item = new Sprite(healthPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::ARMOR_PACK: {
                Sprite* item = new Sprite(armorPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::WEAPON_AMMO: {
                Sprite* item = new Sprite(ammoPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::EXTRA_SCORE: {
                Sprite* item = new Sprite(pointPWBitmap, rcBounds, BA_STOP);
                item->SetPosition(pos);
                game_engine->AddSprite(item);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            //case TileType::SECOND_WEAPON: {
            //    Sprite* item = new Sprite(secondWeaponBitmap, rcBounds, BA_STOP);
            //    item->SetPosition(pos);
            //    game_engine->AddSprite(item);
            //    AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
            //    break;
            //}

            case TileType::END_POINT: {
                // Create a collidable end point sprite
                Sprite* endPoint = new Sprite(endPointBitmap, rcBounds, BA_STOP);
                endPoint->SetPosition(pos);
                game_engine->AddSprite(endPoint);
                AddNonCollidableTile(posX, posY, floorBitmap); // Floor underneath
                break;
            }

            case TileType::START_POINT:
            case TileType::PATH:
            default:
                // For paths and the start point, just draw a non-collidable floor tile
                AddNonCollidableTile(posX, posY, floorBitmap);
                break;
            }
        }
    }

    // 5. After generating the level, place the player at the start position
    std::pair<int, int> startPosCoords = mazeGenerator->GetStartPos();
    if (charSprite != nullptr && startPosCoords.first != -1) {
        charSprite->SetPosition(startPosCoords.first * tile_width, startPosCoords.second * tile_height);
    }
}

void AddNonCollidableTile(int x, int y, Bitmap* bitmap) {
    nonCollidableTiles.push_back({ x, y, bitmap });
}

void CenterCameraOnSprite(Sprite* sprite) {
    if (!sprite || !camera) return;
    RECT pos = sprite->GetPosition();
    int spriteWidth = sprite->GetWidth();
    int spriteHeight = sprite->GetHeight();

    int centerX = pos.left + spriteWidth / 2;
    int centerY = pos.top + spriteHeight / 2;

    int camX = centerX - camera->width / 2;
    int camY = centerY - camera->height / 2;

    camera->SetPosition(camX, camY);
}

void LoadBitmaps(HDC hDC) {

    floorBitmap = new Bitmap(hDC, "tile.bmp");
    wallBitmap = new Bitmap(hDC, "wall.bmp");
    charBitmap = new Bitmap(hDC, IDB_BITMAP3, instance);
    _pEnemyBitmap = new Bitmap(hDC, IDB_ENEMY, instance);
    _pEnemyMissileBitmap = new Bitmap(hDC, IDB_BMISSILE, instance);
    healthPWBitmap = new Bitmap(hDC, "Health.bmp");
    ammoPWBitmap = new Bitmap(hDC, "Ammo.bmp");
    pointPWBitmap = new Bitmap(hDC, "Point.bmp");
    armorPWBitmap = new Bitmap(hDC, "Armor.bmp");
    keyBitmap = new Bitmap(hDC, "Key.bmp");
    endPointBitmap = new Bitmap(hDC, "Gate.bmp");
}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    // Çarpýþan spritelardan birinin player olup olmadýðýný anla
    Player* pPlayer = nullptr;
    Sprite* pOther = nullptr;

    if (pSpriteHitter == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHitter);
        pOther = pSpriteHittee;
    }
    else if (pSpriteHittee == charSprite) {
        pPlayer = static_cast<Player*>(pSpriteHittee);
        pOther = pSpriteHitter;
    }
    else {
        // Çarpýþanlardan hiçbiri player deðil, bu fonksiyonda ilgilenmiyoruz.
        // (Örn: Düþman mermisi duvara çarparsa)
        return FALSE;
    }

    // Player bir þeyle çarpýþtý, neyle çarpýþtýðýný bitmap'inden anla
    Bitmap* pOtherBitmap = pOther->GetBitmap();

    // 1. Anahtar ile çarpýþma
    if (pOtherBitmap == keyBitmap)
    {
        // PlaySound(...); // Anahtar alma sesi
        pPlayer->AddKey();
        pOther->Kill(); // Anahtarý haritadan sil
        // UI'da anahtar sayýsýný güncelle...
    }
    // 2. Can paketi ile çarpýþma
    else if (pOtherBitmap == healthPWBitmap)
    {
        pPlayer->AddHealth(20);
        pOther->Kill();
    }
    // 3. Zýrh paketi ile çarpýþma
    else if (pOtherBitmap == armorPWBitmap)
    {
        pPlayer->AddArmor(20);
        pOther->Kill();
    }
    // 4. Puan ile çarpýþma
    else if (pOtherBitmap == pointPWBitmap)
    {
        pPlayer->AddScore(50);
        pOther->Kill();
    }
    // 5. Ýkinci silah ile çarpýþma
    else if (pOtherBitmap == secondWeaponBitmap)
    {
        pPlayer->GiveSecondWeapon();
        pOther->Kill();
    }
    // 6. Mermi ile çarpýþma (sadece ikinci silah varsa iþe yarar)
    else if (pOtherBitmap == ammoPWBitmap)
    {
        if (pPlayer->HasSecondWeapon()) {
            pPlayer->AddSecondaryAmmo(10);
            pOther->Kill();
        }
        // Ýkinci silah yoksa, mermiyi alamaz, sprite silinmez.
    }
    // 7. Bitiþ noktasý (Gate) ile çarpýþma
    else if (pOtherBitmap == endPointBitmap)
    {
        // Seviyeyi bitirmek için gereken anahtar sayýsýný hesapla
        // (Tasarýmýmýza göre Seviye 1'de 1, 2'de 2, ... 4 ve sonrasýnda 4 anahtar)
        int requiredKeys = min(4, currentLevel);

        if (pPlayer->GetKeys() >= requiredKeys)
        {
            // Yeterli anahtar var! Seviyeyi bitir.
            // PlaySound(...); // Seviye tamamlama sesi
            isLevelFinished = true;
        }
        // Yeterli anahtar yoksa hiçbir þey yapma, kapý kapalý kalýr.
    }

    // Bu fonksiyonun TRUE veya FALSE dönmesi, spritelarýn birbirinin
    // içinden geçip geçemeyeceðini belirler. Genelde item'lar için FALSE
    // dönmek daha mantýklýdýr, böylece oyuncu item'ýn üzerinden geçebilir.
    return FALSE;
}

// Yeni seviye oluþturacak olan yardýmcý fonksiyon
void OnLevelComplete() {
    currentLevel++;

    // UI gösterimi veya bekleme süresi
    // Sleep(3000); // 3 saniye bekle

    CleanupLevel(); // Mevcut haritadaki duvarlarý, item'larý vb. temizle
    GenerateLevel(currentLevel); // Yeni seviyeyi oluþtur
}

// Bu fonksiyon, player hariç tüm spritelarý temizlemeli
void CleanupLevel() {
    // nonCollidableTiles'ý temizle
    nonCollidableTiles.clear();
    // 1. Oyuncuyu motorun listesinden geçici olarak çýkar (ama silme!)
    game_engine->RemoveSprite(charSprite);

    // 2. Þimdi listede oyuncu olmadýðý için, kalan her þeyi güvenle silebiliriz.
    game_engine->CleanupSprites();

    // 3. Oyuncuyu temizlenmiþ listeye geri ekle.
    game_engine->AddSprite(charSprite);
}