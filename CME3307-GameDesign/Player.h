#pragma once
// Player.h
#pragma once
#include "Sprite.h"
#include "MazeGenerator.h"

class Player : public Sprite
{
public:
    Player(Bitmap* pBitmap, MazeGenerator* pMaze);
    int TILE_SIZE = 50;
    virtual SPRITEACTION Update();
    void HandleInput(); // Klavye girdilerini burada iþleyeceðiz

    // Yeni eklenen public metodlar
    void AddKey(int amount = 1);
    int  GetKeys() const;

    void AddHealth(int amount);
    int  GetHealth() const;

    void AddArmor(int amount);
    int  GetArmor() const;

    void AddScore(int amount);
    int  GetScore() const;

    void GiveSecondWeapon();
    bool HasSecondWeapon() const;

    void AddSecondaryAmmo(int amount);
    int  GetSecondaryAmmo() const;


private:
    MazeGenerator* m_pMaze; // Çarpýþma kontrolü için labirent referansý
    int m_iSpeed;

    // Yeni eklenen oyuncu özellikleri
    int m_iKeys;
    int m_iHealth;
    int m_iArmor;
    int m_iScore;
    int m_iSecondaryAmmo;
    bool m_bHasSecondWeapon;
};