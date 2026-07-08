#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>
#include <math.h>
#include <cmath>
using namespace std;

#ifndef RGBA
#define RGBA(r,g,b,a) ((COLORREF)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)|(((DWORD)(BYTE)(a))<<24)))
#endif

#pragma region Definitions & Constants
#define TILE_SIZE     32
#define MAP_WIDTH     40
#define MAP_HEIGHT    23
#define SCREEN_WIDTH  (MAP_WIDTH * TILE_SIZE)
#define SCREEN_HEIGHT (MAP_HEIGHT * TILE_SIZE + 50)

#define MAX_ENEMIES   20
#define BACKPACK_SIZE 9
#define MAX_SHOP_ITEMS 4
#define MAX_LOG       10         // 战斗日志条目数
#define CELL_SIZE     60
#define CELL_GAP      5

#define MAX_ROOMS     8
#define MIN_ROOM_SIZE 4
#define MAX_ROOM_SIZE 8
#define MAX_GEN_ATTEMPTS 50

#define COL_BG          RGB(5, 5, 8)
#define COL_DARK_WALL   RGB(25, 20, 22)
#define COL_DARK_FLOOR  RGB(35, 30, 32)
#define COL_PILLAR      RGB(38, 32, 36)
#define COL_STAIRS      RGB(90, 75, 40)
#define COL_PLAYER      RGB(0, 180, 255)
#define COL_ENEMY       RGB(160, 35, 35)
#define COL_BOSS        RGB(200, 50, 45)
#define COL_UI_BG       RGBA(8, 8, 12, 230)
#define COL_HOVER       RGB(50, 50, 65)
#define COL_BTN         RGB(30, 30, 40)
#define COL_BTN_HOVER   RGB(60, 60, 80)

#define KEY_COOLDOWN_FRAMES 4
#define COMBAT_KEY_COOLDOWN 1

bool explored[MAP_HEIGHT][MAP_WIDTH];
const int VIEW_RADIUS = 6;

#define PLAYER_AVATAR_TYPE 7
const int MAX_COMBAT_LOG = 10;
int logCount = 0;                           // 当前日志数量
#pragma endregion

#pragma region Enums & Structs
enum TileType { TILE_WALL, TILE_FLOOR, TILE_STAIRS, TILE_PILLAR };
enum ItemType { ITEM_POTION, ITEM_WEAPON, ITEM_ARMOR, ITEM_BUFF_POTION };
enum GameStateType { STATE_EXPLORE, STATE_COMBAT, STATE_BACKPACK, STATE_PAUSE, STATE_GAMEOVER };

char g_skillNotice[64] = "";
COLORREF g_skillNoticeColor = RGB(255, 255, 255);
int g_skillNoticeTimer = 0;

struct Player {
    int x, y, hp, maxHp, baseAtk, atk, def, gold, floor;
    int tempAtkBonus, tempAtkDuration;
    int resistance;
    int bleedTurns;
    int webbedTurns;
    int atkDebuffTurns;
} player;

struct Enemy {
    int x, y, hp, maxHp, atk, def;
    bool alive, isBoss;
    char name[32];
    int avatarType;
    int skillCooldown;
} enemies[MAX_ENEMIES];
int enemyCount = 0;

struct Item { int type, value, durability; bool active; char name[32]; };
struct Weapon { char name[32]; int atkBonus, durability; bool isValid; } g_equippedWeapon;
struct Backpack { Item slots[BACKPACK_SIZE]; int count; } g_backpack;
struct ShopItem { char name[32]; int price, type, value; bool isWeapon; } g_shopItems[MAX_SHOP_ITEMS];
int g_shopItemCount = 0;

// 战斗日志结构体数组（统一使用）
struct CombatLog { char text[64]; } combatLogs[MAX_LOG];


struct DamageText { int x, y, val, life; COLORREF color; } dmgTexts[30];
int dmgTextCount = 0;

struct Room { int x, y, w, h; int centerX() const { return x + w / 2; } int centerY() const { return y + h / 2; } } g_rooms[MAX_ROOMS];
int g_roomCount = 0;

int map[MAP_HEIGHT][MAP_WIDTH];
GameStateType gameState = STATE_EXPLORE;
bool inFight = false;
int fightEnemyIndex = -1;
int mouseX = 0, mouseY = 0;
int selectedSlot = -1;
bool showActionMenu = false;
int actionMenuX = 0, actionMenuY = 0;
char g_messageText[64] = "";
COLORREF g_messageColor = WHITE;
int g_messageTimer = 0;
int g_keyCooldown[256] = { 0 };

int animPhase = 0;
int animTimer = 0;
int animOffsetX = 0;
int animOffsetY = 0;
bool showShield = false;
#pragma endregion

#pragma region Function Forward Declarations
//void drawCombatAssassin3D(int cx, int cy, float scale = 2.5f);
int calcDamage(int atk, int def, bool ignoreHalfDef = false);
void applyWeaponStats();
void decrementBuffDuration();
void initGame();
void generateLevel();
void spawnEnemies();
void handleKeyboard();
void handleMouseInput();
int getSlotFromMouse(int mx, int my);
int getShopItemFromMouse(int mx, int my);
void playerAttack();
void enemyAttack(bool isDefending);
void playerDefend();
void tryRun();
void addItemToBackpack(Item item);
void useItem(int slotIdx);
void equipWeapon(int slotIdx);
void sellItem(int slotIdx);
void buyItem(int shopIdx);
void initShop();
void showMessage(const char* msg, COLORREF col);
void addDmgText(int x, int y, int val, COLORREF color);
void addCombatLog(const char* fmt, ...);
void drawDamageTexts();
void drawMessage();
void drawCombatLog();
void drawMap();
void drawPlayer();
void drawEnemies();
void drawUI();
void drawBackpackAndShop();
bool checkKey(int vk);
void updateExplored();
void drawMiniMap();
float getBrightness(int x, int y);
void drawCombatScene();
void drawCharacter(int x, int y, const char* name, int level, int hp, int maxHp, COLORREF bodyColor, bool isEnemy, int avatarType = -1);
void drawHealthBarEx(int x, int y, int w, int h, int cur, int max, COLORREF color);
void triggerAttackAnim(bool isPlayer);
void triggerDefendAnim();
void triggerRunAnim();
void enemyTurn(bool isDefending);   // 前置声明

// 头像绘制函数
void drawAvatar(int offX, int offY, const int data[40][40], const COLORREF palette[], int palSize);
void drawSkeleton(int offX, int offY);
void drawGoblin(int offX, int offY);
void drawShadowWolf(int offX, int offY);
void drawZombie(int offX, int offY);
void drawGargoyle(int offX, int offY);
void drawCryptSpider(int offX, int offY);
void drawNightAssassin(int offX, int offY);
void drawPixelAssassin3D(int ox, int oy);
void drawAvatarScaled(int type, int cx, int cy, int size);
#pragma endregion

#pragma region Helpers & Core Logic
void triggerSkillNotice(const char* text, COLORREF color) {
    strcpy_s(g_skillNotice, text);
    g_skillNoticeColor = color;
    g_skillNoticeTimer = 45;
}
bool checkKey(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

int calcDamage(int atk, int def, bool ignoreHalfDef) {
    int effDef = ignoreHalfDef ? (def / 2) : def;
    return max(1, atk - effDef);
}

void applyWeaponStats() {
    player.atk = player.baseAtk + (g_equippedWeapon.isValid ? g_equippedWeapon.atkBonus : 0) + player.tempAtkBonus;
}

void decrementBuffDuration() {
    if (player.tempAtkDuration > 0) {
        player.tempAtkDuration--;
        if (player.tempAtkDuration == 0) {
            player.tempAtkBonus = 0;
            showMessage("力量药剂效果已消失", RGB(255, 200, 100));
            applyWeaponStats();
        }
    }
}

void showMessage(const char* msg, COLORREF col) {
    strcpy_s(g_messageText, msg);
    g_messageColor = col;
    g_messageTimer = 120;
}

void addDmgText(int x, int y, int val, COLORREF color) {
    if (dmgTextCount >= 30) return;
    dmgTexts[dmgTextCount++] = { x * TILE_SIZE + TILE_SIZE / 2, y * TILE_SIZE, val, 40, color };
}

// 修正后的日志函数：使用 combatLogs 结构体数组
void addCombatLog(const char* fmt, ...) {
    for (int i = MAX_LOG - 1; i > 0; i--) {
        strcpy_s(combatLogs[i].text, combatLogs[i - 1].text);
    }
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    strcpy_s(combatLogs[0].text, buffer);
    if (logCount < MAX_LOG) logCount++;
}

void addItemToBackpack(Item item) {
    if (g_backpack.count >= BACKPACK_SIZE) {
        showMessage("背包已满！", RGB(255, 100, 100));
        return;
    }
    for (int i = 0; i < BACKPACK_SIZE; i++) {
        if (!g_backpack.slots[i].active) {
            g_backpack.slots[i] = item;
            g_backpack.count++;
            return;
        }
    }
}

bool checkResistance() { return (rand() % 100 < player.resistance); }
void triggerScreenShake() { animPhase = 6; animTimer = 0; }
void triggerBleedVignette() { animPhase = 5; animTimer = 0; }

void processPlayerDebuffs() {
    if (player.bleedTurns > 0) {
        int bleedDmg = 2 + player.floor;
        player.hp -= bleedDmg;
        addDmgText(player.x, player.y, bleedDmg, RGB(200, 0, 0));
        addCombatLog("流血让你受到 %d 点伤害！", bleedDmg);
        player.bleedTurns--;
        triggerBleedVignette();
        if (player.hp <= 0) {
            player.hp = 0;
            gameState = STATE_GAMEOVER;
            inFight = false;
        }
    }
    if (player.atkDebuffTurns > 0) {
        player.atkDebuffTurns--;
        if (player.atkDebuffTurns == 0) addCombatLog("尸毒效果已消退。");
    }
    if (player.webbedTurns > 0) player.webbedTurns--;
}
#pragma endregion

#pragma region Map & Enemy Generation
bool roomsOverlap(const Room& a, const Room& b) {
    return !(a.x + a.w + 1 < b.x || b.x + b.w + 1 < a.x || a.y + a.h + 1 < b.y || b.y + b.h + 1 < a.y);
}

void carveRoom(const Room& r) {
    for (int y = r.y; y < r.y + r.h; y++)
        for (int x = r.x; x < r.x + r.w; x++)
            if (y > 0 && y < MAP_HEIGHT - 1 && x > 0 && x < MAP_WIDTH - 1)
                map[y][x] = TILE_FLOOR;
}

void carveCorridor(int x1, int y1, int x2, int y2) {
    int x = x1, y = y1;
    while (x != x2) {
        if (y > 0 && y < MAP_HEIGHT - 1 && x > 0 && x < MAP_WIDTH - 1)
            map[y][x] = TILE_FLOOR;
        x += (x2 > x1) ? 1 : -1;
    }
    while (y != y2) {
        if (y > 0 && y < MAP_HEIGHT - 1 && x > 0 && x < MAP_WIDTH - 1)
            map[y][x] = TILE_FLOOR;
        y += (y2 > y1) ? 1 : -1;
    }
}

bool tryGenerateRoom() {
    int w = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
    int h = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
    int x = 1 + rand() % (MAP_WIDTH - w - 2);
    int y = 1 + rand() % (MAP_HEIGHT - h - 2);
    Room nr = { x, y, w, h };
    for (int i = 0; i < g_roomCount; i++)
        if (roomsOverlap(nr, g_rooms[i])) return false;
    g_rooms[g_roomCount++] = nr;
    carveRoom(nr);
    return true;
}

void connectRooms() {
    for (int i = 1; i < g_roomCount; i++)
        carveCorridor(g_rooms[i - 1].centerX(), g_rooms[i - 1].centerY(),
            g_rooms[i].centerX(), g_rooms[i].centerY());
    for (int i = 0; i < 2; i++) {
        int a = rand() % g_roomCount, b = rand() % g_roomCount;
        if (a != b)
            carveCorridor(g_rooms[a].centerX(), g_rooms[a].centerY(),
                g_rooms[b].centerX(), g_rooms[b].centerY());
    }
}

void addDecorations() {
    for (int k = 0; k < 30; k++) {
        int x = 2 + rand() % (MAP_WIDTH - 4);
        int y = 2 + rand() % (MAP_HEIGHT - 4);
        if (map[y][x] == TILE_FLOOR) {
            int n = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (map[y + dy][x + dx] == TILE_FLOOR) n++;
            if (n >= 5 && n <= 7) map[y][x] = TILE_PILLAR;
        }
    }
}

int findFarthestRoom() {
    int f = 0, md = 0;
    for (int i = 1; i < g_roomCount; i++) {
        int d = abs(g_rooms[i].centerX() - g_rooms[0].centerX()) +
            abs(g_rooms[i].centerY() - g_rooms[0].centerY());
        if (d > md) { md = d; f = i; }
    }
    return f;
}

int findLargestRoom() {
    int l = 0, ma = 0;
    for (int i = 0; i < g_roomCount; i++) {
        int a = g_rooms[i].w * g_rooms[i].h;
        if (a > ma) { ma = a; l = i; }
    }
    return l;
}

const char* enemyNames[] = { "骷髅", "哥布林", "暗影狼", "僵尸", "石像鬼", "地穴蜘蛛", "暗夜刺客" };

bool placeEnemyInRoom(int roomIdx, Enemy& e) {
    Room& r = g_rooms[roomIdx];
    for (int k = 0; k < 20; k++) {
        int ex = r.x + 1 + rand() % (r.w - 2);
        int ey = r.y + 1 + rand() % (r.h - 2);
        if (map[ey][ex] == TILE_FLOOR && !(ex == player.x && ey == player.y)) {
            bool ov = false;
            for (int i = 0; i < enemyCount; i++)
                if (enemies[i].alive && enemies[i].x == ex && enemies[i].y == ey) {
                    ov = true;
                    break;
                }
            if (!ov) {
                e.x = ex;
                e.y = ey;
                e.alive = true;
                return true;
            }
        }
    }
    return false;
}

void spawnEnemies() {
    enemyCount = 0;
    memset(enemies, 0, sizeof(enemies));

    int minC = player.floor + 2, maxC = player.floor * 2 + 3;
    int count = minC + rand() % (maxC - minC + 1);
    if (count >= MAX_ENEMIES) count = MAX_ENEMIES - 1;

    bool spawnBoss = (player.floor % 3 == 0 && player.floor > 0);
    if (spawnBoss) count--;

    for (int i = 0; i < count && enemyCount < MAX_ENEMIES; i++) {
        int ri = rand() % g_roomCount;
        if (ri == 0 && g_roomCount > 1) ri = 1 + rand() % (g_roomCount - 1);

        Enemy& e = enemies[enemyCount];
        if (placeEnemyInRoom(ri, e)) {
            int typeIdx = rand() % 7;
            strcpy_s(e.name, enemyNames[typeIdx]);
            e.avatarType = typeIdx;
            e.hp = e.maxHp = 20 + player.floor * 5;
            e.atk = 5 + player.floor * 2;
            e.def = 1 + player.floor / 2;
            e.isBoss = false;
            e.skillCooldown = 0;
            enemyCount++;
        }
    }

    if (spawnBoss && enemyCount < MAX_ENEMIES) {
        Enemy& b = enemies[enemyCount];
        if (placeEnemyInRoom(findLargestRoom(), b)) {
            int typeIdx = rand() % 7;
            sprintf_s(b.name, "Boss: %s领主", enemyNames[typeIdx]);
            b.avatarType = typeIdx;
            b.hp = b.maxHp = 50 + player.floor * 10;
            b.atk = 15 + player.floor * 3;
            b.def = 5 + player.floor;
            b.isBoss = true;
            b.skillCooldown = 0;
            enemyCount++;
        }
    }
}

void generateLevel() {
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            map[y][x] = TILE_WALL;

    g_roomCount = 0;
    int target = 5 + rand() % 4;
    int attempts = 0;

    while (g_roomCount < target && attempts < MAX_GEN_ATTEMPTS) {
        if (tryGenerateRoom()) {}
        attempts++;
    }

    if (g_roomCount < 2) {
        generateLevel();
        return;
    }

    connectRooms();
    addDecorations();

    player.x = g_rooms[0].centerX();
    player.y = g_rooms[0].centerY();

    int stairRoom = findFarthestRoom();
    map[g_rooms[stairRoom].centerY()][g_rooms[stairRoom].centerX()] = TILE_STAIRS;

    spawnEnemies();
    inFight = false;
    fightEnemyIndex = -1;
    showActionMenu = false;

    memset(explored, 0, sizeof(explored));
    updateExplored();
}
#pragma endregion

#pragma region Combat, Backpack & Shop
void enemyAttack(bool isDefending) {
    if (fightEnemyIndex < 0 || !enemies[fightEnemyIndex].alive) return;
    triggerAttackAnim(false);
    Enemy& e = enemies[fightEnemyIndex];
    int dmg = calcDamage(e.atk, player.def, isDefending);
    player.hp -= dmg;
    addDmgText(player.x, player.y, dmg, RGB(255, 80, 80));
    addCombatLog("%s 攻击你，造成 %d 点伤害", e.name, dmg);
    if (player.hp <= 0) {
        player.hp = 0;
        gameState = STATE_GAMEOVER;
        inFight = false;
    }
}

// 修复后的 enemyTurn 函数
void enemyTurn(bool isDefending) {
    if (fightEnemyIndex < 0 || !enemies[fightEnemyIndex].alive) return;
    Enemy& e = enemies[fightEnemyIndex];

    if (e.skillCooldown > 0) e.skillCooldown--;

    bool canUseSkill = (e.skillCooldown <= 0);
    bool usedSkill = false;

    // Boss 技能
    if (e.isBoss && canUseSkill && rand() % 100 < 60) {
        addCombatLog("   %s 释放【深渊重击】！造成 %d 穿透伤害！", e.name, e.maxHp / 2);
        triggerScreenShake();
        triggerAttackAnim(false);
        int damage = e.maxHp / 2;
        player.hp -= damage;
        addDmgText(player.x * TILE_SIZE + TILE_SIZE / 2, player.y * TILE_SIZE + TILE_SIZE / 2, damage, RGB(255, 0, 0));
        e.skillCooldown = 15;
        usedSkill = true;
    }
    // 普通怪物技能
    else if (!e.isBoss && canUseSkill) {
        int typeIdx = e.avatarType;
        int skillChance = 40;
        if (rand() % 100 < skillChance) {
            switch (typeIdx) {
            case 1: { // 哥布林 - 卑鄙偷窃
                int goldStolen = max(1, player.gold / 4);
                addCombatLog(" %s 释放【卑鄙偷窃】！偷走 %d 金币！", e.name, goldStolen);
                player.gold = max(0, player.gold - goldStolen);
                e.skillCooldown = 8;
                usedSkill = true;  //  已有
                break;
            }
            case 2: { // 暗影狼 - 撕裂
                triggerAttackAnim(false);
                int dmg = calcDamage((int)(e.atk * 0.8), player.def, isDefending);
                player.hp -= dmg;
                addDmgText(player.x, player.y, dmg, RGB(255, 80, 80));
                addCombatLog("%s 释放【撕裂】！造成 %d 伤害并附加流血效果！", e.name, dmg);
                player.bleedTurns = 3;
                triggerBleedVignette();
                e.skillCooldown = 6;
                usedSkill = true;  //  已有
                break;
            }
            case 3: // 僵尸 - 尸毒（攻击削弱）
                triggerAttackAnim(false);
                {
                    int dmg = calcDamage((int)(e.atk * 0.5), player.def, isDefending);
                    player.hp -= dmg;
                    addDmgText(player.x, player.y, dmg, RGB(100, 255, 100));
                    if (!checkResistance()) {
                        player.atkDebuffTurns = 3;
                        addCombatLog("%s【尸毒】造成 %d 伤害并让你攻击力下降！", e.name, dmg);
                    }
                    else {
                        addCombatLog("%s【尸毒】造成 %d 伤害但被你抵抗了！", e.name, dmg);
                    }
                }
                e.skillCooldown = 4;
                usedSkill = true;  //  补充
                break;

            case 4: // 石像鬼 - 岩石护甲（不攻击，仅加防）
            {
                int buff = 2 + player.floor / 2;
                e.def += buff;
                addCombatLog("%s【岩石护甲】防御提升 %d！", e.name, buff);
                e.skillCooldown = 99;
                usedSkill = true;  //  改用 usedSkill = true 代替 return
                break;
            }

            case 5: // 墓穴蜘蛛 - 蛛网缠绕（无法防御）
                triggerAttackAnim(false);
                {
                    int dmg = calcDamage((int)(e.atk * 0.8), player.def, isDefending);
                    player.hp -= dmg;
                    addDmgText(player.x, player.y, dmg, RGB(200, 200, 255));
                    if (!checkResistance()) {
                        player.webbedTurns = 2;
                        addCombatLog("%s【蛛网缠绕】造成 %d 伤害并让你无法防御！", e.name, dmg);
                    }
                    else {
                        addCombatLog("%s【蛛网缠绕】造成 %d 伤害但被你挣脱！", e.name, dmg);
                    }
                }
                e.skillCooldown = 3;
                usedSkill = true;  //  补充
                break;

            case 6: // 暗夜刺客 - 暗影背刺（防御时伤害翻倍）
                triggerAttackAnim(false);
                {
                    int baseDmg = e.atk;
                    if (isDefending) baseDmg *= 2;
                    int dmg = calcDamage(baseDmg, player.def, isDefending);
                    player.hp -= dmg;
                    addDmgText(player.x, player.y, dmg, RGB(150, 50, 255));
                    if (isDefending)
                        addCombatLog("%s【暗影背刺】完美克制防御！造成 %d 伤害！", e.name, dmg);
                    else
                        addCombatLog("%s【暗影背刺】造成 %d 伤害！", e.name, dmg);
                }
                e.skillCooldown = 3;
                usedSkill = true;  //  补充
                break;

            default: // 兜底：普通攻击
                enemyAttack(isDefending);
                usedSkill = true;  //  补充，防止重复攻击
                break;
            }
        }
    }


    // 普通攻击
    if (!usedSkill) {
        int baseDamage = calcDamage(e.atk, player.def, isDefending);
        //printf("Enemy atk: %d, Player def: %d, Damage: %d\n", e.atk, player.def, baseDamage);
        if (isDefending) {
            baseDamage = max(1, baseDamage / 2);
            addCombatLog("你成功防御！%s 攻击造成 %d 点伤害！", e.name, baseDamage);
        }
        else {
            addCombatLog("%s 攻击了你，造成 %d 点伤害！", e.name, baseDamage);
        }
        player.hp -= baseDamage;
        addDmgText(player.x * TILE_SIZE + TILE_SIZE / 2, player.y * TILE_SIZE + TILE_SIZE / 2, baseDamage, RGB(255, 80, 80));
        triggerAttackAnim(false);
    }

    if (player.hp <= 0) {
        player.hp = 0;
        gameState = STATE_GAMEOVER;
        inFight = false;
        addCombatLog("   你被 %s 击败了...", e.name);
    }
}

void playerAttack() {
    if (fightEnemyIndex < 0 || !enemies[fightEnemyIndex].alive) return;
    triggerAttackAnim(true);
    Enemy& e = enemies[fightEnemyIndex];
    int effAtk = player.atk;
    if (player.atkDebuffTurns > 0) effAtk = max(1, effAtk / 2);
    int dmg = calcDamage(effAtk, e.def);
    e.hp -= dmg;
    addDmgText(e.x, e.y, dmg, RGB(255, 200, 0));
    addCombatLog("你攻击 %s，造成 %d 点伤害", e.name, dmg);
    if (g_equippedWeapon.isValid && g_equippedWeapon.durability > 0) {
        g_equippedWeapon.durability--;
        if (g_equippedWeapon.durability <= 0) {
            showMessage("武器损坏！", RGB(255, 100, 100));
            g_equippedWeapon.isValid = false;
            applyWeaponStats();
        }
    }
    if (e.hp <= 0) {
        e.alive = false;
        inFight = false;
        gameState = STATE_EXPLORE;
        int gold = 5 + rand() % 16;
        if (e.isBoss) gold *= 4;
        player.gold += gold;
        addCombatLog("击败 %s！获得 %d 金币", e.name, gold);
        showMessage("战斗胜利！", RGB(100, 255, 100));
    }
    if (inFight) enemyTurn(false);
    decrementBuffDuration();
    if (inFight) processPlayerDebuffs();
}

void playerDefend() {
    if (player.webbedTurns > 0) {
        addCombatLog("你被蛛网缠绕，无法防御！");
        showMessage("无法防御！", RGB(255, 100, 100));
        enemyTurn(false);
        processPlayerDebuffs();
        return;
    }
    triggerDefendAnim();
    enemyTurn(true);
    addCombatLog("你采取防御姿态！");
    decrementBuffDuration();
    if (inFight) processPlayerDebuffs();
}

void tryRun() {
    triggerRunAnim();
    int runChance = (player.webbedTurns > 0) ? 20 : 60;
    if (rand() % 100 < runChance) {
        inFight = false;
        gameState = STATE_EXPLORE;
        addCombatLog("成功逃跑！");
    }
    else {
        addCombatLog("逃跑失败！");
        enemyTurn(false);
        if (inFight) processPlayerDebuffs();
    }
}

void equipWeapon(int slotIdx) {
    if (slotIdx < 0 || slotIdx >= BACKPACK_SIZE) return;
    Item& bp = g_backpack.slots[slotIdx];
    if (!bp.active || bp.type != ITEM_WEAPON) return;
    if (g_equippedWeapon.isValid) {
        Item old = { ITEM_WEAPON, g_equippedWeapon.atkBonus, g_equippedWeapon.durability, true, "" };
        strcpy_s(old.name, g_equippedWeapon.name);
        addItemToBackpack(old);
    }
    strcpy_s(g_equippedWeapon.name, bp.name);
    g_equippedWeapon.atkBonus = bp.value;
    g_equippedWeapon.durability = bp.durability;
    g_equippedWeapon.isValid = true;
    applyWeaponStats();
    char buf[64];
    sprintf_s(buf, "装备 %s，攻击+%d", bp.name, bp.value);
    showMessage(buf, RGB(100, 200, 255));
    bp.active = false;
    g_backpack.count--;
    showActionMenu = false;
}

void useItem(int slotIdx) {
    if (slotIdx < 0 || slotIdx >= BACKPACK_SIZE) return;
    Item& it = g_backpack.slots[slotIdx];
    if (!it.active) return;
    if (it.type == ITEM_POTION) {
        int h = it.value;
        player.hp = min(player.hp + h, player.maxHp);
        char b[32];
        sprintf_s(b, "使用 %s，恢复 %d HP", it.name, h);
        showMessage(b, RGB(100, 255, 100));
        addDmgText(player.x, player.y, -h, RGB(100, 255, 100));
        it.active = false;
        g_backpack.count--;
    }
    else if (it.type == ITEM_ARMOR) {
        player.def += it.value;
        player.resistance += it.value * 2;
        if (player.resistance > 80) player.resistance = 80;
        char b[32];
        sprintf_s(b, "穿戴 %s，防御+%d，抗性+%d", it.name, it.value, it.value * 2);
        showMessage(b, RGB(100, 200, 255));
        it.active = false;
        g_backpack.count--;
    }
    else if (it.type == ITEM_BUFF_POTION) {
        player.tempAtkBonus += it.value;
        player.tempAtkDuration = 3;
        applyWeaponStats();
        char buf[32];
        sprintf_s(buf, "攻击力 +%d，持续3回合", it.value);
        showMessage(buf, RGB(255, 200, 100));
        it.active = false;
        g_backpack.count--;
    }
    showActionMenu = false;
}

void sellItem(int slotIdx) {
    if (slotIdx < 0 || slotIdx >= BACKPACK_SIZE) return;
    Item& it = g_backpack.slots[slotIdx];
    if (!it.active) return;
    int p = (it.type == ITEM_WEAPON) ? it.value * 10 : it.value * 5;
    player.gold += p;
    char b[32];
    sprintf_s(b, "出售 %s，获得 %d 金币", it.name, p);
    showMessage(b, RGB(255, 215, 0));
    it.active = false;
    g_backpack.count--;
    showActionMenu = false;
}

void buyItem(int shopIdx) {
    if (shopIdx < 0 || shopIdx >= g_shopItemCount) return;
    ShopItem& s = g_shopItems[shopIdx];
    if (player.gold < s.price) {
        showMessage("金币不足！", RGB(255, 100, 100));
        return;
    }
    if (g_backpack.count >= BACKPACK_SIZE) {
        showMessage("背包已满！", RGB(255, 100, 100));
        return;
    }
    player.gold -= s.price;
    Item ni = { s.type, s.value, (s.type == ITEM_WEAPON) ? 20 : 0, true, "" };
    strcpy_s(ni.name, s.name);
    addItemToBackpack(ni);
    char b[32];
    sprintf_s(b, "购买 %s 成功！", s.name);
    showMessage(b, RGB(100, 255, 100));
}

void initShop() {
    g_shopItemCount = 4;
    strcpy_s(g_shopItems[0].name, "治疗药水");
    g_shopItems[0].price = 50;
    g_shopItems[0].type = ITEM_POTION;
    g_shopItems[0].value = 20;
    g_shopItems[0].isWeapon = false;

    strcpy_s(g_shopItems[1].name, "铁剑");
    g_shopItems[1].price = 120;
    g_shopItems[1].type = ITEM_WEAPON;
    g_shopItems[1].value = 3;
    g_shopItems[1].isWeapon = true;

    strcpy_s(g_shopItems[2].name, "护甲片");
    g_shopItems[2].price = 80;
    g_shopItems[2].type = ITEM_ARMOR;
    g_shopItems[2].value = 2;
    g_shopItems[2].isWeapon = false;

    strcpy_s(g_shopItems[3].name, "力量药剂");
    g_shopItems[3].price = 90;
    g_shopItems[3].type = ITEM_BUFF_POTION;
    g_shopItems[3].value = 5;
    g_shopItems[3].isWeapon = false;
}
#pragma endregion

#pragma region Input Handling
int getSlotFromMouse(int mx, int my) {
    int sx = 60 + 15, sy = 90 + 40, step = CELL_SIZE + CELL_GAP;
    int c = (mx - sx) / step, r = (my - sy) / step;
    if (c >= 0 && c < 3 && r >= 0 && r < 3) {
        int cx = sx + c * step, cy = sy + r * step;
        if (mx >= cx && mx <= cx + CELL_SIZE && my >= cy && my <= cy + CELL_SIZE)
            return r * 3 + c;
    }
    return -1;
}

int getShopItemFromMouse(int mx, int my) {
    int bpW = 3 * (CELL_SIZE + CELL_GAP) - CELL_GAP + 30;
    int sx = 60 + bpW + 40 + 15, sy = 90 + 40, step = CELL_SIZE + CELL_GAP;
    int c = (mx - sx) / step, r = (my - sy) / step;
    if (c >= 0 && c < 2 && r >= 0 && r < 4) {
        int idx = r * 2 + c;
        if (idx >= g_shopItemCount) return -1;
        int cx = sx + c * step, cy = sy + r * step;
        if (mx >= cx && mx <= cx + CELL_SIZE && my >= cy && my <= cy + CELL_SIZE)
            return idx;
    }
    return -1;
}

void handleKeyboard() {
    if (gameState == STATE_EXPLORE) {
        int dx = 0, dy = 0;
        if ((checkKey('W') || checkKey(VK_UP)) && g_keyCooldown['W'] == 0 && g_keyCooldown[VK_UP] == 0) {
            dy = -1;
            g_keyCooldown['W'] = KEY_COOLDOWN_FRAMES;
            g_keyCooldown[VK_UP] = KEY_COOLDOWN_FRAMES;
        }
        if ((checkKey('S') || checkKey(VK_DOWN)) && g_keyCooldown['S'] == 0 && g_keyCooldown[VK_DOWN] == 0) {
            dy = 1;
            g_keyCooldown['S'] = KEY_COOLDOWN_FRAMES;
            g_keyCooldown[VK_DOWN] = KEY_COOLDOWN_FRAMES;
        }
        if ((checkKey('A') || checkKey(VK_LEFT)) && g_keyCooldown['A'] == 0 && g_keyCooldown[VK_LEFT] == 0) {
            dx = -1;
            g_keyCooldown['A'] = KEY_COOLDOWN_FRAMES;
            g_keyCooldown[VK_LEFT] = KEY_COOLDOWN_FRAMES;
        }
        if ((checkKey('D') || checkKey(VK_RIGHT)) && g_keyCooldown['D'] == 0 && g_keyCooldown[VK_RIGHT] == 0) {
            dx = 1;
            g_keyCooldown['D'] = KEY_COOLDOWN_FRAMES;
            g_keyCooldown[VK_RIGHT] = KEY_COOLDOWN_FRAMES;
        }
        if (checkKey('B') && g_keyCooldown['B'] == 0) {
            gameState = STATE_BACKPACK;
            g_keyCooldown['B'] = KEY_COOLDOWN_FRAMES;
            return;
        }
        if (checkKey('P') && g_keyCooldown['P'] == 0) {
            gameState = STATE_PAUSE;
            g_keyCooldown['P'] = KEY_COOLDOWN_FRAMES;
            return;
        }
        if (dx || dy) {
            int nx = player.x + dx, ny = player.y + dy;
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT && map[ny][nx] != TILE_WALL) {
                bool hit = false;
                for (int i = 0; i < MAX_ENEMIES; i++)
                    if (enemies[i].alive && enemies[i].x == nx && enemies[i].y == ny) {
                        fightEnemyIndex = i;
                        inFight = true;
                        gameState = STATE_COMBAT;
                        logCount = 0;  // 重置日志
                        hit = true;
                        player.bleedTurns = 0;
                        player.webbedTurns = 0;
                        player.atkDebuffTurns = 0;
                        break;
                    }
                if (!hit) {
                    player.x = nx;
                    player.y = ny;
                    updateExplored();
                    if (map[ny][nx] == TILE_STAIRS) {
                        player.floor++;
                        player.def++;
                        showMessage("进入下一层...", RGB(100, 255, 100));
                        generateLevel();
                    }
                }
            }
        }
    }
    else if (gameState == STATE_BACKPACK) {
        if ((checkKey(VK_ESCAPE) || checkKey('B')) && g_keyCooldown[VK_ESCAPE] == 0 && g_keyCooldown['B'] == 0) {
            gameState = STATE_EXPLORE;
            showActionMenu = false;
            g_keyCooldown[VK_ESCAPE] = KEY_COOLDOWN_FRAMES;
            g_keyCooldown['B'] = KEY_COOLDOWN_FRAMES;
        }
    }
    else if (gameState == STATE_COMBAT && inFight) {
        if ((checkKey('1') || checkKey(VK_NUMPAD1)) && g_keyCooldown['1'] == 0 && g_keyCooldown[VK_NUMPAD1] == 0) {
            playerAttack();
            g_keyCooldown['1'] = COMBAT_KEY_COOLDOWN;
            g_keyCooldown[VK_NUMPAD1] = COMBAT_KEY_COOLDOWN;
        }
        if ((checkKey('2') || checkKey(VK_NUMPAD2)) && g_keyCooldown['2'] == 0 && g_keyCooldown[VK_NUMPAD2] == 0) {
            playerDefend();
            g_keyCooldown['2'] = COMBAT_KEY_COOLDOWN;
            g_keyCooldown[VK_NUMPAD2] = COMBAT_KEY_COOLDOWN;
        }
        if ((checkKey('3') || checkKey(VK_NUMPAD3)) && g_keyCooldown['3'] == 0 && g_keyCooldown[VK_NUMPAD3] == 0) {
            tryRun();
            g_keyCooldown['3'] = COMBAT_KEY_COOLDOWN;
            g_keyCooldown[VK_NUMPAD3] = COMBAT_KEY_COOLDOWN;
        }
    }
    else if (gameState == STATE_PAUSE || gameState == STATE_GAMEOVER) {
        if ((checkKey(VK_ESCAPE) || checkKey('P')) && g_keyCooldown[VK_ESCAPE] == 0 && g_keyCooldown['P'] == 0) {
            gameState = STATE_EXPLORE;
            g_keyCooldown[VK_ESCAPE] = KEY_COOLDOWN_FRAMES;
            g_keyCooldown['P'] = KEY_COOLDOWN_FRAMES;
        }
        if (checkKey('R') && g_keyCooldown['R'] == 0) {
            initGame();
            g_keyCooldown['R'] = KEY_COOLDOWN_FRAMES;
        }
    }
}

void handleMouseInput() {
    while (MouseHit()) {
        MOUSEMSG m = GetMouseMsg();
        mouseX = m.x;
        mouseY = m.y;
        if (m.uMsg == WM_LBUTTONDOWN) {
            if (gameState == STATE_BACKPACK) {
                int slot = getSlotFromMouse(m.x, m.y);
                int shop = getShopItemFromMouse(m.x, m.y);
                if (shop != -1) {
                    buyItem(shop);
                }
                else if (slot != -1 && g_backpack.slots[slot].active) {
                    selectedSlot = slot;
                    showActionMenu = true;
                    actionMenuX = m.x + 10;
                    actionMenuY = m.y + 10;
                }
                else if (showActionMenu) {
                    if (m.x > actionMenuX && m.x < actionMenuX + 80) {
                        if (m.y > actionMenuY && m.y < actionMenuY + 25)
                            useItem(selectedSlot);
                        else if (m.y > actionMenuY + 30 && m.y < actionMenuY + 55)
                            (g_backpack.slots[selectedSlot].type == ITEM_WEAPON) ? equipWeapon(selectedSlot) : sellItem(selectedSlot);
                    }
                    showActionMenu = false;
                }
                else {
                    showActionMenu = false;
                }
            }
            else if (gameState == STATE_COMBAT && inFight) {
                int btnY = SCREEN_HEIGHT - 130;
                int btnW = 180, btnH = 55, gap = 50;
                int startX = SCREEN_WIDTH / 2 - (btnW * 3 + gap * 2) / 2;
                if (m.y > btnY && m.y < btnY + btnH) {
                    for (int i = 0; i < 3; i++) {
                        int bx = startX + i * (btnW + gap);
                        if (m.x > bx && m.x < bx + btnW) {
                            if (i == 0) playerAttack();
                            else if (i == 1) playerDefend();
                            else if (i == 2) tryRun();
                            break;
                        }
                    }
                }
            }
            else if (gameState == STATE_PAUSE || gameState == STATE_GAMEOVER) {
                if (m.x > 300 && m.x < 500) {
                    if (m.y > 250 && m.y < 300) {
                        if (gameState == STATE_GAMEOVER) {
                            initGame();
                        }
                        else {
                            gameState = STATE_EXPLORE;
                        }
                    }
                    else if (m.y > 320 && m.y < 370) {
                        closegraph();
                        exit(0);
                    }
                }
            }
        }
    }
}
#pragma endregion

#pragma region Vision System
void updateExplored() {
    float vcx = player.x + 0.5f;
    float vcy = player.y + 0.5f;
    float r2 = (float)(VIEW_RADIUS * VIEW_RADIUS);
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            float dx = (x + 0.5f) - vcx;
            float dy = (y + 0.5f) - vcy;
            if (dx * dx + dy * dy <= r2) {
                explored[y][x] = true;
            }
        }
    }
}

float getBrightness(int x, int y) {
    float dx = (x + 0.5f) - (player.x + 0.5f);
    float dy = (y + 0.5f) - (player.y + 0.5f);
    float dist = sqrtf(dx * dx + dy * dy);
    float radius = (float)VIEW_RADIUS;
    if (dist >= radius) return 0.0f;
    float t = dist / radius;
    float brightness = 1.0f - t * t * (3.0f - 2.0f * t);
    return brightness;
}

void drawMiniMap() {
    int mmX = SCREEN_WIDTH - 160, mmY = 20;
    int mmW = 150, mmH = 120;
    float scaleX = (float)mmW / MAP_WIDTH;
    float scaleY = (float)mmH / MAP_HEIGHT;

    setfillcolor(RGBA(10, 10, 15, 220));
    solidrectangle(mmX - 2, mmY - 2, mmX + mmW + 2, mmY + mmH + 2);
    setlinecolor(RGB(50, 50, 60));
    rectangle(mmX - 2, mmY - 2, mmX + mmW + 2, mmY + mmH + 2);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (explored[y][x]) {
                COLORREF col = RGB(60, 60, 65);
                if (map[y][x] == TILE_WALL || map[y][x] == TILE_PILLAR)
                    col = RGB(100, 100, 105);
                else if (map[y][x] == TILE_STAIRS)
                    col = RGB(180, 160, 80);
                setfillcolor(col);
                solidrectangle(mmX + (int)(x * scaleX), mmY + (int)(y * scaleY),
                    mmX + (int)((x + 1) * scaleX), mmY + (int)((y + 1) * scaleY));
            }
        }
    }

    int blink = (GetTickCount() / 250) % 2;
    if (blink) {
        setfillcolor(RGB(0, 180, 255));
        solidcircle(mmX + (int)((player.x + 0.5f) * scaleX),
            mmY + (int)((player.y + 0.5f) * scaleY), 3);
    }

    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].alive) {
            int ex = enemies[i].x, ey = enemies[i].y;
            if (explored[ey][ex]) {
                setfillcolor(RGB(255, 0, 0));
                solidcircle(mmX + (int)((ex + 0.5f) * scaleX),
                    mmY + (int)((ey + 0.5f) * scaleY), 2);
            }
        }
    }
}
#pragma endregion

#pragma region UI & Rendering
void drawDamageTexts() {
    for (int i = 0; i < dmgTextCount; i++) {
        if (dmgTexts[i].life > 0) {
            char t[16];
            sprintf_s(t, "%d", abs(dmgTexts[i].val));
            settextcolor(dmgTexts[i].color);
            settextstyle(16, 0, "微软雅黑");
            outtextxy(dmgTexts[i].x, dmgTexts[i].y - (40 - dmgTexts[i].life), t);
            dmgTexts[i].life--;
        }
    }
    int w = 0;
    for (int i = 0; i < dmgTextCount; i++)
        if (dmgTexts[i].life > 0)
            dmgTexts[w++] = dmgTexts[i];
    dmgTextCount = w;
}

void drawMessage() {
    if (g_messageTimer > 0) {
        settextcolor(g_messageColor);
        settextstyle(20, 0, "微软雅黑");
        outtextxy(SCREEN_WIDTH / 2 - textwidth(g_messageText) / 2,
            SCREEN_HEIGHT / 2 - 60, g_messageText);
        g_messageTimer--;
    }
}

void drawCombatLog() {
    settextstyle(14, 0, "微软雅黑");
    for (int i = 0; i < logCount; i++) {
        settextcolor(RGBA(200, 200, 210, 255 - i * 35));
        outtextxy(20, 50 + i * 20, combatLogs[i].text);
    }
}

int hashCoord(int x, int y, int seed) {
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + seed * 1013904223);
    h = (h ^ (h >> 13)) * 1274126177;
    return (int)(h & 0x7FFFFFFF);
}

void drawMap() {
    float vcx = player.x + 0.5f;
    float vcy = player.y + 0.5f;
    float r2 = (float)(VIEW_RADIUS * VIEW_RADIUS);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            float dx = (x + 0.5f) - vcx;
            float dy = (y + 0.5f) - vcy;
            float distSq = dx * dx + dy * dy;
            if (distSq > r2) continue;

            int sx = x * TILE_SIZE, sy = y * TILE_SIZE;

            float dist = sqrtf(distSq);
            float t = dist / VIEW_RADIUS;
            float brightness = 1.0f - t * t * (3.0f - 2.0f * t);
            float factor = 0.5f + 0.5f * brightness;

            if (map[y][x] == TILE_WALL) {
                int baseR = 35, baseG = 28, baseB = 30;
                int r = (int)(baseR * factor);
                int g = (int)(baseG * factor);
                int b = (int)(baseB * factor);
                setfillcolor(RGB(r, g, b));
                solidrectangle(sx, sy, sx + TILE_SIZE, sy + TILE_SIZE);

                int gapR = 10, gapG = 8, gapB = 10;
                setlinecolor(RGB((int)(gapR * factor), (int)(gapG * factor), (int)(gapB * factor)));
                setlinestyle(PS_SOLID, 1);

                int h1 = sy + 10 + (hashCoord(x, y, 1) % 6);
                int h2 = sy + 22 + (hashCoord(x, y, 2) % 4);
                line(sx, h1, sx + TILE_SIZE, h1);
                line(sx, h2, sx + TILE_SIZE, h2);

                int v1 = sx + 8 + (hashCoord(x, y, 3) % 8);
                int v2 = sx + 20 + (hashCoord(x, y, 4) % 6);
                line(v1, sy, v1, h1);
                line(v2, h1, v2, h2);
                line(v1 - 4, h2, v1 - 4, sy + TILE_SIZE);

                setlinecolor(RGB((int)(55 * factor), (int)(45 * factor), (int)(48 * factor)));
                line(sx, sy, sx + TILE_SIZE, sy);
                line(sx, sy, sx, sy + TILE_SIZE);
                setlinecolor(RGB((int)(15 * factor), (int)(12 * factor), (int)(15 * factor)));
                line(sx, sy + TILE_SIZE - 1, sx + TILE_SIZE, sy + TILE_SIZE - 1);
                line(sx + TILE_SIZE - 1, sy, sx + TILE_SIZE - 1, sy + TILE_SIZE);

                if (hashCoord(x, y, 5) % 100 < 30) {
                    int cx = sx + 5 + hashCoord(x, y, 6) % 20;
                    int cy = sy + 5 + hashCoord(x, y, 7) % 20;
                    setlinecolor(RGB((int)(15 * factor), (int)(10 * factor), (int)(12 * factor)));
                    line(cx, cy, cx + 4, cy + 6);
                    line(cx + 4, cy + 6, cx + 2, cy + 10);
                }
                if (hashCoord(x, y, 8) % 100 < 20) {
                    int mx = sx + hashCoord(x, y, 9) % 28;
                    int my = sy + hashCoord(x, y, 10) % 28;
                    setfillcolor(RGB((int)(20 * factor), (int)(25 * factor), (int)(18 * factor)));
                    solidcircle(mx, my, 2);
                }
            }
            else if (map[y][x] == TILE_FLOOR) {
                int baseR = 45, baseG = 40, baseB = 38;
                int noise = (hashCoord(x, y, 11) % 10) - 5;
                int r = (int)((baseR + noise) * factor);
                int g = (int)((baseG + noise) * factor);
                int b = (int)((baseB + noise) * factor);
                setfillcolor(RGB(r, g, b));
                solidrectangle(sx, sy, sx + TILE_SIZE, sy + TILE_SIZE);

                setlinecolor(RGB((int)(20 * factor), (int)(18 * factor), (int)(16 * factor)));
                setlinestyle(PS_SOLID, 1);
                if (hashCoord(x, y, 12) % 3 == 0) {
                    int cy = sy + 12 + hashCoord(x, y, 13) % 8;
                    line(sx + 2, cy, sx + TILE_SIZE - 2, cy + (hashCoord(x, y, 14) % 4 - 2));
                }
                if (hashCoord(x, y, 15) % 3 == 0) {
                    int cx = sx + 14 + hashCoord(x, y, 16) % 6;
                    line(cx, sy + 2, cx + (hashCoord(x, y, 17) % 4 - 2), sy + TILE_SIZE - 2);
                }

                int detailCount = hashCoord(x, y, 18) % 4;
                for (int i = 0; i < detailCount; i++) {
                    int dx = sx + 4 + hashCoord(x, y, 19 + i * 2) % 24;
                    int dy = sy + 4 + hashCoord(x, y, 20 + i * 2) % 24;
                    int type = hashCoord(x, y, 21 + i) % 3;
                    if (type == 0) {
                        setfillcolor(RGB((int)(25 * factor), (int)(22 * factor), (int)(20 * factor)));
                        solidcircle(dx, dy, 1);
                    }
                    else if (type == 1) {
                        setfillcolor(RGB((int)(60 * factor), (int)(55 * factor), (int)(50 * factor)));
                        solidcircle(dx, dy, 1);
                    }
                    else {
                        setfillcolor(RGB((int)(30 * factor), (int)(25 * factor), (int)(25 * factor)));
                        solidrectangle(dx, dy, dx + 2, dy + 1);
                    }
                }

                if (hashCoord(x, y, 30) % 100 < 3) {
                    int bx = sx + 8 + hashCoord(x, y, 31) % 16;
                    int by = sy + 8 + hashCoord(x, y, 32) % 16;
                    setfillcolor(RGB((int)(60 * factor), (int)(15 * factor), (int)(15 * factor)));
                    solidcircle(bx, by, 2);
                    setfillcolor(RGB((int)(45 * factor), (int)(10 * factor), (int)(10 * factor)));
                    solidcircle(bx + 3, by + 2, 1);
                }
            }
            else if (map[y][x] == TILE_STAIRS) {
                int baseR = 90, baseG = 75, baseB = 40;
                setfillcolor(RGB((int)(baseR * factor), (int)(baseG * factor), (int)(baseB * factor)));
                solidrectangle(sx, sy, sx + TILE_SIZE, sy + TILE_SIZE);
                settextcolor(RGB((int)(150 * factor), (int)(130 * factor), (int)(80 * factor)));
                settextstyle(14, 0, "微软雅黑");
                outtextxy(sx + 10, sy + 8, "▼");
            }
            else if (map[y][x] == TILE_PILLAR) {
                int baseR = 50, baseG = 42, baseB = 45;
                setfillcolor(RGB((int)(baseR * factor), (int)(baseG * factor), (int)(baseB * factor)));
                solidrectangle(sx, sy, sx + TILE_SIZE, sy + TILE_SIZE);
                setfillcolor(RGB((int)(65 * factor), (int)(55 * factor), (int)(58 * factor)));
                solidrectangle(sx + 8, sy, sx + 16, sy + TILE_SIZE);
            }
            else {
                setfillcolor(COL_BG);
                solidrectangle(sx, sy, sx + TILE_SIZE, sy + TILE_SIZE);
            }
        }
    }
}

void drawPlayer() {
    int px = player.x * TILE_SIZE + TILE_SIZE / 2;
    int py = player.y * TILE_SIZE + TILE_SIZE / 2;

    setfillcolor(RGBA(0, 180, 255, 15)); solidcircle(px, py, 20);
    setfillcolor(RGBA(0, 180, 255, 30)); solidcircle(px, py, 14);
    setfillcolor(RGBA(0, 180, 255, 60)); solidcircle(px, py, 8);
    setfillcolor(RGBA(0, 180, 255, 120)); solidcircle(px, py, 4);

    drawAvatarScaled(PLAYER_AVATAR_TYPE, px, py, TILE_SIZE);
}

void drawAvatar(int offX, int offY, const int data[40][40], const COLORREF palette[], int palSize) {
    for (int y = 0; y < 40; y++)
        for (int x = 0; x < 40; x++) {
            int idx = data[y][x];
            if (idx > 0 && idx < palSize)
                putpixel(offX + x, offY + y, palette[idx]);
        }
}

void drawSkeleton(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(230,225,210), RGB(190,185,170), RGB(20,10,10), RGB(180,30,30), RGB(100,95,85) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 4; y <= 30; y++) for (int x = 8; x <= 31; x++) {
        float cx = 19.5f, cy = 17.0f;
        float rx = 12.0f, ry = 14.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }
    for (int y = 24; y <= 30; y++) for (int x = 10; x <= 29; x++) {
        if (d[y][x] == 1) d[y][x] = 2;
    }
    for (int y = 12; y <= 17; y++) for (int x = 11; x <= 16; x++) {
        float cx2 = 13.5f, cy2 = 14.5f;
        float dx2 = (x - cx2) / 3.0f, dy2 = (y - cy2) / 3.0f;
        if (dx2 * dx2 + dy2 * dy2 <= 1.0f) d[y][x] = 3;
    }
    for (int y = 12; y <= 17; y++) for (int x = 23; x <= 28; x++) {
        float cx2 = 25.5f, cy2 = 14.5f;
        float dx2 = (x - cx2) / 3.0f, dy2 = (y - cy2) / 3.0f;
        if (dx2 * dx2 + dy2 * dy2 <= 1.0f) d[y][x] = 3;
    }
    d[14][13] = 4; d[14][14] = 4; d[15][13] = 4;
    d[14][25] = 4; d[14][26] = 4; d[15][25] = 4;
    d[20][18] = 3; d[20][19] = 3; d[21][18] = 3; d[21][19] = 3;
    d[20][20] = 3; d[20][21] = 3; d[21][20] = 3; d[21][21] = 3;
    for (int x = 13; x <= 26; x++) {
        d[26][x] = 1; d[27][x] = 1;
    }
    for (int x = 13; x <= 26; x += 2) {
        d[27][x] = 5;
    }
    for (int x = 12; x <= 27; x++) {
        d[28][x] = 2; d[29][x] = 2;
    }
    for (int x = 14; x <= 25; x++) d[30][x] = 2;

    drawAvatar(offX, offY, d, pal, 6);
}

void drawGoblin(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(80,180,60), RGB(50,120,35), RGB(240,240,230),
                       RGB(20,20,20), RGB(220,200,50), RGB(120,80,40), RGB(60,30,20) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 6; y <= 33; y++) for (int x = 9; x <= 30; x++) {
        float cx = 19.5f, cy = 19.0f, rx = 11.0f, ry = 14.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }
    for (int y = 26; y <= 33; y++) for (int x = 9; x <= 30; x++)
        if (d[y][x] == 1) d[y][x] = 2;

    for (int i = 0; i < 8; i++) { d[12 + i][8 - i / 2] = 1; d[12 + i][9 - i / 2] = 1; d[12 + i][7 - i / 2] = 2; }
    for (int i = 0; i < 8; i++) { d[12 + i][31 + i / 2] = 1; d[12 + i][30 + i / 2] = 1; d[12 + i][32 + i / 2] = 2; }

    for (int y = 14; y <= 19; y++) for (int x = 12; x <= 17; x++) {
        float dx = (x - 14.5f) / 3.0f, dy = (y - 16.5f) / 2.8f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }
    d[16][14] = 5; d[16][15] = 5; d[17][14] = 4; d[17][15] = 4;
    for (int y = 14; y <= 19; y++) for (int x = 22; x <= 27; x++) {
        float dx = (x - 24.5f) / 3.0f, dy = (y - 16.5f) / 2.8f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }
    d[16][24] = 5; d[16][25] = 5; d[17][24] = 4; d[17][25] = 4;
    d[21][19] = 2; d[21][20] = 2; d[22][19] = 2; d[22][20] = 2;
    for (int x = 14; x <= 25; x++) d[27][x] = 7;
    for (int x = 14; x <= 25; x++) d[28][x] = 7;
    d[26][15] = 3; d[26][16] = 3; d[29][15] = 3; d[29][16] = 3;
    d[26][23] = 3; d[26][24] = 3; d[29][23] = 3; d[29][24] = 3;

    drawAvatar(offX, offY, d, pal, 8);
}

void drawShadowWolf(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(70,70,85), RGB(40,40,55), RGB(255,40,40),
                       RGB(50,50,65), RGB(230,230,220), RGB(100,100,120) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 8; y <= 32; y++) for (int x = 7; x <= 32; x++) {
        float cx = 19.5f, cy = 20.0f, rx = 13.0f, ry = 13.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }
    for (int y = 22; y <= 32; y++) for (int x = 7; x <= 32; x++)
        if (d[y][x] == 1) d[y][x] = 2;

    for (int y = 2; y <= 10; y++) {
        int w = (y - 2);
        for (int x = 10 - w / 2; x <= 10 + w / 2; x++) if (x >= 0 && x < 40) d[y][x] = 1;
    }
    for (int y = 2; y <= 10; y++) {
        int w = (y - 2);
        for (int x = 29 - w / 2; x <= 29 + w / 2; x++) if (x >= 0 && x < 40) d[y][x] = 1;
    }
    d[5][10] = 6; d[6][10] = 6; d[5][29] = 6; d[6][29] = 6;

    for (int y = 24; y <= 34; y++) for (int x = 14; x <= 25; x++) {
        float cx = 19.5f, cy = 28.0f, rx = 6.0f, ry = 6.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 4;
    }
    d[25][19] = 2; d[25][20] = 2; d[26][19] = 2; d[26][20] = 2;

    for (int y = 14; y <= 18; y++) for (int x = 11; x <= 16; x++) {
        float dx = (x - 13.5f) / 2.8f, dy = (y - 16.0f) / 2.2f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }
    for (int y = 14; y <= 18; y++) for (int x = 23; x <= 28; x++) {
        float dx = (x - 25.5f) / 2.8f, dy = (y - 16.0f) / 2.2f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }
    d[30][16] = 5; d[30][17] = 5; d[30][22] = 5; d[30][23] = 5;
    d[31][16] = 5; d[31][22] = 5;
    d[32][17] = 5; d[32][22] = 5;

    drawAvatar(offX, offY, d, pal, 7);
}

void drawZombie(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(120,150,90), RGB(80,110,60), RGB(220,210,180),
                       RGB(180,40,40), RGB(30,20,20), RGB(100,40,40), RGB(140,30,30) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 5; y <= 34; y++) for (int x = 8; x <= 31; x++) {
        float cx = 19.5f, cy = 19.0f, rx = 12.0f, ry = 15.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }
    for (int y = 25; y <= 34; y++) for (int x = 8; x <= 31; x++)
        if (d[y][x] == 1) d[y][x] = 2;

    for (int y = 13; y <= 19; y++) for (int x = 11; x <= 17; x++) {
        float dx = (x - 14.0f) / 3.2f, dy = (y - 16.0f) / 3.2f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }
    d[15][13] = 4; d[15][14] = 4; d[16][13] = 4; d[16][14] = 4;
    for (int y = 14; y <= 19; y++) for (int x = 23; x <= 28; x++) {
        float dx = (x - 25.5f) / 2.5f, dy = (y - 16.5f) / 2.5f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }
    d[16][25] = 4; d[16][26] = 4; d[17][25] = 4;

    d[8][16] = 6; d[8][17] = 6; d[8][18] = 7; d[9][17] = 7; d[9][18] = 6; d[9][19] = 7;
    d[20][10] = 6; d[21][10] = 7; d[21][11] = 6; d[22][11] = 7;

    for (int x = 13; x <= 26; x++) { d[27][x] = 5; d[28][x] = 5; }
    d[26][15] = 5; d[26][24] = 5;
    d[29][16] = 5; d[29][23] = 5;
    d[29][14] = 4; d[30][15] = 4; d[30][24] = 4; d[29][25] = 4;
    d[27][16] = 3; d[27][23] = 3; d[28][19] = 3;

    d[5][15] = 2; d[4][16] = 2; d[5][18] = 2; d[4][20] = 2; d[5][22] = 2; d[4][24] = 2;

    drawAvatar(offX, offY, d, pal, 8);
}

void drawGargoyle(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(150,150,155), RGB(100,100,108), RGB(80,65,50),
                       RGB(240,200,50), RGB(90,85,100), RGB(120,120,128) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 10; y <= 33; y++) for (int x = 10; x <= 29; x++) {
        float cx = 19.5f, cy = 21.0f, rx = 10.0f, ry = 12.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }
    d[15][14] = 6; d[16][15] = 6; d[17][15] = 6; d[18][16] = 6;
    d[25][24] = 6; d[26][25] = 6; d[27][25] = 6;

    for (int y = 2; y <= 11; y++) {
        int xoff = 12 - (11 - y) / 2;
        d[y][xoff] = 3; d[y][xoff + 1] = 3;
    }
    for (int y = 2; y <= 11; y++) {
        int xoff = 27 + (11 - y) / 2;
        d[y][xoff] = 3; d[y][xoff - 1] = 3;
    }

    for (int y = 16; y <= 30; y++) for (int x = 1; x <= 9; x++) {
        int span = (y - 16);
        if (x >= 9 - span / 2 && x <= 9) d[y][x] = 5;
    }
    for (int y = 16; y <= 30; y++) for (int x = 30; x <= 38; x++) {
        int span = (y - 16);
        if (x >= 30 && x <= 30 + span / 2) d[y][x] = 5;
    }

    for (int y = 16; y <= 20; y++) for (int x = 13; x <= 17; x++) {
        float dx = (x - 15.0f) / 2.2f, dy = (y - 18.0f) / 2.0f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 4;
    }
    for (int y = 16; y <= 20; y++) for (int x = 22; x <= 26; x++) {
        float dx = (x - 24.0f) / 2.2f, dy = (y - 18.0f) / 2.0f;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 4;
    }
    for (int x = 14; x <= 25; x++) { d[29][x] = 2; d[30][x] = 2; }
    for (int x = 16; x <= 23; x++) d[31][x] = 2;
    d[28][15] = 1; d[28][16] = 1; d[28][23] = 1; d[28][24] = 1;
    d[29][14] = 2; d[29][25] = 2;
    d[23][19] = 2; d[23][20] = 2; d[24][19] = 2; d[24][20] = 2;

    drawAvatar(offX, offY, d, pal, 7);
}

void drawCryptSpider(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(90,60,100), RGB(55,35,65), RGB(220,50,50),
                       RGB(75,55,45), RGB(70,45,80), RGB(130,100,140) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 18; y <= 36; y++) for (int x = 10; x <= 29; x++) {
        float cx = 19.5f, cy = 27.0f, rx = 10.0f, ry = 9.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }
    d[24][19] = 5; d[24][20] = 5; d[25][18] = 5; d[25][21] = 5;
    d[26][17] = 5; d[26][22] = 5; d[27][19] = 5; d[27][20] = 5;
    d[28][18] = 5; d[28][21] = 5; d[30][19] = 5; d[30][20] = 5;

    for (int y = 8; y <= 22; y++) for (int x = 13; x <= 26; x++) {
        float cx = 19.5f, cy = 15.0f, rx = 7.0f, ry = 7.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 2;
    }

    d[12][16] = 3; d[12][17] = 3; d[12][22] = 3; d[12][23] = 3;
    d[14][15] = 3; d[14][18] = 3; d[14][21] = 3; d[14][24] = 3;

    for (int i = 0; i < 12; i++) { int y = 10 + i / 2; int x = 13 - i; if (y < 40 && x >= 0) d[y][x] = 4; }
    for (int i = 0; i < 14; i++) { int y = 13 + i / 2; int x = 12 - i; if (y < 40 && x >= 0) d[y][x] = 4; }
    for (int i = 0; i < 14; i++) { int y = 17 + i / 2; int x = 12 - i; if (y < 40 && x >= 0) d[y][x] = 4; }
    for (int i = 0; i < 12; i++) { int y = 21 + i / 2; int x = 11 - i; if (y < 40 && x >= 0) d[y][x] = 4; }
    for (int i = 0; i < 12; i++) { int y = 10 + i / 2; int x = 26 + i; if (y < 40 && x < 40) d[y][x] = 4; }
    for (int i = 0; i < 14; i++) { int y = 13 + i / 2; int x = 27 + i; if (y < 40 && x < 40) d[y][x] = 4; }
    for (int i = 0; i < 14; i++) { int y = 17 + i / 2; int x = 27 + i; if (y < 40 && x < 40) d[y][x] = 4; }
    for (int i = 0; i < 12; i++) { int y = 21 + i / 2; int x = 28 + i; if (y < 40 && x < 40) d[y][x] = 4; }

    d[20][18] = 6; d[20][19] = 6; d[21][18] = 6;
    d[20][20] = 6; d[20][21] = 6; d[21][21] = 6;

    drawAvatar(offX, offY, d, pal, 7);
}

void drawNightAssassin(int offX, int offY) {
    COLORREF pal[] = { RGB(45,45,45), RGB(50,30,70), RGB(30,18,45), RGB(60,45,55),
                       RGB(100,180,255), RGB(35,25,40), RGB(180,180,195), RGB(210,215,225) };
    int d[40][40];
    for (int i = 0; i < 40; i++) for (int j = 0; j < 40; j++) d[i][j] = 0;

    for (int y = 5; y <= 38; y++) {
        int half = 5 + (y - 5) * 14 / 33;
        int cx = 19;
        for (int x = cx - half; x <= cx + half; x++)
            if (x >= 0 && x < 40) d[y][x] = 1;
    }
    for (int y = 5; y <= 38; y++) {
        int half = 5 + (y - 5) * 14 / 33;
        int cx = 19;
        for (int x = cx; x <= cx + half; x++)
            if (x >= 0 && x < 40) d[y][x] = 2;
    }

    for (int y = 3; y <= 18; y++) for (int x = 10; x <= 29; x++) {
        float cx = 19.5f, cy = 15.0f, rx = 10.0f, ry = 12.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 1;
    }

    for (int y = 11; y <= 20; y++) for (int x = 14; x <= 25; x++) {
        float cx = 19.5f, cy = 15.5f, rx = 6.0f, ry = 5.0f;
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) d[y][x] = 3;
    }

    for (int y = 16; y <= 20; y++) for (int x = 14; x <= 25; x++) {
        if (d[y][x] == 3) d[y][x] = 5;
    }

    d[13][16] = 4; d[13][17] = 4; d[14][16] = 4; d[14][17] = 4;
    d[13][22] = 4; d[13][23] = 4; d[14][22] = 4; d[14][23] = 4;

    d[28][30] = 6; d[29][31] = 6; d[30][32] = 7; d[31][33] = 7;
    d[32][34] = 7; d[33][35] = 7; d[34][36] = 7;
    d[27][30] = 6; d[28][31] = 6;

    for (int x = 8; x <= 12; x++) { d[20][x] = 6; d[21][x] = 6; }
    for (int x = 27; x <= 31; x++) { d[20][x] = 6; d[21][x] = 6; }

    drawAvatar(offX, offY, d, pal, 8);
}

// ============================================================
//  drawPixelAssassin3D —— 立体像素刺客（40×40 画布）
//  替换原 drawPixelAssassin，可直接放入源.cpp
//  光源：左上方 45°
// ============================================================
void drawPixelAssassin3D(int ox, int oy)
{
    // ---- 辅助 lambda：画一个带高光和阴影的"立体像素块" ----
    // (x,y) 左上角, w×h 尺寸, baseColor 基色, lightDir 光源方向
    auto drawBlock3D = [&](int x, int y, int w, int h,
        COLORREF base, bool bevel = true)
        {
            int br = GetRValue(base), bg = GetGValue(base), bb = GetBValue(base);
            // 高光色（左上方）
            COLORREF hi = RGB(min(255, br + 60), min(255, bg + 60), min(255, bb + 60));
            // 阴影色（右下方）
            COLORREF sh = RGB(max(0, br - 50), max(0, bg - 50), max(0, bb - 50));
            // 过渡色
            COLORREF mid = base;

            if (bevel) {
                // 1) 底面阴影（向右下偏移 1px）
                setfillcolor(sh);
                solidrectangle(x + 1, y + 1, x + w + 1, y + h + 1);
                // 2) 主体填充
                setfillcolor(mid);
                solidrectangle(x, y, x + w, y + h);
                // 3) 顶边高光（1px 线）
                setlinecolor(hi);
                line(x, y, x + w - 1, y);
                line(x, y, x, y + h - 1);
                // 4) 底边暗线
                setlinecolor(sh);
                line(x + w - 1, y, x + w - 1, y + h - 1);
                line(x, y + h - 1, x + w - 1, y + h - 1);
            }
            else {
                setfillcolor(mid);
                solidrectangle(x, y, x + w, y + h);
            }
        };

    // ---- 辅助：画渐变圆形（模拟球体光照） ----
    auto drawSphere = [&](int cx, int cy, int r,
        COLORREF base, COLORREF highlight)
        {
            int br = GetRValue(base), bg = GetGValue(base), bb = GetBValue(base);
            int hr = GetRValue(highlight), hg = GetGValue(highlight), hb = GetBValue(highlight);
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    float dist = sqrtf((float)(dx * dx + dy * dy));
                    if (dist > r) continue;
                    // 光源偏移：高光在左上
                    float lx = (float)(dx + r * 0.3f) / r;
                    float ly = (float)(dy + r * 0.3f) / r;
                    float light = 1.0f - sqrtf(lx * lx + ly * ly);
                    if (light < 0) light = 0;
                    float t = dist / r; // 0=中心, 1=边缘
                    // 边缘暗化
                    float edge = 1.0f - t * t * 0.5f;
                    float l = light * edge;
                    int pr = (int)(br + (hr - br) * l);
                    int pg = (int)(bg + (hg - bg) * l);
                    int pb = (int)(bb + (hb - bb) * l);
                    pr = max(0, min(255, pr));
                    pg = max(0, min(255, pg));
                    pb = max(0, min(255, pb));
                    putpixel(ox + cx + dx, oy + cy + dy, RGB(pr, pg, pb));
                }
            }
        };

    // ======== 开始绘制刺客 ========
    // 坐标基于 40×40 画布，(ox, oy) 为左上角偏移

    // ---------- 1. 披风（最底层，有飘动感） ----------
    // 披风左侧亮面
    drawBlock3D(12, 16, 3, 14, RGB(50, 20, 70));    // 深紫
    drawBlock3D(13, 17, 2, 12, RGB(70, 30, 100));   // 亮紫
    // 披风右侧暗面
    drawBlock3D(25, 16, 3, 14, RGB(35, 12, 50));    // 暗紫
    drawBlock3D(26, 17, 2, 12, RGB(45, 18, 65));
    // 披风飘动末端
    drawBlock3D(11, 28, 4, 3, RGB(55, 22, 80), false);
    drawBlock3D(25, 28, 4, 3, RGB(40, 15, 55), false);
    drawBlock3D(10, 30, 3, 2, RGB(45, 18, 65), false);  // 飘出
    drawBlock3D(27, 30, 3, 2, RGB(30, 10, 40), false);

    // ---------- 2. 身体 / 胸甲 ----------
    // 躯干底色
    drawBlock3D(15, 16, 10, 12, RGB(30, 30, 40));
    // 胸甲片（立体金属感）
    drawBlock3D(16, 17, 8, 5, RGB(60, 65, 80));     // 钢蓝灰
    // 胸甲高光条纹
    setfillcolor(RGB(100, 110, 140));
    solidrectangle(ox + 17, oy + 18, ox + 19, oy + 21);
    // 胸甲暗缝
    setlinecolor(RGB(20, 20, 30));
    line(ox + 20, oy + 17, ox + 20, oy + 22);       // 中缝
    // 胸甲下沿
    drawBlock3D(16, 23, 8, 2, RGB(45, 48, 60));

    // 腰带
    drawBlock3D(15, 25, 10, 2, RGB(80, 55, 30));    // 皮带
    drawBlock3D(19, 25, 3, 2, RGB(180, 160, 60));   // 金色带扣
    setfillcolor(RGB(220, 200, 80));
    solidrectangle(ox + 20, oy + 25, ox + 21, oy + 26); // 带扣高光

    // ---------- 3. 腿部 ----------
    // 左腿（受光面）
    drawBlock3D(16, 28, 4, 8, RGB(35, 35, 45));
    drawBlock3D(16, 28, 1, 8, RGB(50, 50, 65));     // 左边缘高光
    // 右腿（背光面）
    drawBlock3D(21, 28, 4, 8, RGB(25, 25, 35));
    // 靴子
    drawBlock3D(15, 34, 5, 3, RGB(40, 30, 25));     // 左靴
    drawBlock3D(15, 34, 5, 1, RGB(65, 50, 40));     // 靴面高光
    drawBlock3D(21, 34, 5, 3, RGB(30, 22, 18));     // 右靴
    drawBlock3D(21, 34, 5, 1, RGB(50, 38, 30));

    // ---------- 4. 手臂 ----------
    // 左臂（受光）
    drawBlock3D(12, 17, 3, 8, RGB(40, 40, 55));
    drawBlock3D(12, 17, 1, 8, RGB(60, 60, 80));     // 高光边
    // 左护腕
    drawBlock3D(12, 23, 3, 3, RGB(70, 70, 90));
    drawBlock3D(12, 23, 3, 1, RGB(100, 100, 130));  // 护腕高光
    // 左手
    drawSphere(13, 27, 2, RGB(180, 150, 120), RGB(240, 220, 190));

    // 右臂（背光）
    drawBlock3D(25, 17, 3, 8, RGB(30, 30, 42));
    // 右护腕
    drawBlock3D(25, 23, 3, 3, RGB(55, 55, 72));
    // 右手持匕首
    drawSphere(27, 27, 2, RGB(160, 130, 100), RGB(220, 200, 170));

    // ---------- 5. 匕首（右手武器） ----------
    // 刀柄
    drawBlock3D(27, 24, 2, 4, RGB(60, 40, 25));
    drawBlock3D(27, 24, 2, 1, RGB(90, 60, 40));
    // 护手
    drawBlock3D(26, 24, 4, 1, RGB(150, 140, 80));
    // 刀刃（渐变蓝白光）
    for (int i = 0; i < 6; i++) {
        int bright = 180 + i * 12;
        if (bright > 255) bright = 255;
        COLORREF blade = RGB(bright - 40, bright - 20, bright);
        setfillcolor(blade);
        solidrectangle(ox + 28, oy + 18 - i, ox + 29, oy + 19 - i);
    }
    // 刀尖高光
    putpixel(ox + 28, oy + 12, RGB(255, 255, 255));
    putpixel(ox + 29, oy + 12, RGB(220, 240, 255));

    // ---------- 6. 头部（球体光照） ----------
    // 头巾/面罩底色
    drawSphere(20, 10, 6, RGB(35, 35, 50), RGB(80, 80, 120));

    // 面罩细节
    // 眼缝（深色横条）
    setfillcolor(RGB(10, 10, 15));
    solidrectangle(ox + 16, oy + 9, ox + 24, oy + 11);
    // 眼睛发光（左眼亮，右眼稍暗）
    putpixel(ox + 17, oy + 10, RGB(0, 220, 255));   // 左眼 - 青蓝
    putpixel(ox + 18, oy + 10, RGB(0, 180, 220));
    putpixel(ox + 22, oy + 10, RGB(0, 160, 200));   // 右眼 - 稍暗
    putpixel(ox + 23, oy + 10, RGB(0, 130, 170));
    // 眼睛发光光晕
    putpixel(ox + 17, oy + 9, RGB(0, 80, 100));
    putpixel(ox + 18, oy + 11, RGB(0, 60, 80));
    putpixel(ox + 22, oy + 9, RGB(0, 60, 80));

    // 头巾顶部高光弧
    for (int i = 0; i < 5; i++) {
        int alpha = 80 + i * 20;
        putpixel(ox + 16 + i, oy + 5, RGB(alpha, alpha, alpha + 30));
    }

    // 头巾包裹线条
    setlinecolor(RGB(50, 50, 70));
    line(ox + 14, oy + 10, ox + 15, oy + 14);
    line(ox + 26, oy + 10, ox + 25, oy + 14);

    // 头巾飘带（右侧飘出）
    drawBlock3D(26, 8, 3, 2, RGB(40, 20, 60), false);
    drawBlock3D(28, 7, 3, 2, RGB(50, 25, 75), false);
    drawBlock3D(30, 6, 2, 2, RGB(45, 22, 65), false);

    // ---------- 7. 肩膀护甲（立体金属片） ----------
    // 左肩甲
    drawBlock3D(11, 15, 5, 3, RGB(70, 75, 95));
    drawBlock3D(11, 15, 5, 1, RGB(110, 120, 150));  // 高光
    drawBlock3D(12, 16, 3, 1, RGB(90, 95, 120));    // 次高光
    // 右肩甲
    drawBlock3D(24, 15, 5, 3, RGB(55, 58, 75));
    drawBlock3D(24, 15, 5, 1, RGB(85, 90, 115));

    // ---------- 8. 环境光效（底部投影） ----------
    for (int i = 0; i < 3; i++) {
        int a = 30 - i * 10;
        if (a < 0) a = 0;
        setfillcolor(RGB(a, a, a + 5));
        solidrectangle(ox + 14 + i, oy + 37, ox + 26 - i, oy + 38);
    }

    // ---------- 9. 能量符文（胸甲上的发光标记） ----------
    DWORD tick = GetTickCount();
    float pulse = sinf((float)tick / 400.0f) * 0.4f + 0.8f;
    int glow = (int)(100 + 155 * pulse);
    putpixel(ox + 20, oy + 19, RGB(0, glow, (int)(glow * 0.8f)));
    putpixel(ox + 19, oy + 20, RGB(0, (int)(glow * 0.7f), (int)(glow * 0.5f)));
    putpixel(ox + 21, oy + 20, RGB(0, (int)(glow * 0.7f), (int)(glow * 0.5f)));
    putpixel(ox + 20, oy + 21, RGB(0, (int)(glow * 0.5f), (int)(glow * 0.3f)));
}
// ============================================================
//  drawCombatAssassin3D —— 战斗场景大尺寸立体刺客立绘
// ============================================================
void drawCombatAssassin3D(int cx, int cy, float scale = 2.5f)
{
    // 1. 创建 40x40 的离屏画布 (修正：直接在构造函数传参)
    IMAGE buf(40, 40);

    // 2. 保存当前绘图设备，并切换到离屏画布 (修正：GetWorkingImage)
    IMAGE* pOld = GetWorkingImage();
    SetWorkingImage(&buf);

    // 3. 清空离屏画布（使用背景色，以便后续过滤）
    setbkcolor(RGB(20, 20, 25));
    cleardevice();

    // 4. 画角色（使用之前写好的 3D 函数，坐标 0,0）
    drawPixelAssassin3D(0, 0);

    // 5. 切回屏幕
    SetWorkingImage(pOld);
    int sz = (int)(40 * scale);
    int sx = cx - sz / 2;
    int sy = cy - sz / 2-25;

    // ---- 底部阴影椭圆 ----
    for (int i = 0; i < 4; i++) {
        int a = 30 - i * 8; 
        if (a < 0) a = 0;
        int shadowWidth = (int)(sz * 0.8); 
        int shadowHeight = (int)(sz * 0.5);
        setfillcolor(RGB(a, a, a + 10)); 
        /*fillellipse(cx, cy + sz * 0.4,
            shadowWidth,
            shadowHeight);*/
    }

    // ---- 放大绘制（最近邻采样，过滤背景色） ----
    COLORREF bgColor = RGB(20, 20, 25);
    DWORD* pBuf = GetImageBuffer(&buf);
    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            int srcX = (int)(x / scale);
            int srcY = (int)(y / scale);
            if (srcX >= 40) srcX = 39;
            if (srcY >= 40) srcY = 39;
            COLORREF c = pBuf[srcY * 40 + srcX];
            if (c == bgColor) continue;

            // ---- 立体增强：边缘检测描边 ----
            bool isEdge = false;
            if (srcX > 0 && pBuf[srcY * 40 + srcX - 1] == bgColor) isEdge = true;
            if (srcX < 39 && pBuf[srcY * 40 + srcX + 1] == bgColor) isEdge = true;
            if (srcY > 0 && pBuf[(srcY - 1) * 40 + srcX] == bgColor) isEdge = true;
            if (srcY < 39 && pBuf[(srcY + 1) * 40 + srcX] == bgColor) isEdge = true;

            int drawPx = sx + x;
            int drawPy = sy + y;
            if (drawPx >= 0 && drawPx < SCREEN_WIDTH && drawPy >= 0 && drawPy < SCREEN_HEIGHT) {
                if (isEdge) {
                    putpixel(drawPx, drawPy, RGB(15, 15, 20)); // 深色轮廓
                }
                else {
                    putpixel(drawPx, drawPy, c);
                }
            }
        }
    }

    // ---- 外发光（角色周围的能量光晕 ） ----
    DWORD tick = GetTickCount();
    float breathe = sinf((float)tick / 400.0f) * 0.4f + 0.8f;
    for (int ring = 0; ring < 5; ring++) {
        int r = (int)sz * 0.6 + ring * 8; // 半径从角色外侧更远处开始
        int alpha = (int)((30 - ring * 6) * breathe);
        if (alpha <= 0) continue;
        setlinecolor(RGB(0, alpha * 4, alpha * 6)); // 颜色更亮
        setlinestyle(PS_SOLID, 2); // 线宽加粗
        circle(cx, cy - 10, r); // 圆心也跟着角色上移
    }
    setlinestyle(PS_SOLID, 1); // 恢复默认线宽
}
void drawAvatarScaled(int type, int cx, int cy, int size) {
    IMAGE img(40, 40);
    SetWorkingImage(&img);
    setbkcolor(RGB(20, 20, 25));
    cleardevice();

    switch (type) {
    case 0: drawSkeleton(0, 0); break;
    case 1: drawGoblin(0, 0); break;
    case 2: drawShadowWolf(0, 0); break;
    case 3: drawZombie(0, 0); break;
    case 4: drawGargoyle(0, 0); break;
    case 5: drawCryptSpider(0, 0); break;
    case 6: drawNightAssassin(0, 0); break;
    case 7: drawPixelAssassin3D(0, 0); break;
    default: break;
    }

    SetWorkingImage();

    float scale = (float)size / 40.0f;
    int half = size / 2;
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            int srcX = (int)(dx / scale);
            int srcY = (int)(dy / scale);
            if (srcX < 0) srcX = 0; if (srcX >= 40) srcX = 39;
            if (srcY < 0) srcY = 0; if (srcY >= 40) srcY = 39;

            SetWorkingImage(&img);
            COLORREF col = getpixel(srcX, srcY);
            SetWorkingImage();

            if (col != RGB(20, 20, 25)) {
                int px = cx - half + dx;
                int py = cy - half + dy;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                    putpixel(px, py, col);
            }
        }
    }
}

void drawEnemies() {
    int px = player.x, py = player.y;
    int r2 = VIEW_RADIUS * VIEW_RADIUS;

    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;
        int ex = enemies[i].x, ey = enemies[i].y;
        int dx = ex - px, dy = ey - py;
        if (dx * dx + dy * dy > r2) continue;

        int drawX = ex * TILE_SIZE + (TILE_SIZE - 40) / 2;
        int drawY = ey * TILE_SIZE + (TILE_SIZE - 40) / 2;

        switch (enemies[i].avatarType) {
        case 0: drawSkeleton(drawX, drawY); break;
        case 1: drawGoblin(drawX, drawY); break;
        case 2: drawShadowWolf(drawX, drawY); break;
        case 3: drawZombie(drawX, drawY); break;
        case 4: drawGargoyle(drawX, drawY); break;
        case 5: drawCryptSpider(drawX, drawY); break;
        case 6: drawNightAssassin(drawX, drawY); break;
        default: break;
        }
    }
}

void drawUI() {
    int barY = MAP_HEIGHT * TILE_SIZE;
    setfillcolor(RGBA(20, 20, 25, 220));
    solidrectangle(0, barY, SCREEN_WIDTH, SCREEN_HEIGHT);
    setlinecolor(RGB(60, 60, 70));
    line(0, barY, SCREEN_WIDTH, barY);

    char st[128];
    const char* wpn = g_equippedWeapon.isValid ? g_equippedWeapon.name : "无";
    char buffInfo[32] = "";
    if (player.tempAtkDuration > 0) {
        sprintf_s(buffInfo, "  |  临时攻击: +%d (%d回合)", player.tempAtkBonus, player.tempAtkDuration);
    }
    sprintf_s(st, "层数: %d  |  攻击: %d  |  防御: %d  |  金币: %d  |  武器: %s%s",
        player.floor, player.atk, player.def, player.gold, wpn, buffInfo);
    settextstyle(18, 0, "微软雅黑");
    settextcolor(RGB(200, 200, 210));
    outtextxy(20, barY + 12, st);
    // 在右侧添加血量显示
    int hpBarW = 200, hpBarH = 20;
    int hpBarX = SCREEN_WIDTH - hpBarW - 50;
    int hpBarY = barY + 15;

    // 绘制 "HP:" 标签
    settextstyle(16, 0, "微软雅黑");
    settextcolor(RGB(255, 100, 100));
    outtextxy(hpBarX - 35, hpBarY + 2, "HP:");

    // 绘制血条
    drawHealthBarEx(hpBarX, hpBarY, hpBarW, hpBarH, player.hp, player.maxHp, RGB(100, 180, 255));

    drawMessage();
    drawDamageTexts();
}

void drawHealthBarEx(int x, int y, int w, int h, int cur, int max, COLORREF color) {
    setfillcolor(RGB(30, 30, 35));
    solidrectangle(x, y, x + w, y + h);
    setlinecolor(RGB(60, 60, 70));
    rectangle(x, y, x + w, y + h);

    float ratio = max > 0 ? (float)cur / max : 0;
    int fillW = (int)(w * ratio);
    if (fillW > 0) {
        COLORREF barColor = color;
        if (ratio < 0.25) barColor = RGB(255, 60, 60);
        else if (ratio < 0.5) barColor = RGB(255, 200, 60);
        setfillcolor(barColor);
        solidrectangle(x + 2, y + 2, x + fillW - 2, y + h - 2);
        setfillcolor(RGBA(255, 255, 255, 80));
        solidrectangle(x + 2, y + 2, x + fillW - 2, y + h / 2);
    }

    char hpText[32];
    sprintf_s(hpText, "%d / %d", cur, max);
    settextstyle(14, 0, "微软雅黑");
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    outtextxy(x + w / 2 - textwidth(hpText) / 2, y + h / 2 - 7, hpText);
}

void drawCharacter(int x, int y, const char* name, int level, int hp, int maxHp, COLORREF bodyColor, bool isEnemy, int avatarType) {
    int bodyR = 60;
    int drawX = x + animOffsetX * (isEnemy ? -1 : 1);
    int drawY = y + animOffsetY;

    setfillcolor(RGBA(0, 0, 0, 60));
    solidellipse(drawX - bodyR + 5, drawY - bodyR + 10, drawX + bodyR + 5, drawY + bodyR + 10);

    if (avatarType >= 0) {
        setfillcolor(RGB(20, 20, 25));
        solidcircle(drawX, drawY, bodyR);
        drawAvatarScaled(avatarType, drawX, drawY, 120);
    }
    else {
        setfillcolor(bodyColor);
        solidcircle(drawX, drawY, bodyR);

        int eyeOffX = isEnemy ? -15 : 15;
        setfillcolor(WHITE);
        solidcircle(drawX + eyeOffX - 8, drawY - 15, 12);
        solidcircle(drawX + eyeOffX + 8, drawY - 15, 12);
        setfillcolor(isEnemy ? RGB(200, 30, 30) : RGB(30, 30, 30));
        solidcircle(drawX + eyeOffX - 8, drawY - 15, 6);
        solidcircle(drawX + eyeOffX + 8, drawY - 15, 6);

        setlinecolor(isEnemy ? RGB(150, 20, 20) : RGB(20, 20, 20));
        setlinestyle(PS_SOLID, 3);
        if (isEnemy) {
            arc(drawX - 20, drawY + 10, drawX + 20, drawY + 35, 0, 3.14f);
        }
        else {
            arc(drawX - 15, drawY + 5, drawX + 15, drawY + 25, 0, 3.14f);
        }

        if (showShield && !isEnemy) {
            setlinecolor(RGBA(100, 200, 255, 150 + (int)(sin(animTimer * 0.2) * 50)));
            setlinestyle(PS_SOLID, 4);
            circle(drawX, drawY, bodyR + 15);
            circle(drawX, drawY, bodyR + 20);
        }
    }

    char nameLv[48];
    sprintf_s(nameLv, "%s  Lv.%d", name, level);
    settextstyle(18, 0, "微软雅黑");
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    int textW = textwidth(nameLv);
    outtextxy(x - textW / 2, y - bodyR - 35, nameLv);

    int barW = 180, barH = 20;
    int barX = x - barW / 2;
    int barY = y + bodyR + 20;
    drawHealthBarEx(barX, barY, barW, barH, hp, maxHp, isEnemy ? RGB(100, 255, 100) : RGB(100, 180, 255));
}

void triggerAttackAnim(bool isPlayer) {
    animPhase = isPlayer ? 1 : 2;
    animTimer = 0;
    animOffsetX = 0;
    animOffsetY = 0;
}

void triggerDefendAnim() {
    animPhase = 3;
    animTimer = 0;
    showShield = true;
}

void triggerRunAnim() {
    animPhase = 4;
    animTimer = 0;
}

void drawCombatScene() {
    // 背景渐变
    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
        float t = (float)y / SCREEN_HEIGHT;
        int r = (int)(15 + t * 20);
        int g = (int)(15 + t * 10);
        int b = (int)(35 + t * 30);
        setlinecolor(RGB(r, g, b));
        line(0, y, SCREEN_WIDTH, y);
    }

    // 地面区域
    setfillcolor(RGBA(20, 30, 20, 100));
    solidrectangle(0, SCREEN_HEIGHT * 0.65, SCREEN_WIDTH, SCREEN_HEIGHT * 0.75);
    setfillcolor(RGBA(15, 25, 15, 120));
    solidrectangle(0, SCREEN_HEIGHT * 0.75, SCREEN_WIDTH, SCREEN_HEIGHT);

    // 动画处理
    if (animPhase > 0) {
        animTimer++;
        if (animPhase == 1 || animPhase == 2) {
            if (animTimer < 8) animOffsetX = animTimer * 8;
            else if (animTimer < 16) animOffsetX = (16 - animTimer) * 8;
            else { animPhase = 0; animOffsetX = 0; }
        }
        else if (animPhase == 3) {
            if (animTimer > 30) { animPhase = 0; showShield = false; }
        }
        else if (animPhase == 4) {
            if (animTimer > 20) animPhase = 0;
        }
        else if (animPhase == 5) { if (animTimer > 30) animPhase = 0; }
        else if (animPhase == 6) { if (animTimer > 20) animPhase = 0; }
    }

    // 绘制敌人
    if (fightEnemyIndex >= 0 && enemies[fightEnemyIndex].alive) {
        Enemy& e = enemies[fightEnemyIndex];
        int eLevel = player.floor * 2 + (e.isBoss ? 5 : 0);
        drawCharacter(SCREEN_WIDTH / 2, 220, e.name, eLevel, e.hp, e.maxHp,
            e.isBoss ? RGB(200, 50, 80) : RGB(180, 60, 60), true, e.avatarType);
    }
        // 绘制玩家 (使用 3D 战斗立绘)
    int pLevel = player.floor + 5;
    int playerDrawX = SCREEN_WIDTH / 2 + animOffsetX; // 保留攻击动画偏移
    int playerDrawY = 520 + animOffsetY;

    // 1. 绘制底部阴影和 3D 放大立绘
    drawCombatAssassin3D(playerDrawX, playerDrawY, 2.5f);

    // 2. 绘制防御护盾 (如果有)
    if (showShield) {
        setlinecolor(RGBA(100, 200, 255, 150 + (int)(sin(animTimer * 0.2) * 50)));
        setlinestyle(PS_SOLID, 4);
        circle(playerDrawX, playerDrawY, 75);
        circle(playerDrawX, playerDrawY, 80);
    }

    // 3. 绘制名字和等级
    char nameLv[48];
    sprintf_s(nameLv, "勇者  Lv.%d", pLevel);
    settextstyle(18, 0, "微软雅黑");
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    int textW = textwidth(nameLv);
    outtextxy(SCREEN_WIDTH / 2 - textW / 2, 520 - 85, nameLv);

    // 4. 绘制血条
    int barW = 180, barH = 20;
    int barX = SCREEN_WIDTH / 2 - barW / 2;
    int barY = 520 + 80;
    drawHealthBarEx(barX, barY, barW, barH, player.hp, player.maxHp, RGB(100, 180, 255));
    // 【核心修复】战斗日志框 + 文字显示
    // 1. 强制重置绘图状态，防止被前面的角色绘制干扰
    setbkmode(TRANSPARENT);
    setlinestyle(PS_SOLID, 1);
    setfillcolor(RGB(0, 0, 0, 100)); // 先用纯黑测试，确保框能显示
    solidrectangle(SCREEN_WIDTH / 2 - 250, 340, SCREEN_WIDTH / 2 + 250, 420);

    // 2. 设置字体和颜色
    settextstyle(16, 0, "微软雅黑");
    settextcolor(RGB(220, 220, 230)); // 先用黄色测试，最显眼

    int logX = SCREEN_WIDTH / 2 - 240;
    int logY = 350;

    // 3. 绘制文字
    if (logCount == 0) {
        outtextxy(logX, logY, "等待战斗日志..."); // 测试空状态
    }
    else {
        for (int i = 0; i < min(logCount, 3); i++) {
            // 强制每行都设置颜色，防止状态丢失
            settextcolor(RGB(255, 255, 0));
            outtextxy(logX, logY + i * 22, combatLogs[i].text);
        }
    }
    // 操作按钮
    int btnY = SCREEN_HEIGHT - 130;
    int btnW = 180, btnH = 55, gap = 50;
    int startX = SCREEN_WIDTH / 2 - (btnW * 3 + gap * 2) / 2;
    const char* btnTexts[] = { "攻击", "防御", "逃跑" };

    for (int i = 0; i < 3; i++) {
        int bx = startX + i * (btnW + gap);
        bool hover = (mouseX > bx && mouseX < bx + btnW && mouseY > btnY && mouseY < btnY + btnH);

        setfillcolor(hover ? RGB(70, 70, 100) : RGB(40, 40, 60));
        solidrectangle(bx, btnY, bx + btnW, btnY + btnH);
        setlinecolor(hover ? RGB(100, 150, 255) : RGB(80, 80, 100));
        setlinestyle(PS_SOLID, 2);
        rectangle(bx, btnY, bx + btnW, btnY + btnH);

        settextstyle(20, 0, "微软雅黑");
        settextcolor(hover ? RGB(200, 220, 255) : RGB(180, 180, 200));
        setbkmode(TRANSPARENT);
        outtextxy(bx + btnW / 2 - textwidth(btnTexts[i]) / 2, btnY + btnH / 2 - 10, btnTexts[i]);

        char keyHint[8];
        sprintf_s(keyHint, "[%d]", i + 1);
        settextstyle(12, 0, "微软雅黑");
        settextcolor(RGB(120, 120, 140));
        outtextxy(bx + btnW / 2 - textwidth(keyHint) / 2, btnY + btnH / 2 + 10, keyHint);
    }

    // 伤害数字
    drawDamageTexts();

    // 流血特效
    if (animPhase == 5) {
        int alpha = 120 - animTimer * 4;
        if (alpha > 0) {
            setfillcolor(RGBA(200, 20, 20, alpha));
            solidrectangle(0, 0, SCREEN_WIDTH, 40);
            solidrectangle(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, SCREEN_HEIGHT);
            solidrectangle(0, 0, 40, SCREEN_HEIGHT);
            solidrectangle(SCREEN_WIDTH - 40, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
    if (animPhase == 6) {
        if (animTimer % 4 < 2) {
            setlinecolor(RGB(255, 50, 50));
            setlinestyle(PS_SOLID, 4);
            rectangle(10, 10, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10);
        }
    }
    if (player.bleedTurns > 0 && animPhase != 5 && animPhase != 6) {
        int pulse = (GetTickCount() / 500) % 2;
        if (pulse) {
            setfillcolor(RGBA(150, 0, 0, 30));
            solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }

    // Debuff状态显示
    int debuffY = 620;
    settextstyle(16, 0, "微软雅黑");
    setbkmode(TRANSPARENT);
    int debuffX = SCREEN_WIDTH / 2 - 120;
    if (player.bleedTurns > 0) {
        settextcolor(RGB(255, 50, 50));
        outtextxy(debuffX, debuffY, "【流血】");
        debuffX += 80;
    }
    if (player.webbedTurns > 0) {
        settextcolor(RGB(200, 200, 255));
        outtextxy(debuffX, debuffY, "【缠绕】");
        debuffX += 80;
    }
    if (player.atkDebuffTurns > 0) {
        settextcolor(RGB(100, 255, 100));
        outtextxy(debuffX, debuffY, "【中毒】");
    }

    // 技能提示
    if (g_skillNoticeTimer > 0) {
        g_skillNoticeTimer--;
        int alpha = (g_skillNoticeTimer > 30) ? (45 - g_skillNoticeTimer) * 17 : g_skillNoticeTimer * 8;
        if (alpha > 255) alpha = 255;

        settextstyle(48, 0, "微软雅黑");
        setbkmode(TRANSPARENT);
        settextcolor(RGBA(0, 0, 0, alpha));
        outtextxy(SCREEN_WIDTH / 2 - 140 + 2, SCREEN_HEIGHT / 2 - 100 + 2, g_skillNotice);
        settextcolor(RGBA(GetRValue(g_skillNoticeColor), GetGValue(g_skillNoticeColor), GetBValue(g_skillNoticeColor), alpha));
        outtextxy(SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 100, g_skillNotice);
    }

}
void drawBackpackAndShop() {
    setfillcolor(RGBA(10, 10, 15, 200));
    solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    int barY = 30, barH = 40;
    setfillcolor(RGBA(30, 30, 42, 220));
    solidrectangle(40, barY, SCREEN_WIDTH - 40, barY + barH);
    setlinecolor(RGB(0, 191, 255));
    rectangle(40, barY, SCREEN_WIDTH - 40, barY + barH);
    settextstyle(18, 0, "微软雅黑");
    char st[128];
    const char* wpn = g_equippedWeapon.isValid ? g_equippedWeapon.name : "无";
    char buffInfo[32] = "";
    if (player.tempAtkDuration > 0) {
        sprintf_s(buffInfo, "  |  临时攻击: +%d (%d回合)", player.tempAtkBonus, player.tempAtkDuration);
    }
    sprintf_s(st, "金币: %d  |  攻击: %d  |  防御: %d  |  武器: %s%s", player.gold, player.atk, player.def, wpn, buffInfo);
    settextcolor(RGB(255, 215, 0));
    outtextxy(60, barY + 10, st);

    int bpX = 60, bpY = 90;
    int bpW = 3 * (CELL_SIZE + CELL_GAP) - CELL_GAP + 30;
    int bpH = 3 * (CELL_SIZE + CELL_GAP) - CELL_GAP + 60;
    setfillcolor(RGBA(25, 25, 35, 210));
    solidrectangle(bpX, bpY, bpX + bpW, bpY + bpH);
    setlinecolor(RGB(0, 191, 255));
    rectangle(bpX, bpY, bpX + bpW, bpY + bpH);
    settextstyle(20, 0, "微软雅黑");
    settextcolor(RGB(0, 191, 255));
    outtextxy(bpX + 15, bpY + 10, "背包");

    int sx = bpX + 15, sy = bpY + 40;
    for (int i = 0; i < BACKPACK_SIZE; i++) {
        int cx = sx + (i % 3) * (CELL_SIZE + CELL_GAP);
        int cy = sy + (i / 3) * (CELL_SIZE + CELL_GAP);
        bool hv = (getSlotFromMouse(mouseX, mouseY) == i);
        setfillcolor(hv ? RGB(55, 55, 70) : RGB(40, 40, 50));
        solidrectangle(cx, cy, cx + CELL_SIZE, cy + CELL_SIZE);
        setlinecolor(hv ? RGB(0, 191, 255) : RGB(80, 80, 90));
        rectangle(cx, cy, cx + CELL_SIZE, cy + CELL_SIZE);

        if (g_backpack.slots[i].active) {
            Item& it = g_backpack.slots[i];
            const char* ic = (it.type == ITEM_WEAPON) ? "[W]" : (it.type == ITEM_POTION) ? "[P]" : (it.type == ITEM_ARMOR) ? "[A]" : "[B]";
            COLORREF cl = (it.type == ITEM_WEAPON) ? RGB(255, 107, 107) : (it.type == ITEM_POTION) ? RGB(100, 255, 100) : (it.type == ITEM_ARMOR) ? RGB(100, 200, 255) : RGB(255, 200, 100);
            settextstyle(20, 0, "微软雅黑");
            settextcolor(cl);
            outtextxy(cx + 5, cy + 5, ic);
            settextstyle(13, 0, "微软雅黑");
            settextcolor(WHITE);
            outtextxy(cx + 5, cy + 28, it.name);
            if (it.type == ITEM_WEAPON) {
                char d[16];
                sprintf_s(d, "耐:%d", it.durability);
                settextcolor(RGB(160, 160, 160));
                outtextxy(cx + 5, cy + 44, d);
            }
            else if (it.type == ITEM_BUFF_POTION) {
                char extra[16];
                sprintf_s(extra, "+%d攻", it.value);
                settextcolor(RGB(255, 200, 100));
                outtextxy(cx + 5, cy + 44, extra);
            }
        }
        else {
            settextstyle(13, 0, "微软雅黑");
            settextcolor(RGBA(120, 120, 130, 100));
            outtextxy(cx + CELL_SIZE / 2 - 7, cy + CELL_SIZE / 2 - 8, "空");
        }
    }
    settextstyle(12, 0, "微软雅黑");
    settextcolor(RGB(140, 140, 150));
    outtextxy(bpX + 15, bpY + bpH - 18, "点击物品操作");

    int spX = bpX + bpW + 40;
    int spW = 2 * (CELL_SIZE + CELL_GAP) - CELL_GAP + 30;
    setfillcolor(RGBA(25, 25, 35, 210));
    solidrectangle(spX, bpY, spX + spW, bpY + bpH);
    setlinecolor(RGB(255, 215, 0));
    rectangle(spX, bpY, spX + spW, bpY + bpH);
    settextstyle(20, 0, "微软雅黑");
    settextcolor(RGB(255, 215, 0));
    outtextxy(spX + 15, bpY + 10, "商店");

    sx = spX + 15;
    sy = bpY + 40;
    for (int i = 0; i < g_shopItemCount; i++) {
        int cx = sx + (i % 2) * (CELL_SIZE + CELL_GAP);
        int cy = sy + (i / 2) * (CELL_SIZE + CELL_GAP);
        bool hv = (getShopItemFromMouse(mouseX, mouseY) == i);
        bool cb = (player.gold >= g_shopItems[i].price);
        setfillcolor(cb ? (hv ? RGB(60, 55, 45) : RGB(45, 40, 35)) : RGB(35, 35, 38));
        solidrectangle(cx, cy, cx + CELL_SIZE, cy + CELL_SIZE);
        setlinecolor(hv && cb ? RGB(255, 215, 0) : RGB(80, 80, 90));
        rectangle(cx, cy, cx + CELL_SIZE, cy + CELL_SIZE);

        ShopItem& it = g_shopItems[i];
        const char* ic = (it.type == ITEM_WEAPON) ? "[W]" : (it.type == ITEM_POTION) ? "[P]" : (it.type == ITEM_ARMOR) ? "[A]" : "[B]";
        COLORREF cl = (it.type == ITEM_WEAPON) ? RGB(255, 107, 107) : (it.type == ITEM_POTION) ? RGB(100, 255, 100) : (it.type == ITEM_ARMOR) ? RGB(100, 200, 255) : RGB(255, 200, 100);
        settextstyle(20, 0, "微软雅黑");
        settextcolor(cb ? cl : RGB(100, 100, 100));
        outtextxy(cx + 5, cy + 5, ic);
        settextstyle(13, 0, "微软雅黑");
        settextcolor(cb ? WHITE : RGB(120, 120, 120));
        outtextxy(cx + 5, cy + 28, it.name);
        char pr[16];
        sprintf_s(pr, "$%d", it.price);
        settextcolor(cb ? RGB(255, 215, 0) : RGB(100, 100, 100));
        outtextxy(cx + 5, cy + 44, pr);
        if (!cb) {
            settextstyle(11, 0, "微软雅黑");
            settextcolor(RGB(255, 80, 80));
            outtextxy(cx + CELL_SIZE - 40, cy + 4, "不足");
        }
    }
    settextstyle(12, 0, "微软雅黑");
    settextcolor(RGB(140, 140, 150));
    outtextxy(spX + 15, bpY + bpH - 18, "点击商品购买");

    if (showActionMenu && selectedSlot >= 0 && g_backpack.slots[selectedSlot].active) {
        int mw = 100, mh = 70;
        int mx = min(actionMenuX, SCREEN_WIDTH - mw - 10);
        int my = min(actionMenuY, SCREEN_HEIGHT - mh - 10);
        setfillcolor(RGBA(20, 20, 30, 230));
        solidrectangle(mx, my, mx + mw, my + mh);
        setlinecolor(RGB(0, 191, 255));
        rectangle(mx, my, mx + mw, my + mh);

        bool hu = (mouseX > mx && mouseX < mx + mw && mouseY > my && mouseY < my + 35);
        bool hs = (mouseX > mx && mouseX < mx + mw && mouseY > my + 35 && mouseY < my + mh);
        setfillcolor(hu ? RGB(50, 50, 70) : RGBA(0, 0, 0, 0));
        solidrectangle(mx, my, mx + mw, my + 35);
        setfillcolor(hs ? RGB(50, 50, 70) : RGBA(0, 0, 0, 0));
        solidrectangle(mx, my + 35, mx + mw, my + mh);

        settextstyle(15, 0, "微软雅黑");
        settextcolor(hu ? RGB(0, 191, 255) : WHITE);
        outtextxy(mx + 10, my + 8, (g_backpack.slots[selectedSlot].type == ITEM_WEAPON) ? "装备" : "使用");
        settextcolor(hs ? RGB(255, 215, 0) : WHITE);
        outtextxy(mx + 10, my + 43, "出售");
    }
}
#pragma endregion

#pragma region Initialization & Main Loop
void initGame() {
    srand((unsigned)time(NULL));
    player = { 1, 1, 200, 200, 10, 10, 1, 100, 1, 0, 0, 0, 0, 0, 0 };
    g_equippedWeapon = { "", 0, 0, false };
    memset(&g_backpack, 0, sizeof(g_backpack));
    enemyCount = 0;
    inFight = false;
    gameState = STATE_EXPLORE;
    logCount = 0;
    dmgTextCount = 0;
    g_messageTimer = 0;
    showActionMenu = false;
    memset(g_keyCooldown, 0, sizeof(g_keyCooldown));
    initShop();
    generateLevel();
    showMessage("欢迎来到新肉鸽！WASD移动  B背包  P暂停", WHITE);
}

int main() {
    initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
    HWND hwnd = GetHWnd();
    SetWindowText(hwnd, _T("新肉鸽 - EasyX Roguelike"));
    initGame();
    BeginBatchDraw();

    while (true) {
        for (int i = 0; i < 256; i++) {
            if (g_keyCooldown[i] > 0) g_keyCooldown[i]--;
        }

        handleKeyboard();
        handleMouseInput();
        cleardevice();

        if (gameState == STATE_EXPLORE || gameState == STATE_PAUSE || gameState == STATE_GAMEOVER) {
            drawMap();
            drawEnemies();
            drawPlayer();
            drawUI();
            drawMiniMap();

            if (gameState == STATE_PAUSE) {
                setfillcolor(RGBA(0, 0, 0, 180));
                solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                settextstyle(24, 0, "微软雅黑");
                settextcolor(WHITE);
                outtextxy(SCREEN_WIDTH / 2 - 60, 200, "暂停");
                settextstyle(18, 0, "微软雅黑");
                outtextxy(320, 260, "继续游戏");
                outtextxy(320, 330, "退出游戏");
            }
            else if (gameState == STATE_GAMEOVER) {
                setfillcolor(RGBA(0, 0, 0, 200));
                solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                settextstyle(28, 0, "微软雅黑");
                settextcolor(RGB(255, 80, 80));
                outtextxy(SCREEN_WIDTH / 2 - 80, 200, "游戏结束");
                settextstyle(18, 0, "微软雅黑");
                settextcolor(WHITE);
                outtextxy(320, 260, "重新开始");
                outtextxy(320, 330, "退出游戏");
            }
        }
        else if (gameState == STATE_COMBAT) {
            drawCombatScene();
        }
        else if (gameState == STATE_BACKPACK) {
            drawBackpackAndShop();
        }

        FlushBatchDraw();
        Sleep(16);
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
#pragma endregion