#ifndef BOMB_FLIP_STATE_H
#define BOMB_FLIP_STATE_H

#include <stdbool.h>

#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

#ifndef CHEATS_ENABLED
#define CHEATS_ENABLED 0
#endif

enum {
    MAX_GRID_SIZE = 6,
    MAX_LEVEL = 12,
    MAX_SCANNERS = 2,
    SCREEN_SIZE = 256,
    TILE_SIZE = 24,
    TARGET_FPS = 60,
    BASE_TIME_PER_LEVEL = 45,
    TIME_BONUS_PER_CARD = 3,
    MAX_TIME = 150,
    TIME_WARNING_THRESHOLD = 10,
    FLIP_ANIMATION_FRAMES = 15,
    EXPLOSION_DURATION = 60,
    EXPLOSION_RADIUS = 50,
    LEVEL_CLEAR_DELAY_FRAMES = 60,
    LEVEL_CLEARED_PANEL_FRAMES = 180,
    SCANNER_PREVIEW_FRAMES = 30,
    CHAIN_EXPLOSION_DELAY_FRAMES = 10
};

typedef struct {
    int value;
    bool revealed;
    int flipFrame;
} Tile;

typedef enum {
    GAME_END_PLAYING,
    GAME_END_BOMB,
    GAME_END_COMPLETE,
    GAME_END_TIMEOUT,
    GAME_END_FOLD,
    GAME_END_EASTER_EGG
} GameEndState;

typedef enum {
    GAME_PHASE_ACTIVE,
    GAME_PHASE_BOMB_REVEAL,
    GAME_PHASE_TIMEOUT_CHAIN,
    GAME_PHASE_LEVEL_CLEARING,
    GAME_PHASE_LEVEL_CLEARED,
    GAME_PHASE_NEXT_LEVEL,
    GAME_PHASE_FINISHED
} GamePhase;

typedef struct {
    Tile grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
    int level;
    int totalCoins;
    int levelCoins;
    int rowTotals[MAX_GRID_SIZE];
    int columnTotals[MAX_GRID_SIZE];
    int rowBombs[MAX_GRID_SIZE];
    int columnBombs[MAX_GRID_SIZE];
    int totalCardsFlipped;

    GamePhase phase;
    GameEndState endState;
    int selectedX;
    int selectedY;

    int explosionFrame;
    int explosionCellX;
    int explosionCellY;
    int bombRevealFrame;
    int timeoutNextCell;
    int timeoutDelayFrames;

    int levelClearDelay;
    int levelClearedTimer;
    int timeBonus;
    int foldedCoins;
    float timeRemaining;
    int frameCount;

    bool hasScanner;
    int scannerUses;
    int scannerX[MAX_SCANNERS];
    int scannerY[MAX_SCANNERS];
    bool scannerRevealed[MAX_SCANNERS];
    int scannerCount;
    bool scannerInUse;
} GameState;

typedef enum {
    APP_MODE_TITLE,
    APP_MODE_TRANSITION,
    APP_MODE_PLAYING,
    APP_MODE_GAME_OVER
} AppMode;

#endif
