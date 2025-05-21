#include "Player.h"
#include <algorithm>
#include <iostream>

Player::Player() {
    maxHealth = 100.0f;
    health = maxHealth;
    maxArmor = 30.0f;
    armor = 10.0f;
    maxStamina = 100.0f;
    stamina = maxStamina;
    position[0] = position[1] = 0.0f;
    velocity[0] = velocity[1] = 0.0f;
    currentWeaponIdx = 0;
    isAlive = true;
    isSprinting = false;
    isAiming = false;
    lightLevel = 1.0f;
    povAngle = 90.0f;
    kills = 0;
    solvedPuzzles = 0;
    level = 1;
    playTime = 0.0f;

    // Weapons: Pistol as default
    weapons = {
        {WeaponType::Pistol, 6, 6, 0, 0.7f, -10.0f, true}
    };

    // Inventory
    inventory["Medkit"] = { "Medkit", 1 };
    inventory["TerminalPassword"] = { "TerminalPassword", 1 };
    inventory["Battery"] = { "Battery", 0 };
}

void Player::Spawn() {
    health = maxHealth;
    armor = 10.0f; // Düþük zýrh
    stamina = maxStamina;
    position[0] = position[1] = 1.0f; // Spawn location
    isAlive = true;
    currentWeaponIdx = 0;
    // Reset other state as needed
}

void Player::Update(float deltaTime) {
    playTime += deltaTime;
    // Update stamina
    if (isSprinting) {
        UseStamina(20.0f * deltaTime); // arbitrary stamina drain rate
        if (stamina <= 0) isSprinting = false;
    }
    else {
        RestoreStamina(10.0f * deltaTime); // slow regen
    }
    // Update light/pov
    UpdateLight(deltaTime);
}

void Player::Move(float dx, float dy, float deltaTime, bool sprinting) {
    if (!isAlive) return;
    float speed = 3.0f; // base speed
    if (sprinting && stamina > 0) speed *= 2.0f;
    if (isAiming) speed *= 0.5f;
    position[0] += dx * speed * deltaTime;
    position[1] += dy * speed * deltaTime;
    // Clamp position as needed
}

void Player::Aim(bool aiming) {
    isAiming = aiming;
}

void Player::UseMedkit() {
    auto& medkit = inventory["Medkit"];
    if (medkit.amount > 0 && health < maxHealth) {
        health = std::min(maxHealth, health + 25.0f);
        medkit.amount--;
        std::cout << "Medkit used! Health: " << health << std::endl;
    }
}

void Player::Interact() {
    // Placeholder: Would check for interactables in radius, call appropriate handler
}

void Player::TakeDamage(float dmg) {
    if (armor > 0) {
        float absorbed = std::min(armor, dmg);
        armor -= absorbed;
        dmg -= absorbed;
    }
    health -= dmg;
    if (health <= 0 && isAlive) {
        isAlive = false;
        std::cout << "Player Died.\n";
    }
}

void Player::UseStamina(float amount) {
    stamina = std::max(0.0f, stamina - amount);
}

void Player::RestoreStamina(float amount) {
    stamina = std::min(maxStamina, stamina + amount);
}

void Player::FireWeapon() {
    if (!isAlive) return;
    Weapon& weapon = weapons[currentWeaponIdx];
    if (weapon.ammo > 0 || weapon.infiniteAmmo) {
        weapon.ammo -= weapon.infiniteAmmo ? 0 : 1;
        weapon.lastFiredTime = playTime;
        std::cout << "Fired weapon! Ammo left: " << weapon.ammo << std::endl;
        // TODO: Spawn bullet/projectile, play effects
    }
    else {
        std::cout << "No ammo! Switch to melee.\n";
        // TODO: Melee attack
    }
}

void Player::Reload() {
    Weapon& weapon = weapons[currentWeaponIdx];
    if (weapon.ammo < weapon.clipSize && weapon.ammoReserve > 0) {
        int needed = weapon.clipSize - weapon.ammo;
        int reloadAmount = std::min(needed, weapon.ammoReserve);
        weapon.ammo += reloadAmount;
        weapon.ammoReserve -= reloadAmount;
        // TODO: Apply reload delay
        std::cout << "Reloaded. Ammo: " << weapon.ammo << ", Reserve: " << weapon.ammoReserve << std::endl;
    }
}

void Player::SwitchWeapon(int idx) {
    if (idx >= 0 && idx < weapons.size()) {
        currentWeaponIdx = idx;
    }
}

void Player::PickUpItem(const std::string& itemName, int amount) {
    inventory[itemName].name = itemName;
    inventory[itemName].amount += amount;
    std::cout << "Picked up " << itemName << " x" << amount << std::endl;
}

void Player::OnLevelUp() {
    level++;
    // Reset stats, or increase difficulty
    Spawn();
}

void Player::AddScoreKill() {
    kills++;
}

void Player::AddScorePuzzle() {
    solvedPuzzles++;
}

float Player::CalculateScore() {
    float levelMultiplier = 1.0f + (level - 1) * 0.2f;
    float playTimeMin = playTime / 60.0f;
    if (playTimeMin < 1.0f) playTimeMin = 1.0f;
    return ((kills * 10 + solvedPuzzles * 25) / playTimeMin) * levelMultiplier;
}

void Player::UpdateLight(float deltaTime) {
    // Light drops over time
    lightLevel -= 0.01f * deltaTime; // 90s to zero
    lightLevel = std::max(0.0f, lightLevel);
    // POV narrows as light decreases
    povAngle = 60.0f + 30.0f * lightLevel;
}