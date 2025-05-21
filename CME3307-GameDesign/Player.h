#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class WeaponType { Pistol, Shotgun, AK, Knife, Pipe, LavaGun };

struct Weapon {
    WeaponType type;
    int ammo;
    int clipSize;
    int ammoReserve;
    float cooldown;
    float lastFiredTime;
    bool infiniteAmmo;
};

struct InventoryItem {
    std::string name;
    int amount;
};

class Player {
public:
    // Core stats
    float health;
    float maxHealth;
    float armor;
    float maxArmor;
    float stamina;
    float maxStamina;
    float position[2]; // x, y in the maze
    float velocity[2]; // x, y

    // Inventory
    std::unordered_map<std::string, InventoryItem> inventory;
    std::vector<Weapon> weapons;
    int currentWeaponIdx;

    // Light/POV
    float lightLevel; // 0-1, 1 is max
    float povAngle; // in degrees

    // State
    bool isAlive;
    bool isSprinting;
    bool isAiming;

    // Score
    int kills;
    int solvedPuzzles;
    int level;
    float playTime; // seconds

    Player();

    void Spawn();
    void Update(float deltaTime);
    void Move(float dx, float dy, float deltaTime, bool sprinting);
    void Aim(bool aiming);
    void UseMedkit();
    void Interact();
    void TakeDamage(float dmg);
    void UseStamina(float amount);
    void RestoreStamina(float amount);
    void FireWeapon();
    void Reload();
    void SwitchWeapon(int idx);
    void PickUpItem(const std::string& itemName, int amount);
    void OnLevelUp();
    void AddScoreKill();
    void AddScorePuzzle();
    float CalculateScore();
    void UpdateLight(float deltaTime);
};