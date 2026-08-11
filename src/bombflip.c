#include <riv.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define SEQT_IMPL
#include "seqt.h"

// DEBUG MODE FLAG
#define DEBUG_MODE 0
// Constants
#define GRID_SIZE 6
#define TILE_SIZE 24
#define SCREEN_SIZE 256
#define BASE_TIME_PER_LEVEL 45
#define TIME_BONUS_PER_CARD 3
#define MAX_TIME 150
#define EXPLOSION_DURATION 60
#define EXPLOSION_RADIUS 50
#define FLIP_ANIMATION_FRAMES 15
#define PI 3.14159265358979323846
#define MAX_LEVEL 12
#define CHEAT_WIN_LEVEL RIV_GAMEPAD_R1
#define TIME_WARNING_THRESHOLD 10
#define TRANSITION_FRAMES 60
#define TRANSITION_EXTRA_FRAMES 10
#define FRAME_RADIUS 200
#define FRAME_THICKNESS 50
#define FRAME_ANIMATION_SPEED 0.05f
#define TITLE_EASTER_EGG_DELAY_FRAMES 3600
#define SCANNER_TILE_START_LEVEL 1
#define SCANNER_TILE_COLOR RIV_COLOR_PEACH
#define SCANNER_PREVIEW_FRAMES 30
#define CHAIN_EXPLOSION_DELAY_FRAMES 10
#define EASTER_EGG_COIN_PENALTY -1000000
#define NUCLEAR_EXPLOSION_DURATION 300 // 5 seconds at 60 FPS
seqt_source *backgroundMusic = NULL;
uint64_t backgroundMusicId = 0;

// Structs and Enums
typedef struct {
    int value;
    bool revealed;
    int flipFrame;
} Tile;

typedef enum {
    GAME_STATE_PLAYING,
    GAME_STATE_OVER_BOMB,
    GAME_STATE_OVER_COMPLETE,
    GAME_STATE_OVER_TIMEOUT,
    GAME_STATE_OVER_FOLD,
    GAME_STATE_OVER_EASTER_EGG
} GameEndState;

typedef struct {
    int x2Cards;
    int x3Cards;
    int bombs;
    int originalMinimumReward;
    int originalMaximumReward;
} LevelConfig;

typedef struct {
    Tile grid[GRID_SIZE][GRID_SIZE];
    int level;
    int totalCoins;
    int levelCoins;
    int rowTotals[GRID_SIZE];
    int columnTotals[GRID_SIZE];
    int rowBombs[GRID_SIZE];
    int columnBombs[GRID_SIZE];
    int cardsFlipped;
    int explosionFrame;
    int explosionX;
    int explosionY;
    bool levelCleared;
    bool bombRevealed;
    int bombRevealFrame;
    bool levelClearing;
    int levelClearDelay;
    int levelClearedTimer;
    bool transitioningToNextLevel;
    GameEndState gameOverState;
    int foldedCoins;
    float timeRemaining;
    int frameCount;
    bool musicPlaying;
    int timeBonus;
    bool hasScanner;
    int scannerUses;
    int scannerX[2];
    int scannerY[2];
    bool scannerRevealed[2];
    int scannerCount;
    int totalCardsFlipped;
    bool scannerInUse;
} GameState;

// Global variables
GameState game = {0};
bool gameStarted = false;
int flashTimer = 0;
int selectedX = 0;
int selectedY = 0;
int transitionFrame = 0;
bool transitioning = false;
float bombYOffset = 0.0f;
float bombBounceSpeed = 2.0f;
float bombBounceAmplitude = 5.0f;
float frameAnimation = 0.0f;
bool backgroundMusicPlaying = false;
int titleScreenTimer = 0;
bool titleExploded = false;
int titleExplosionRadius = 0;
bool gameOverTriggered = false;
int nuclearExplosionFrame = 0;

// The final two reward columns are original 2024 data and are intentionally unused.
const LevelConfig LEVEL_CONFIGS[MAX_LEVEL] = {{3, 1, 6, 24, 48},
                                              {4, 2, 7, 54, 108},
                                              {5, 3, 8, 96, 192},
                                              {6, 3, 8, 192, 384},
                                              {7, 4, 10, 288, 576},
                                              {8, 4, 10, 480, 960},
                                              {8, 5, 10, 720, 1440},
                                              {10, 5, 10, 1080, 2160},
                                              {7, 3, 13, 1500, 3000},
                                              {8, 3, 14, 2000, 4000},
                                              {9, 3, 15, 2500, 5000},
                                              {10, 3, 16, 3000, 6000}};

// Sound effects
riv_waveform_desc revealSound = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f,
    .decay = 0.01f,
    .sustain = 0.1f,
    .release = 0.01f,
    .start_frequency = RIV_NOTE_C4,
    .end_frequency = RIV_NOTE_C5,
    .amplitude = 0.25f,
    .sustain_level = 0.5f,
};

riv_waveform_desc gameOverSound = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f,
    .decay = 0.01f,
    .sustain = 0.2f,
    .release = 0.1f,
    .start_frequency = RIV_NOTE_A3,
    .end_frequency = RIV_NOTE_A2,
    .amplitude = 0.5f,
    .sustain_level = 0.5f,
};

riv_waveform_desc explosionSound = {
    .type = RIV_WAVEFORM_NOISE,
    .attack = 0.01f,
    .decay = 0.1f,
    .sustain = 0.2f,
    .release = 0.3f,
    .start_frequency = 100.0f,
    .end_frequency = 50.0f,
    .amplitude = 0.5f,
    .sustain_level = 0.3f,
};

riv_waveform_desc fanfareSounds[] = {{
                                         .id = 201,
                                         .type = RIV_WAVEFORM_TRIANGLE,
                                         .attack = 0.05f,
                                         .decay = 0.1f,
                                         .sustain = 0.2f,
                                         .release = 0.1f,
                                         .start_frequency = 523.25f,
                                         .end_frequency = 523.25f, // C5
                                         .amplitude = 0.25f,
                                         .sustain_level = 0.5f,
                                         .duty_cycle = 0.5f,
                                         .pan = -0.2f,
                                     },
                                     {
                                         .id = 202,
                                         .type = RIV_WAVEFORM_TRIANGLE,
                                         .attack = 0.05f,
                                         .decay = 0.1f,
                                         .sustain = 0.2f,
                                         .release = 0.1f,
                                         .start_frequency = 659.25f,
                                         .end_frequency = 659.25f, // E5
                                         .amplitude = 0.25f,
                                         .sustain_level = 0.5f,
                                         .duty_cycle = 0.5f,
                                         .pan = 0.2f,
                                     },
                                     {
                                         .id = 203,
                                         .type = RIV_WAVEFORM_TRIANGLE,
                                         .attack = 0.05f,
                                         .decay = 0.1f,
                                         .sustain = 0.3f,
                                         .release = 0.2f,
                                         .start_frequency = 783.99f,
                                         .end_frequency = 783.99f, // G5
                                         .amplitude = 0.25f,
                                         .sustain_level = 0.5f,
                                         .duty_cycle = 0.5f,
                                         .pan = 0.0f,
                                     },
                                     {
                                         .id = 204,
                                         .type = RIV_WAVEFORM_TRIANGLE,
                                         .attack = 0.05f,
                                         .decay = 0.1f,
                                         .sustain = 0.4f,
                                         .release = 0.3f,
                                         .start_frequency = 1046.50f,
                                         .end_frequency = 1046.50f, // C6
                                         .amplitude = 0.25f,
                                         .sustain_level = 0.5f,
                                         .duty_cycle = 0.5f,
                                         .pan = 0.0f,
                                     }};

riv_waveform_desc startGameSound = {
    .type = RIV_WAVEFORM_SQUARE,
    .attack = 0.01f,
    .decay = 0.05f,
    .sustain = 0.1f,
    .release = 0.1f,
    .start_frequency = RIV_NOTE_C5,
    .end_frequency = RIV_NOTE_G5,
    .amplitude = 0.3f,
    .sustain_level = 0.5f,
};

riv_waveform_desc enhancedExplosionSounds[] = {{
                                                   .type = RIV_WAVEFORM_NOISE,
                                                   .attack = 0.01f,
                                                   .decay = 0.1f,
                                                   .sustain = 0.3f,
                                                   .release = 0.5f,
                                                   .start_frequency = 100.0f,
                                                   .end_frequency = 50.0f,
                                                   .amplitude = 0.6f,
                                                   .sustain_level = 0.4f,
                                               },
                                               {
                                                   .type = RIV_WAVEFORM_NOISE,
                                                   .attack = 0.005f,
                                                   .decay = 0.05f,
                                                   .sustain = 0.2f,
                                                   .release = 0.3f,
                                                   .start_frequency = 200.0f,
                                                   .end_frequency = 100.0f,
                                                   .amplitude = 0.5f,
                                                   .sustain_level = 0.3f,
                                               },
                                               {
                                                   .type = RIV_WAVEFORM_NOISE,
                                                   .attack = 0.02f,
                                                   .decay = 0.15f,
                                                   .sustain = 0.4f,
                                                   .release = 0.6f,
                                                   .start_frequency = 80.0f,
                                                   .end_frequency = 40.0f,
                                                   .amplitude = 0.7f,
                                                   .sustain_level = 0.5f,
                                               }};

// Function prototypes
int getCurrentGridSize(void);
void resetLevelState(void);
void clearGrid(int gridSize);
void placeLevelCards(int gridSize, int bombCount, int x2Count, int x3Count);
void calculateClues(int gridSize);
void initializeLevel(void);
void drawGame(void);
void updateGame(void);
void updateOutcard(void);
void playTimerTick(void);
void triggerChainExplosion(void);
void pollBackgroundMusic(void);
void startBackgroundMusic(void);
void stopBackgroundMusic(void);
void playExplosionSound(void);
void playLevelClearFanfare(void);
void finishGame(GameEndState state);
void endWithBomb(void);
void drawEndScreen(void);
void drawFinalScore(int y);
void drawTitleScreen(void);
void drawStandardTitle(void);
void drawGrowingTitleExplosion(void);
void drawJungleScene(void);
void drawNuclearCloud(float progress);
void drawNuclearExplosion(void);
void drawTitleTransition(void);
void drawPixelatedBomb(int x, int y, int size);
void drawCoin(int x, int y, int size, int value);
bool allHighCardsFlipped(void);
void assignScannerTiles(void);
void useScanner(int x, int y);
void drawScannerOverlay(void);
void debugLog(const char *message);
void playEnhancedExplosionSound(void);
void drawMagnifyingGlass(int x, int y, int size, uint32_t color);
void updateFlipAnimations(int gridSize);
void updateLevelClearing(void);
void updateClearedLevel(void);
void startNextLevel(void);
bool updateTimer(void);
void moveSelection(int gridSize);
void revealSelectedCard(void);
bool handleFoldInput(void);
bool handleBoardInput(void);
void updateBombAnimation(void);
void updateExplosionAnimation(void);
bool scannerIsAvailable(void);
void handleScannerInput(void);
void selectLevelColors(uint32_t *backgroundColor, uint32_t *tileColor, uint32_t *revealedColor);
void drawTileContent(int centerX, int centerY, int size, int value);
void drawTile(
    int x, int y, int gridOffsetX, int gridOffsetY, uint32_t tileColor, uint32_t revealedColor);
void drawBoard(
    int gridSize, int gridOffsetX, int gridOffsetY, uint32_t tileColor, uint32_t revealedColor);
void drawClues(int gridSize, int gridOffsetX, int gridOffsetY);
void drawStatus(void);
void drawExplosion(int gridSize);
void drawLevelClearedPanel(void);
void configureConsole(void);
void initializeApplication(void);
bool loadBackgroundMusic(void);
void beginGameTransition(void);
void drawCurrentFrame(void);

// Function implementations
int getCurrentGridSize(void) {
    return game.level <= 8 ? 5 : 6;
}

void resetLevelState(void) {
    game.levelCoins = 0;
    game.cardsFlipped = 0;
    game.explosionFrame = 0;
    game.levelCleared = false;
    game.bombRevealed = false;
    game.bombRevealFrame = 0;
    game.levelClearing = false;
    game.levelClearDelay = 0;
    game.levelClearedTimer = 0;
    game.transitioningToNextLevel = false;
    game.timeBonus = 0;
    game.hasScanner = false;
    game.scannerUses = 0;
    game.scannerCount = 0;
    game.scannerInUse = false;

    for (int i = 0; i < 2; i++) {
        game.scannerX[i] = -1;
        game.scannerY[i] = -1;
        game.scannerRevealed[i] = false;
    }
}

void clearGrid(int gridSize) {
    for (int i = 0; i < GRID_SIZE; i++) {
        game.rowTotals[i] = 0;
        game.columnTotals[i] = 0;
        game.rowBombs[i] = 0;
        game.columnBombs[i] = 0;
    }

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            game.grid[y][x].value = x < gridSize && y < gridSize ? 1 : -1;
            game.grid[y][x].revealed = false;
            game.grid[y][x].flipFrame = 0;
        }
    }
}

void placeLevelCards(int gridSize, int bombCount, int x2Count, int x3Count) {
    if (x2Count == 0 && x3Count == 0) {
        x2Count = 1;
    }

    while (bombCount > 0 || x2Count > 0 || x3Count > 0) {
        int x = riv_rand_uint(gridSize - 1);
        int y = riv_rand_uint(gridSize - 1);

        if (game.grid[y][x].value == 1) {
            if (bombCount > 0) {
                game.grid[y][x].value = 0;
                bombCount--;
            } else if (x2Count > 0) {
                game.grid[y][x].value = 2;
                x2Count--;
            } else if (x3Count > 0) {
                game.grid[y][x].value = 3;
                x3Count--;
            }
        }
    }
}

void calculateClues(int gridSize) {
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            game.rowTotals[y] += game.grid[y][x].value;
            game.columnTotals[x] += game.grid[y][x].value;
            if (game.grid[y][x].value == 0) {
                game.rowBombs[y]++;
                game.columnBombs[x]++;
            }
        }
    }
}

void initializeLevel(void) {
    LevelConfig config = LEVEL_CONFIGS[game.level - 1];
    int gridSize = getCurrentGridSize();

    resetLevelState();
    clearGrid(gridSize);
    placeLevelCards(gridSize, config.bombs, config.x2Cards, config.x3Cards);
    calculateClues(gridSize);

    if (game.level >= SCANNER_TILE_START_LEVEL) {
        assignScannerTiles();
    }

    game.timeRemaining = (float)(BASE_TIME_PER_LEVEL + (game.level - 1) * 5);
    game.frameCount = 0;
    game.musicPlaying = true;

#if DEBUG_MODE
    char debugMessage[100];
    snprintf(debugMessage,
             sizeof(debugMessage),
             "Game initialized. Has scanner: %d, Scanner positions: (%d, %d) and (%d, %d)",
             game.hasScanner,
             game.scannerX[0],
             game.scannerY[0],
             game.scannerX[1],
             game.scannerY[1]);
    debugLog(debugMessage);
#endif
}

void drawCoin(int x, int y, int size, int value) {
    riv_draw_circle_fill(x, y, size / 2, RIV_COLOR_YELLOW);
    riv_draw_circle_line(x, y, size / 2, RIV_COLOR_ORANGE);
    char valueText[2];
    snprintf(valueText, sizeof(valueText), "%d", value);
    riv_draw_text(valueText, RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, x, y, 1, RIV_COLOR_BLACK);
}

void drawStandardTitle(void) {
    riv_clear(RIV_COLOR_BLUE);

    int centerX = SCREEN_SIZE / 2;
    int centerY = SCREEN_SIZE / 2;
    frameAnimation += FRAME_ANIMATION_SPEED;
    if (frameAnimation >= 1.0f) {
        frameAnimation -= 1.0f;
    }

    for (int i = 0; i < FRAME_THICKNESS; i++) {
        float progress = (float)i / FRAME_THICKNESS;
        float animatedProgress = fmodf(progress + frameAnimation, 1.0f);
        int radius = FRAME_RADIUS + i;
        uint32_t frameColor = i % 2 == 0 ? RIV_COLOR_BLUE : RIV_COLOR_LIGHTBLUE;

        if (animatedProgress < 0.5f) {
            riv_draw_circle_line(centerX, centerY, radius, frameColor);
        }
    }

    riv_draw_text("BOMB FLIP",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 3,
                  3,
                  RIV_COLOR_YELLOW);
    riv_draw_text("BETA VERSION",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE * 3 / 4 + 20,
                  1,
                  RIV_COLOR_RED);

    flashTimer++;
    if (flashTimer / 30 % 2 == 0) {
        riv_draw_text("Press Start",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      SCREEN_SIZE * 3 / 4,
                      1,
                      RIV_COLOR_WHITE);
    }

    bombYOffset = sinf(riv->frame * bombBounceSpeed * 0.1f) * bombBounceAmplitude;
    drawPixelatedBomb(SCREEN_SIZE / 2, SCREEN_SIZE / 2 + (int)bombYOffset, 48);

    titleScreenTimer++;
    if (titleScreenTimer >= TITLE_EASTER_EGG_DELAY_FRAMES) {
        titleExploded = true;
        titleExplosionRadius = 1;
        playEnhancedExplosionSound();
    }
}

void drawGrowingTitleExplosion(void) {
    riv_draw_circle_fill(SCREEN_SIZE / 2, SCREEN_SIZE / 2, titleExplosionRadius, RIV_COLOR_RED);
    riv_draw_circle_fill(
        SCREEN_SIZE / 2, SCREEN_SIZE / 2, titleExplosionRadius * 0.8, RIV_COLOR_ORANGE);
    riv_draw_circle_fill(
        SCREEN_SIZE / 2, SCREEN_SIZE / 2, titleExplosionRadius * 0.6, RIV_COLOR_YELLOW);
    titleExplosionRadius += 5;

    if (titleExplosionRadius == 1) {
        playEnhancedExplosionSound();
    }
    if (titleExplosionRadius % 50 == 0) {
        playEnhancedExplosionSound();
    }
}

void drawJungleScene(void) {
    for (int i = 0; i < 5; i++) {
        int treeX = -10 + i * 60;
        int treeY = SCREEN_SIZE * 2 / 3;
        int treeHeight = 80 + sinf(i * 1.5f) * 20;
        int trunkWidth = 6 + i % 3;
        riv_draw_rect_fill(
            treeX - trunkWidth / 2, treeY - treeHeight, trunkWidth, treeHeight, RIV_COLOR_BROWN);

        int canopyWidth = treeHeight / 2;
        int canopyHeight = treeHeight * 2 / 3;
        for (int j = 0; j < 20; j++) {
            int leafX = treeX + riv_rand() % canopyWidth - canopyWidth / 2;
            int leafY = treeY - treeHeight + riv_rand() % canopyHeight;
            int leafSize = 10 + riv_rand() % 10;
            uint32_t leafColor = j % 2 == 0 ? RIV_COLOR_GREEN : RIV_COLOR_DARKGREEN;
            riv_draw_circle_fill(leafX, leafY, leafSize, leafColor);
        }

        for (int j = 0; j < 5; j++) {
            int leafX = treeX + riv_rand() % canopyWidth - canopyWidth / 2;
            int leafY = treeY - treeHeight + riv_rand() % canopyHeight;
            int leafSize = 5 + riv_rand() % 5;
            riv_draw_circle_fill(leafX, leafY, leafSize, RIV_COLOR_LIGHTGREEN);
        }
    }

    int arcadeX = SCREEN_SIZE - 70;
    int arcadeY = SCREEN_SIZE * 2 / 3 - 60;
    int arcadeWidth = 50;
    int arcadeHeight = 60;
    riv_draw_rect_fill(arcadeX, arcadeY, arcadeWidth, arcadeHeight, RIV_COLOR_PURPLE);
    riv_draw_rect_fill(
        arcadeX + 2, arcadeY + 2, arcadeWidth - 4, arcadeHeight - 4, RIV_COLOR_DARKPURPLE);
    riv_draw_rect_fill(
        arcadeX + 5, arcadeY + 5, arcadeWidth - 10, arcadeHeight / 2, RIV_COLOR_BLACK);
    riv_draw_rect_line(
        arcadeX + 4, arcadeY + 4, arcadeWidth - 8, arcadeHeight / 2 + 2, RIV_COLOR_WHITE);
    riv_draw_text("RIVES",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  arcadeX + arcadeWidth / 2,
                  arcadeY + arcadeHeight / 4 + 2,
                  1,
                  RIV_COLOR_GREEN);

    int controlY = arcadeY + arcadeHeight * 3 / 4;
    riv_draw_circle_fill(arcadeX + arcadeWidth / 3, controlY, 4, RIV_COLOR_RED);
    riv_draw_circle_fill(arcadeX + arcadeWidth * 2 / 3, controlY, 4, RIV_COLOR_RED);
    riv_draw_rect_fill(arcadeX + 5, controlY + 10, arcadeWidth - 10, 5, RIV_COLOR_BLACK);

    int benchY = SCREEN_SIZE * 2 / 3 - 20;
    riv_draw_rect_fill(SCREEN_SIZE - 150, benchY, 60, 5, RIV_COLOR_BROWN);
    riv_draw_rect_fill(SCREEN_SIZE - 145, benchY + 5, 5, 15, RIV_COLOR_BROWN);
    riv_draw_rect_fill(SCREEN_SIZE - 95, benchY + 5, 5, 15, RIV_COLOR_BROWN);
}

void drawNuclearCloud(float progress) {
    float easedProgress = 1 - (1 - progress) * (1 - progress);
    int explosionSize = (int)(SCREEN_SIZE * 1.5 * easedProgress);
    int cloudBaseY = SCREEN_SIZE * 2 / 3;

    int shockwaveRadius = (int)(SCREEN_SIZE * 2 * progress);
    riv_draw_circle_line(SCREEN_SIZE / 2, cloudBaseY, shockwaveRadius, RIV_COLOR_WHITE);

    int stemHeight = explosionSize / 2;
    int cloudTopY = cloudBaseY - stemHeight;
    int stemWidth = 20 + (int)(20 * (1 - easedProgress));
    riv_draw_rect_fill(SCREEN_SIZE / 2 - stemWidth / 2,
                       cloudBaseY - stemHeight,
                       stemWidth,
                       stemHeight,
                       RIV_COLOR_GREY);

    int cloudRadius = explosionSize / 2;
    riv_draw_circle_fill(SCREEN_SIZE / 2, cloudTopY, cloudRadius, RIV_COLOR_GREY);
    for (int i = 0; i < 8; i++) {
        float angle = i * (PI / 4);
        int puffX = SCREEN_SIZE / 2 + cosf(angle) * cloudRadius * 0.8;
        int puffY = cloudTopY + sinf(angle) * cloudRadius * 0.8;
        int puffRadius = cloudRadius * 0.4;
        riv_draw_circle_fill(puffX, puffY, puffRadius, RIV_COLOR_GREY);
    }

    int lightRadius = cloudRadius / 2;
    riv_draw_circle_fill(SCREEN_SIZE / 2, cloudTopY, lightRadius, RIV_COLOR_YELLOW);
    riv_draw_circle_fill(SCREEN_SIZE / 2, cloudTopY, lightRadius * 2 / 3, RIV_COLOR_WHITE);

    for (int i = 0; i < 30; i++) {
        float debrisProgress = progress * 2 > 1.0f ? 1.0f : progress * 2;
        int debrisX = SCREEN_SIZE / 2 + sinf(i * 0.5f) * explosionSize / 2 * debrisProgress;
        int debrisY = cloudBaseY - i * 5 - stemHeight * debrisProgress;
        riv_draw_circle_fill(debrisX, debrisY, 2, RIV_COLOR_GREY);
    }
}

void drawNuclearExplosion(void) {
    float progress = (float)nuclearExplosionFrame / NUCLEAR_EXPLOSION_DURATION;

    if (nuclearExplosionFrame < 10) {
        uint32_t flashColor = nuclearExplosionFrame % 2 == 0 ? RIV_COLOR_WHITE : RIV_COLOR_ORANGE;
        riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, flashColor);
    } else {
        riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE * 2 / 3, RIV_COLOR_PEACH);
        riv_draw_rect_fill(0, SCREEN_SIZE * 2 / 3, SCREEN_SIZE, SCREEN_SIZE / 3, RIV_COLOR_BROWN);
        if (progress < 0.3) {
            drawJungleScene();
        }
        drawNuclearCloud(progress);
    }

    if (nuclearExplosionFrame > NUCLEAR_EXPLOSION_DURATION / 2) {
        riv_draw_text("BOOM!",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      SCREEN_SIZE / 4,
                      3,
                      RIV_COLOR_RED);
    }

    if (nuclearExplosionFrame == 1 || nuclearExplosionFrame == NUCLEAR_EXPLOSION_DURATION / 4 ||
        nuclearExplosionFrame == NUCLEAR_EXPLOSION_DURATION / 2 ||
        nuclearExplosionFrame == 3 * NUCLEAR_EXPLOSION_DURATION / 4) {
        playEnhancedExplosionSound();
    }

    nuclearExplosionFrame++;
    if (nuclearExplosionFrame >= NUCLEAR_EXPLOSION_DURATION) {
        finishGame(GAME_STATE_OVER_EASTER_EGG);
    }
}

void drawTitleScreen(void) {
    if (!titleExploded) {
        drawStandardTitle();
    } else if (titleExplosionRadius < FRAME_RADIUS * 4) {
        drawGrowingTitleExplosion();
    } else if (!gameOverTriggered) {
        drawNuclearExplosion();
    }
}

void drawTitleTransition(void) {
    drawGame();

    int overlayPosition = -((transitionFrame * SCREEN_SIZE) / TRANSITION_FRAMES);
    riv_draw_rect_fill(0, overlayPosition, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLUE);

    int titleY = overlayPosition + (SCREEN_SIZE / 2) - (SCREEN_SIZE / 6);
    riv_draw_text("BOMB FLIP",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  titleY,
                  3,
                  RIV_COLOR_YELLOW);

    bombYOffset = sinf(riv->frame * bombBounceSpeed * 0.1f) * bombBounceAmplitude;
    int bombY = overlayPosition + (SCREEN_SIZE / 2) + (int)bombYOffset;
    drawPixelatedBomb(SCREEN_SIZE / 2, bombY, 48);

    if (overlayPosition > -SCREEN_SIZE / 2) {
        flashTimer++;
        if (flashTimer / 30 % 2 == 0) {
            riv_draw_text("Press Start",
                          RIV_SPRITESHEET_FONT_5X7,
                          RIV_CENTER,
                          SCREEN_SIZE / 2,
                          overlayPosition + SCREEN_SIZE - 20,
                          1,
                          RIV_COLOR_WHITE);
        }
    }
}

void drawPixelatedBomb(int x, int y, int size) {
    int pixelSize = size / 6;

    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            if (dx * dx + dy * dy <= 10) {
                riv_draw_rect_fill(
                    x + dx * pixelSize, y + dy * pixelSize, pixelSize, pixelSize, RIV_COLOR_GREY);
            }
        }
    }

    riv_draw_rect_fill(
        x - pixelSize, y - 4 * pixelSize, pixelSize, 2 * pixelSize, RIV_COLOR_ORANGE);
    riv_draw_rect_fill(x, y - 5 * pixelSize, pixelSize, pixelSize, RIV_COLOR_ORANGE);

    riv_draw_rect_fill(x - 2 * pixelSize, y - pixelSize, pixelSize, pixelSize, RIV_COLOR_WHITE);
    riv_draw_rect_fill(x + pixelSize, y - pixelSize, pixelSize, pixelSize, RIV_COLOR_WHITE);

    riv_draw_rect_fill(
        x - 3 * pixelSize, y - 2 * pixelSize, 2 * pixelSize, pixelSize, RIV_COLOR_RED);
    riv_draw_rect_fill(x + pixelSize, y - 2 * pixelSize, 2 * pixelSize, pixelSize, RIV_COLOR_RED);
}

bool allHighCardsFlipped(void) {
    int currentGridSize = getCurrentGridSize();
    for (int y = 0; y < currentGridSize; y++) {
        for (int x = 0; x < currentGridSize; x++) {
            if ((game.grid[y][x].value == 2 || game.grid[y][x].value == 3) &&
                !game.grid[y][x].revealed) {
                return false;
            }
        }
    }
    return true;
}

void pollBackgroundMusic(void) {
    if (!backgroundMusicPlaying)
        return;
    seqt_poll();
}

void startBackgroundMusic(void) {
    backgroundMusicPlaying = true;
    backgroundMusicId = seqt_play(backgroundMusic, -1); // -1 means loop indefinitely
}

void stopBackgroundMusic(void) {
    backgroundMusicPlaying = false;
    if (backgroundMusicId != 0) {
        seqt_stop(backgroundMusicId);
        backgroundMusicId = 0;
    }
}

void playExplosionSound(void) {
    riv_waveform(&explosionSound);
}

void playLevelClearFanfare(void) {
    int fanfareCount = sizeof(fanfareSounds) / sizeof(fanfareSounds[0]);
    for (int i = 0; i < fanfareCount; i++) {
        riv_waveform_desc fanfare = fanfareSounds[i];
        fanfare.delay = i * 0.2f; // Add a slight delay between notes
        riv_waveform(&fanfare);
    }
}

void selectLevelColors(uint32_t *backgroundColor, uint32_t *tileColor, uint32_t *revealedColor) {
    *revealedColor = RIV_COLOR_LIGHTGREY;

    if (game.level <= 4) {
        *backgroundColor = RIV_COLOR_DARKGREEN;
        *tileColor = RIV_COLOR_LIGHTGREEN;
    } else if (game.level <= 8) {
        *backgroundColor = RIV_COLOR_BLUE;
        *tileColor = RIV_COLOR_LIGHTBLUE;
    } else if (game.level <= 10) {
        *backgroundColor = RIV_COLOR_DARKRED;
        *tileColor = RIV_COLOR_RED;
    } else {
        *backgroundColor = RIV_COLOR_DARKPURPLE;
        *tileColor = RIV_COLOR_PURPLE;
    }
}

void drawTileContent(int centerX, int centerY, int size, int value) {
    if (value == 0) {
        drawPixelatedBomb(centerX, centerY, size);
    } else {
        drawCoin(centerX, centerY, size, value);
    }
}

void drawTile(
    int x, int y, int gridOffsetX, int gridOffsetY, uint32_t tileColor, uint32_t revealedColor) {
    Tile *tile = &game.grid[y][x];
    int tileX = x * TILE_SIZE + gridOffsetX;
    int tileY = y * TILE_SIZE + gridOffsetY;

    if (tile->flipFrame > 0 || (game.bombRevealed && tile->value == 0)) {
        float progress = tile->value == 0 && game.bombRevealed
                             ? (float)game.bombRevealFrame / FLIP_ANIMATION_FRAMES
                             : (float)tile->flipFrame / FLIP_ANIMATION_FRAMES;
        float angle = progress * PI;
        int width = (int)(TILE_SIZE * fabsf(cosf(angle)));

        if (progress < 0.5f) {
            riv_draw_rect_fill(tileX + (TILE_SIZE - width) / 2, tileY, width, TILE_SIZE, tileColor);
        } else {
            riv_draw_rect_fill(
                tileX + (TILE_SIZE - width) / 2, tileY, width, TILE_SIZE, revealedColor);
            float scale = (float)width / TILE_SIZE;
            drawTileContent(tileX + TILE_SIZE / 2,
                            tileY + TILE_SIZE / 2,
                            (int)(TILE_SIZE * 3 / 4 * scale),
                            tile->value);
        }
    } else if (tile->revealed || game.levelClearing || game.levelCleared) {
        riv_draw_rect_fill(tileX, tileY, TILE_SIZE, TILE_SIZE, revealedColor);
        drawTileContent(
            tileX + TILE_SIZE / 2, tileY + TILE_SIZE / 2, TILE_SIZE * 3 / 4, tile->value);
    } else {
        riv_draw_rect_fill(tileX, tileY, TILE_SIZE, TILE_SIZE, tileColor);
    }

    if (x == selectedX && y == selectedY) {
        bool scannerActive = scannerIsAvailable();
        uint32_t highlightColor = scannerActive ? SCANNER_TILE_COLOR : RIV_COLOR_WHITE;
        riv_draw_rect_line(tileX, tileY, TILE_SIZE, TILE_SIZE, highlightColor);

        if (scannerActive && !game.scannerInUse && !tile->revealed) {
            drawMagnifyingGlass(tileX + TILE_SIZE * 2 / 6,
                                tileY + TILE_SIZE * 2 / 6,
                                TILE_SIZE,
                                SCANNER_TILE_COLOR);
        }
    } else {
        riv_draw_rect_line(tileX, tileY, TILE_SIZE, TILE_SIZE, RIV_COLOR_BLACK);
    }
}

void drawBoard(
    int gridSize, int gridOffsetX, int gridOffsetY, uint32_t tileColor, uint32_t revealedColor) {
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            drawTile(x, y, gridOffsetX, gridOffsetY, tileColor, revealedColor);
        }
    }

    for (int i = 0; i < game.scannerCount; i++) {
        int x = game.scannerX[i];
        int y = game.scannerY[i];
        Tile *tile = &game.grid[y][x];

        if (tile->revealed && game.hasScanner) {
            int tileX = x * TILE_SIZE + gridOffsetX;
            int tileY = y * TILE_SIZE + gridOffsetY;
            riv_draw_rect_fill(tileX, tileY, TILE_SIZE, TILE_SIZE, SCANNER_TILE_COLOR);
            drawCoin(tileX + TILE_SIZE / 2, tileY + TILE_SIZE / 2, TILE_SIZE * 3 / 4, tile->value);
        }
    }
}

void drawClues(int gridSize, int gridOffsetX, int gridOffsetY) {
    int boardSize = gridSize * TILE_SIZE;

    for (int i = 0; i < gridSize; i++) {
        char clueText[16];
        snprintf(clueText, sizeof(clueText), "%d:%d", game.rowTotals[i], game.rowBombs[i]);
        riv_draw_text(clueText,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_LEFT,
                      gridOffsetX + boardSize + 5,
                      gridOffsetY + i * TILE_SIZE + TILE_SIZE / 2,
                      1,
                      RIV_COLOR_WHITE);

        snprintf(clueText, sizeof(clueText), "%d:%d", game.columnTotals[i], game.columnBombs[i]);
        riv_draw_text(clueText,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      gridOffsetX + i * TILE_SIZE + TILE_SIZE / 2,
                      gridOffsetY + boardSize + 5,
                      1,
                      RIV_COLOR_WHITE);
    }
}

void drawStatus(void) {
    char levelText[32];
    snprintf(levelText, sizeof(levelText), "Level: %d", game.level);
    riv_draw_text(levelText, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 5, 5, 1, RIV_COLOR_WHITE);

    char timerText[32];
    int seconds = (int)game.timeRemaining;
    int minutes = seconds / 60;
    seconds %= 60;
    snprintf(timerText, sizeof(timerText), "Time: %02d:%02d", minutes, seconds);
    riv_draw_text(timerText, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 5, 15, 1, RIV_COLOR_WHITE);

    if (game.timeRemaining <= TIME_WARNING_THRESHOLD && game.frameCount % 30 < 15) {
        riv_draw_text(timerText, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 5, 15, 1, RIV_COLOR_RED);
    }

    char totalCoinsText[32];
    snprintf(totalCoinsText, sizeof(totalCoinsText), "Total Coins: %d", game.totalCoins);
    riv_draw_text(totalCoinsText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPRIGHT,
                  SCREEN_SIZE - 5,
                  5,
                  1,
                  RIV_COLOR_WHITE);

    char levelCoinsText[32];
    snprintf(levelCoinsText, sizeof(levelCoinsText), "Level Coins: %d", game.levelCoins);
    riv_draw_text(levelCoinsText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_BOTTOMLEFT,
                  5,
                  SCREEN_SIZE - 5,
                  1,
                  RIV_COLOR_YELLOW);
}

void drawExplosion(int gridSize) {
    if (game.explosionFrame <= 0) {
        return;
    }

    int offsetX = (SCREEN_SIZE - gridSize * TILE_SIZE) / 2;
    int offsetY = (SCREEN_SIZE - gridSize * TILE_SIZE) / 2;
    int explosionX = offsetX + game.explosionX;
    int explosionY = offsetY + game.explosionY;

    drawPixelatedBomb(explosionX, explosionY, TILE_SIZE);

    if (game.explosionFrame > 5) {
        float progress = (float)(game.explosionFrame - 5) / EXPLOSION_DURATION;
        int radius = (int)(EXPLOSION_RADIUS * progress);
        uint32_t color = RIV_COLOR_RED;
        if (progress > 0.3f) {
            color = RIV_COLOR_ORANGE;
        }
        if (progress > 0.6f) {
            color = RIV_COLOR_YELLOW;
        }
        riv_draw_circle_line(explosionX, explosionY, radius, color);
        riv_draw_circle_line(explosionX, explosionY, radius - 2, color);
    }

    int boomX = explosionX;
    int boomY = explosionY - EXPLOSION_RADIUS - 15;
    int bubbleWidth = 80;
    int bubbleHeight = 40;
    float pulse = sinf(game.explosionFrame * 0.2f) * 0.2f + 1.0f;
    int scaledWidth = (int)(bubbleWidth * pulse);
    int scaledHeight = (int)(bubbleHeight * pulse);

    riv_draw_rect_fill(boomX - scaledWidth / 2,
                       boomY - scaledHeight / 2,
                       scaledWidth,
                       scaledHeight,
                       RIV_COLOR_WHITE);
    riv_draw_rect_line(boomX - scaledWidth / 2,
                       boomY - scaledHeight / 2,
                       scaledWidth,
                       scaledHeight,
                       RIV_COLOR_BLACK);

    int tailWidth = 20;
    int tailHeight = 20;
    riv_draw_triangle_fill(boomX - tailWidth / 2,
                           boomY + scaledHeight / 2,
                           boomX + tailWidth / 2,
                           boomY + scaledHeight / 2,
                           boomX,
                           boomY + scaledHeight / 2 + tailHeight,
                           RIV_COLOR_WHITE);
    riv_draw_triangle_line(boomX - tailWidth / 2,
                           boomY + scaledHeight / 2,
                           boomX + tailWidth / 2,
                           boomY + scaledHeight / 2,
                           boomX,
                           boomY + scaledHeight / 2 + tailHeight,
                           RIV_COLOR_BLACK);
    riv_draw_text("BOOM!", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, boomX, boomY, 2, RIV_COLOR_RED);
}

void drawLevelClearedPanel(void) {
    if (!game.levelCleared) {
        return;
    }

    char levelClearedText[32];
    char coinsEarnedText[32];
    char timeBonusText[32];
    char totalEarnedText[32];
    snprintf(levelClearedText, sizeof(levelClearedText), "Level %d Cleared!", game.level);
    snprintf(coinsEarnedText,
             sizeof(coinsEarnedText),
             "Coins Earned: %d",
             game.levelCoins - game.timeBonus);
    snprintf(timeBonusText, sizeof(timeBonusText), "Time Bonus: %d", game.timeBonus);
    snprintf(totalEarnedText, sizeof(totalEarnedText), "Total Earned: %d", game.levelCoins);

    riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLACK);
    riv_draw_text(levelClearedText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 4,
                  2,
                  RIV_COLOR_YELLOW);
    riv_draw_text(coinsEarnedText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 - 20,
                  1,
                  RIV_COLOR_WHITE);
    riv_draw_text(timeBonusText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2,
                  1,
                  RIV_COLOR_WHITE);
    riv_draw_text(totalEarnedText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 + 20,
                  1,
                  RIV_COLOR_GREEN);
}

void drawGame(void) {
    uint32_t backgroundColor, tileColor, revealedColor;
    selectLevelColors(&backgroundColor, &tileColor, &revealedColor);
    riv_clear(backgroundColor);

    int currentGridSize = getCurrentGridSize();
    int gridSize = currentGridSize * TILE_SIZE;
    int gridOffsetX = (SCREEN_SIZE - gridSize) / 2;
    int gridOffsetY = (SCREEN_SIZE - gridSize) / 2;
    drawBoard(currentGridSize, gridOffsetX, gridOffsetY, tileColor, revealedColor);

    drawClues(currentGridSize, gridOffsetX, gridOffsetY);
    drawStatus();

    drawExplosion(currentGridSize);
    drawLevelClearedPanel();

    // Draw "Select to fold" text only when the game board is visible
    if (!game.levelCleared && !game.levelClearing) {
        riv_draw_text("SELECT to fold",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_BOTTOMRIGHT,
                      SCREEN_SIZE - 5,
                      SCREEN_SIZE - 5,
                      1,
                      RIV_COLOR_WHITE);
    }

    // Draw scanner overlay if active and level is not cleared
    if (game.hasScanner && game.scannerRevealed[0] && game.scannerUses > 0 && !game.levelCleared &&
        !game.levelClearing) {
        drawScannerOverlay();
    }
}

void drawEndScreen(void) {
    drawGame();
    riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLACK);

    char mainMessage[64];
    char subMessage[64];
    char coinMessage[64];
    uint32_t mainColor;

    switch (game.gameOverState) {
    case GAME_STATE_OVER_COMPLETE:
        snprintf(mainMessage, sizeof(mainMessage), "CONGRATULATIONS!");
        snprintf(subMessage, sizeof(subMessage), "You've completed all levels!");
        mainColor = RIV_COLOR_YELLOW;
        break;
    case GAME_STATE_OVER_BOMB:
        snprintf(mainMessage, sizeof(mainMessage), "GAME OVER");
        snprintf(subMessage, sizeof(subMessage), "You hit a bomb! Your coins went boom!");
        mainColor = RIV_COLOR_RED;
        break;
    case GAME_STATE_OVER_FOLD:
        snprintf(mainMessage, sizeof(mainMessage), "YOU FOLDED!");
        snprintf(subMessage, sizeof(subMessage), "Level coins retained: %d", game.foldedCoins);
        mainColor = RIV_COLOR_YELLOW;
        break;
    case GAME_STATE_OVER_TIMEOUT:
        snprintf(mainMessage, sizeof(mainMessage), "TIME'S UP!");
        snprintf(subMessage, sizeof(subMessage), "All your coins went KABOOM!");
        mainColor = RIV_COLOR_RED;
        break;
    case GAME_STATE_OVER_EASTER_EGG:
        snprintf(mainMessage, sizeof(mainMessage), "EASTER EGG!");
        snprintf(subMessage, sizeof(subMessage), "You found the secret explosion!");
        snprintf(coinMessage, sizeof(coinMessage), "Coin Penalty: %d", EASTER_EGG_COIN_PENALTY);
        mainColor = RIV_COLOR_PURPLE;
        break;
    default:
        snprintf(mainMessage, sizeof(mainMessage), "GAME OVER");
        subMessage[0] = '\0';
        mainColor = RIV_COLOR_RED;
        break;
    }

    // Define fixed positions for all elements
    int mainMessageY = SCREEN_SIZE / 4;
    int subMessageY = SCREEN_SIZE / 2;
    int scoreY = 3 * SCREEN_SIZE / 4;

    // Draw main message
    riv_draw_text(mainMessage,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  mainMessageY,
                  2,
                  mainColor);

    // Draw sub message
    riv_draw_text(subMessage,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  subMessageY,
                  1,
                  RIV_COLOR_WHITE);

    if (game.gameOverState == GAME_STATE_OVER_EASTER_EGG) {
        riv_draw_text(coinMessage,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      scoreY,
                      1,
                      RIV_COLOR_RED);
        return;
    }

    drawFinalScore(scoreY);
}

void drawFinalScore(int y) {
    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "Final Coins: %d", game.totalCoins);
    riv_draw_text(
        scoreText, RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, SCREEN_SIZE / 2, y, 1, RIV_COLOR_WHITE);
}

void finishGame(GameEndState state) {
    game.gameOverState = state;
    gameOverTriggered = true;
    stopBackgroundMusic();

    riv->quit_frame = riv->frame + 3 * riv->target_fps;

    switch (state) {
    case GAME_STATE_OVER_COMPLETE:
        playLevelClearFanfare();
        break;
    case GAME_STATE_OVER_BOMB:
        riv_waveform(&gameOverSound);
        break;
    case GAME_STATE_OVER_FOLD:
        game.foldedCoins = game.levelCoins / 2;
        game.totalCoins += game.foldedCoins;
        riv_waveform(&revealSound);
        break;
    case GAME_STATE_OVER_TIMEOUT:
        game.totalCoins = 0;
        game.levelCoins = 0;
        riv_waveform(&gameOverSound);
        break;
    case GAME_STATE_OVER_EASTER_EGG:
        game.totalCoins += EASTER_EGG_COIN_PENALTY; // Add the penalty to existing coins
        game.levelCoins = EASTER_EGG_COIN_PENALTY;
        riv_waveform(&gameOverSound);
        break;
    default:
        break;
    }

    // Update the score immediately after handling the game over state
    updateOutcard();
}

void endWithBomb(void) {
    finishGame(GAME_STATE_OVER_BOMB);
}

void updateFlipAnimations(int gridSize) {
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            int *flipFrame = &game.grid[y][x].flipFrame;
            if (*flipFrame > 0 && *flipFrame < FLIP_ANIMATION_FRAMES) {
                (*flipFrame)++;
            }
        }
    }
}

void updateLevelClearing(void) {
    int gridSize = getCurrentGridSize();
    bool allCardsRevealed = true;

    updateFlipAnimations(gridSize);

    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            Tile *tile = &game.grid[y][x];
            if (!tile->revealed) {
                tile->revealed = true;
                tile->flipFrame = 1;
                game.cardsFlipped++;
                allCardsRevealed = false;
            } else if (tile->flipFrame < FLIP_ANIMATION_FRAMES) {
                allCardsRevealed = false;
            }
        }
    }

    if (!allCardsRevealed) {
        return;
    }

    if (game.levelClearDelay > 0) {
        game.levelClearDelay--;
        return;
    }

    game.levelClearing = false;
    game.levelCleared = true;
    game.levelClearedTimer = 180;
    game.timeBonus = (int)(game.timeRemaining * 10);
    game.totalCoins += game.timeBonus;
    game.levelCoins += game.timeBonus;
}

void updateClearedLevel(void) {
    if (game.levelClearedTimer > 0) {
        game.levelClearedTimer--;
    } else if (game.level == MAX_LEVEL) {
        finishGame(GAME_STATE_OVER_COMPLETE);
    } else {
        game.transitioningToNextLevel = true;
        game.levelCleared = false;
    }
}

void startNextLevel(void) {
    game.level++;
    initializeLevel();
    game.transitioningToNextLevel = false;
    startBackgroundMusic();
}

bool updateTimer(void) {
    float previousTime = game.timeRemaining;
    game.timeRemaining -= 1.0f / 60.0f;

    if (game.timeRemaining <= 0) {
        stopBackgroundMusic();
        triggerChainExplosion();
        finishGame(GAME_STATE_OVER_TIMEOUT);
        return false;
    }

    if (game.timeRemaining <= TIME_WARNING_THRESHOLD) {
        if (game.musicPlaying) {
            stopBackgroundMusic();
            game.musicPlaying = false;
        }

        if ((int)previousTime != (int)game.timeRemaining) {
            playTimerTick();
        }
    } else if (!game.musicPlaying) {
        startBackgroundMusic();
        game.musicPlaying = true;
    }

    return true;
}

void moveSelection(int gridSize) {
    if (riv->keys[RIV_GAMEPAD_UP].press) {
        selectedY = (selectedY - 1 + gridSize) % gridSize;
    } else if (riv->keys[RIV_GAMEPAD_DOWN].press) {
        selectedY = (selectedY + 1) % gridSize;
    } else if (riv->keys[RIV_GAMEPAD_LEFT].press) {
        selectedX = (selectedX - 1 + gridSize) % gridSize;
    } else if (riv->keys[RIV_GAMEPAD_RIGHT].press) {
        selectedX = (selectedX + 1) % gridSize;
    }
}

void revealSelectedCard(void) {
    Tile *tile = &game.grid[selectedY][selectedX];
    if (tile->revealed) {
        return;
    }

    tile->revealed = true;
    tile->flipFrame = 1;
    game.cardsFlipped++;
    game.totalCardsFlipped++;

#if DEBUG_MODE
    char debugMessage[100];
    snprintf(debugMessage,
             sizeof(debugMessage),
             "Tile revealed at (%d, %d) with value %d",
             selectedX,
             selectedY,
             tile->value);
    debugLog(debugMessage);
#endif

    for (int i = 0; i < game.scannerCount; i++) {
        if (selectedX == game.scannerX[i] && selectedY == game.scannerY[i]) {
            game.scannerRevealed[i] = true;
            game.scannerUses += tile->value;
#if DEBUG_MODE
            snprintf(debugMessage,
                     sizeof(debugMessage),
                     "Scanner tile %d revealed. Total uses now: %d",
                     i + 1,
                     game.scannerUses);
            debugLog(debugMessage);
#endif
            break;
        }
    }

    if (tile->value == 0) {
        game.bombRevealed = true;
        game.bombRevealFrame = 1;
        game.explosionX = selectedX * TILE_SIZE + TILE_SIZE / 2;
        game.explosionY = selectedY * TILE_SIZE + TILE_SIZE / 2;
        stopBackgroundMusic();
        return;
    }

    game.levelCoins += tile->value * 100;
    riv_waveform(&revealSound);
    game.timeRemaining += TIME_BONUS_PER_CARD * tile->value;
    if (game.timeRemaining > MAX_TIME) {
        game.timeRemaining = MAX_TIME;
    }

    if (allHighCardsFlipped()) {
        game.totalCoins += game.levelCoins;
        game.levelClearing = true;
        game.levelClearDelay = 60;
        stopBackgroundMusic();
        playLevelClearFanfare();
    }
}

bool handleFoldInput(void) {
    if (!riv->keys[RIV_GAMEPAD_SELECT].press) {
        return false;
    }

    game.foldedCoins = game.levelCoins / 2;
    int foldCoins = game.totalCoins + game.foldedCoins;
    char foldMessage[64];
    snprintf(foldMessage, sizeof(foldMessage), "Fold now for %d coins?", foldCoins);

    while (riv_present()) {
        riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLACK);
        riv_draw_text(foldMessage,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      SCREEN_SIZE / 2 - 20,
                      1,
                      RIV_COLOR_WHITE);
        riv_draw_text("SELECT: Fold   START: Continue",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      SCREEN_SIZE / 2 + 20,
                      1,
                      RIV_COLOR_WHITE);

        if (riv->keys[RIV_GAMEPAD_SELECT].press) {
            finishGame(GAME_STATE_OVER_FOLD);
            return true;
        } else if (riv->keys[RIV_GAMEPAD_START].press) {
            return true;
        }
    }

    return false;
}

bool handleBoardInput(void) {
    if (game.explosionFrame > 0 || game.bombRevealed) {
        return false;
    }

    moveSelection(getCurrentGridSize());
    if (riv->keys[RIV_GAMEPAD_A1].press) {
        revealSelectedCard();
    }

    return handleFoldInput();
}

void updateBombAnimation(void) {
    if (!game.bombRevealed) {
        return;
    }

    if (game.bombRevealFrame < FLIP_ANIMATION_FRAMES) {
        game.bombRevealFrame++;
    } else if (game.bombRevealFrame == FLIP_ANIMATION_FRAMES) {
        game.explosionFrame = 1;
        playExplosionSound();
        game.bombRevealFrame++;
    }
}

void updateExplosionAnimation(void) {
    if (game.explosionFrame <= 0) {
        return;
    }

    if (game.explosionFrame < EXPLOSION_DURATION) {
        game.explosionFrame++;
    } else {
        endWithBomb();
    }
}

bool scannerIsAvailable(void) {
    return game.hasScanner && game.scannerRevealed[0] && game.scannerUses > 0 &&
           !allHighCardsFlipped();
}

void handleScannerInput(void) {
    if (!scannerIsAvailable() || !riv->keys[RIV_GAMEPAD_A2].press) {
        return;
    }

#if DEBUG_MODE
    char debugMessage[100];
    snprintf(debugMessage,
             sizeof(debugMessage),
             "Scanner use attempted. Uses left: %d",
             game.scannerUses);
    debugLog(debugMessage);
#endif
    useScanner(selectedX, selectedY);
}

void updateGame(void) {
    game.frameCount++;

    if (game.levelClearing) {
        updateLevelClearing();
        return;
    }

    if (game.levelCleared) {
        updateClearedLevel();
        return;
    }

    if (game.transitioningToNextLevel) {
        startNextLevel();
        return;
    }

    if (!updateTimer()) {
        return;
    }

    if (handleBoardInput()) {
        return;
    }

    updateFlipAnimations(getCurrentGridSize());
    updateBombAnimation();
    updateExplosionAnimation();

#if DEBUG_MODE
    int currentGridSize = getCurrentGridSize();
    if (riv->keys[CHEAT_WIN_LEVEL].press) {
        // Reveal all non-bomb tiles
        for (int y = 0; y < currentGridSize; y++) {
            for (int x = 0; x < currentGridSize; x++) {
                if (game.grid[y][x].value > 0) {
                    game.grid[y][x].revealed = true;
                    game.grid[y][x].flipFrame = FLIP_ANIMATION_FRAMES;
                    game.cardsFlipped++;
                    game.levelCoins += game.grid[y][x].value * 100;
                }
            }
        }
        // Update total coins
        game.totalCoins += game.levelCoins;
        game.levelClearing = true;
        game.levelClearDelay = 60;
        stopBackgroundMusic();
        playLevelClearFanfare();
    }
#endif

    handleScannerInput();
}

void updateOutcard(void) {
    riv->outcard_len =
        riv_snprintf((char *)riv->outcard,
                     RIV_SIZE_OUTCARD,
                     "JSON{\"score\":%d,\"level\":%d,\"cards_flipped\":%d,\"time_remaining\":%.2f}",
                     game.totalCoins,
                     game.level,
                     game.totalCardsFlipped,
                     game.timeRemaining);
}

void playTimerTick(void) {
    riv_waveform_desc tickSound = {
        .type = RIV_WAVEFORM_SQUARE,
        .attack = 0.01f,
        .decay = 0.01f,
        .sustain = 0.05f,
        .release = 0.01f,
        .start_frequency = 1000,
        .end_frequency = 1000,
        .amplitude = 0.2f,
        .sustain_level = 0.2f,
    };
    riv_waveform(&tickSound);
}

void drawMagnifyingGlass(int x, int y, int size, uint32_t color) {
    int lensRadius = size / 2;
    int handleLength = size / 3;
    int thickness = size / 10;

    // Draw the lens (circle) with multiple circles for thickness
    for (int i = 0; i < thickness; i++) {
        riv_draw_circle_line(x, y, lensRadius - i, color);
    }

    // Calculate the start and end points of the handle
    float angle = PI / 4.0f; // 45 degrees in radians
    int handleStartX = x + lensRadius * cosf(angle);
    int handleStartY = y + lensRadius * sinf(angle);

    // Rotate the rectangle to align with the 45-degree angle
    riv_draw_box_fill(handleStartX, handleStartY, handleLength, thickness, angle, color);
}

void triggerChainExplosion(void) {
    int currentGridSize = getCurrentGridSize();
    for (int y = 0; y < currentGridSize; y++) {
        for (int x = 0; x < currentGridSize; x++) {
            if (game.grid[y][x].value == 0) { // If it's a bomb
                game.grid[y][x].revealed = true;
                game.explosionX = x * TILE_SIZE + TILE_SIZE / 2;
                game.explosionY = y * TILE_SIZE + TILE_SIZE / 2;
                game.explosionFrame = 1;
                playExplosionSound();

                for (int i = 0; i < CHAIN_EXPLOSION_DELAY_FRAMES; i++) {
                    drawGame();
                    riv_present();
                }
            }
        }
    }
}

void assignScannerTiles(void) {
    int currentGridSize = getCurrentGridSize();
    int scannersToAssign = (game.level >= 9) ? 2 : 1;
    game.scannerCount = 0;

    for (int i = 0; i < scannersToAssign; i++) {
        int attempts = 0;
        while (attempts < 100) {
            int x = riv_rand_uint(currentGridSize - 1);
            int y = riv_rand_uint(currentGridSize - 1);
            if (game.grid[y][x].value > 0) {
                bool alreadyAssigned = false;
                for (int j = 0; j < game.scannerCount; j++) {
                    if (game.scannerX[j] == x && game.scannerY[j] == y) {
                        alreadyAssigned = true;
                        break;
                    }
                }
                if (!alreadyAssigned) {
                    game.scannerX[game.scannerCount] = x;
                    game.scannerY[game.scannerCount] = y;
                    game.scannerRevealed[game.scannerCount] = false;
                    game.scannerCount++;
                    game.hasScanner = true;
#if DEBUG_MODE
                    char debugMessage[100];
                    snprintf(debugMessage,
                             sizeof(debugMessage),
                             "Scanner tile %d assigned to (%d, %d) with value %d",
                             game.scannerCount,
                             x,
                             y,
                             game.grid[y][x].value);
                    debugLog(debugMessage);
#endif
                    break;
                }
            }
            attempts++;
        }
    }
#if DEBUG_MODE
    if (game.scannerCount < scannersToAssign) {
        debugLog("Failed to assign all scanner tiles after 100 attempts");
    }
#endif
}

void useScanner(int x, int y) {
#if DEBUG_MODE
    char debugMessage[100];
    snprintf(debugMessage, sizeof(debugMessage), "Using scanner on tile (%d, %d)", x, y);
    debugLog(debugMessage);
#endif
    if (!game.grid[y][x].revealed) {
        game.scannerInUse = true;

        // Flip animation
        game.grid[y][x].flipFrame = 1;

        // Wait for flip animation to complete
        while (game.grid[y][x].flipFrame < FLIP_ANIMATION_FRAMES) {
            game.grid[y][x].flipFrame++;
            drawGame();
            riv_present();
        }

#if DEBUG_MODE
        snprintf(debugMessage,
                 sizeof(debugMessage),
                 "Scanner revealed tile value: %d",
                 game.grid[y][x].value);
        debugLog(debugMessage);
#endif

        // Show the tile content briefly
        for (int i = 0; i < SCANNER_PREVIEW_FRAMES; i++) {
            drawGame();
            riv_present();
        }

        // Flip back animation
        while (game.grid[y][x].flipFrame > 0) {
            game.grid[y][x].flipFrame--;
            drawGame();
            riv_present();
        }

        game.scannerUses--;
        game.scannerInUse = false;
#if DEBUG_MODE
        snprintf(debugMessage,
                 sizeof(debugMessage),
                 "Scanner use complete. Remaining uses: %d",
                 game.scannerUses);
        debugLog(debugMessage);
#endif
    }
}

void drawScannerOverlay(void) {
    char usesText[32];
    snprintf(usesText, sizeof(usesText), "Scanner Uses: %d", game.scannerUses);
    riv_draw_text(usesText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_BOTTOMLEFT,
                  5,
                  SCREEN_SIZE - 20,
                  1,
                  SCANNER_TILE_COLOR);

    // Add instructions for using the scanner
    riv_draw_text("Press A2 to use scanner",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_BOTTOMRIGHT,
                  SCREEN_SIZE - 5,
                  SCREEN_SIZE - 20,
                  1,
                  SCANNER_TILE_COLOR);
}

void debugLog(const char *message) {
#if DEBUG_MODE
    printf("DEBUG: %s\n", message);
#else
    (void)message;
#endif
}

void playEnhancedExplosionSound(void) {
    int soundCount = sizeof(enhancedExplosionSounds) / sizeof(enhancedExplosionSounds[0]);
    for (int i = 0; i < soundCount; i++) {
        riv_waveform_desc sfx = enhancedExplosionSounds[i];
        sfx.delay = i * 0.1f; // Add a slight delay between each sound
        riv_waveform(&sfx);
    }
}

void configureConsole(void) {
    riv->width = SCREEN_SIZE;
    riv->height = SCREEN_SIZE;
    riv->target_fps = 60;

    riv->tracked_keys[RIV_GAMEPAD_UP] = true;
    riv->tracked_keys[RIV_GAMEPAD_DOWN] = true;
    riv->tracked_keys[RIV_GAMEPAD_LEFT] = true;
    riv->tracked_keys[RIV_GAMEPAD_RIGHT] = true;
    riv->tracked_keys[RIV_GAMEPAD_A1] = true;
    riv->tracked_keys[RIV_GAMEPAD_A2] = true;
    riv->tracked_keys[RIV_GAMEPAD_START] = true;
    riv->tracked_keys[RIV_GAMEPAD_SELECT] = true;

#if DEBUG_MODE
    riv->tracked_keys[CHEAT_WIN_LEVEL] = true;
#endif
}

void initializeApplication(void) {
    game.level = 1;
    game.totalCoins = 0;
    game.totalCardsFlipped = 0;
    game.gameOverState = GAME_STATE_PLAYING;
    initializeLevel();
    updateOutcard();
}

bool loadBackgroundMusic(void) {
    seqt_init();
    backgroundMusic = seqt_make_source_from_file("songs/gameplay.rivcard");
    if (!backgroundMusic) {
        printf("Failed to load background music\n");
        return false;
    }
    return true;
}

void beginGameTransition(void) {
    initializeLevel();
    transitioning = true;
    transitionFrame = 0;
    riv_waveform(&startGameSound);
    titleScreenTimer = 0;
    titleExploded = false;
}

void drawCurrentFrame(void) {
    if (!gameStarted) {
        if (riv->keys[RIV_GAMEPAD_START].press && !transitioning && !gameOverTriggered) {
            beginGameTransition();
        }

        if (transitioning) {
            drawTitleTransition();
            transitionFrame++;
            if (transitionFrame >= TRANSITION_FRAMES + TRANSITION_EXTRA_FRAMES) {
                gameStarted = true;
                transitioning = false;
                startBackgroundMusic();
            }
        } else if (!gameOverTriggered) {
            drawTitleScreen();
        } else {
            drawEndScreen();
        }
    } else {
        if (gameOverTriggered) {
            drawEndScreen();
        } else {
            updateGame();
            if (game.gameOverState != GAME_STATE_PLAYING) {
                drawEndScreen();
            } else {
                drawGame();
            }
            updateOutcard();
        }
    }
}

int main(void) {
    configureConsole();
    initializeApplication();
    if (!loadBackgroundMusic()) {
        return 1;
    }

    while (riv_present()) {
        drawCurrentFrame();
        pollBackgroundMusic();
    }

    seqt_destroy_source(backgroundMusic);
    return 0;
}
