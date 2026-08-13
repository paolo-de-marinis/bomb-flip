#include "render.h"

#include "board.h"

#include <math.h>
#include <riv.h>
#include <stdio.h>

#define PI 3.14159265358979323846f

static bool scanner_is_available(const GameState *game) {
    return game->phase == GAME_PHASE_ACTIVE && game->hasScanner && game->scannerUses > 0 &&
           !board_all_high_cards_flipped(game);
}

static void draw_coin(int x, int y, int size, int value) {
    riv_draw_circle_fill(x, y, size / 2, RIV_COLOR_YELLOW);
    riv_draw_circle_line(x, y, size / 2, RIV_COLOR_ORANGE);

    char valueText[2];
    snprintf(valueText, sizeof(valueText), "%d", value);
    riv_draw_text(valueText, RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, x, y, 1, RIV_COLOR_BLACK);
}

void render_draw_pixelated_bomb(int x, int y, int size) {
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

static void select_level_colors(const GameState *game,
                                uint32_t *backgroundColor,
                                uint32_t *tileColor,
                                uint32_t *revealedColor) {
    *revealedColor = RIV_COLOR_LIGHTGREY;
    if (game->level <= 4) {
        *backgroundColor = RIV_COLOR_DARKGREEN;
        *tileColor = RIV_COLOR_LIGHTGREEN;
    } else if (game->level <= 8) {
        *backgroundColor = RIV_COLOR_BLUE;
        *tileColor = RIV_COLOR_LIGHTBLUE;
    } else if (game->level <= 10) {
        *backgroundColor = RIV_COLOR_DARKRED;
        *tileColor = RIV_COLOR_RED;
    } else {
        *backgroundColor = RIV_COLOR_DARKPURPLE;
        *tileColor = RIV_COLOR_PURPLE;
    }
}

static void draw_tile_content(int centerX, int centerY, int size, int value) {
    if (value == 0) {
        render_draw_pixelated_bomb(centerX, centerY, size);
    } else {
        draw_coin(centerX, centerY, size, value);
    }
}

static void draw_magnifying_glass(int x, int y, int size, uint32_t color) {
    int lensRadius = size / 2;
    int handleLength = size / 3;
    int thickness = size / 10;

    for (int i = 0; i < thickness; i++) {
        riv_draw_circle_line(x, y, lensRadius - i, color);
    }

    float angle = PI / 4.0f;
    int handleStartX = x + (int)(lensRadius * cosf(angle));
    int handleStartY = y + (int)(lensRadius * sinf(angle));
    riv_draw_box_fill(handleStartX, handleStartY, handleLength, thickness, angle, color);
}

static void draw_tile(const GameState *game,
                      int x,
                      int y,
                      int gridOffsetX,
                      int gridOffsetY,
                      uint32_t tileColor,
                      uint32_t revealedColor) {
    const Tile *tile = &game->grid[y][x];
    int tileX = x * TILE_SIZE + gridOffsetX;
    int tileY = y * TILE_SIZE + gridOffsetY;
    bool selectedBombAnimating = game->phase == GAME_PHASE_BOMB_REVEAL &&
                                 x == game->explosionCellX && y == game->explosionCellY;

    if (tile->flipFrame > 0 || selectedBombAnimating) {
        float progress = selectedBombAnimating
                             ? (float)game->bombRevealFrame / FLIP_ANIMATION_FRAMES
                             : (float)tile->flipFrame / FLIP_ANIMATION_FRAMES;
        float angle = progress * PI;
        int width = (int)(TILE_SIZE * fabsf(cosf(angle)));

        if (progress < 0.5f) {
            riv_draw_rect_fill(tileX + (TILE_SIZE - width) / 2, tileY, width, TILE_SIZE, tileColor);
        } else {
            riv_draw_rect_fill(
                tileX + (TILE_SIZE - width) / 2, tileY, width, TILE_SIZE, revealedColor);
            float scale = (float)width / TILE_SIZE;
            draw_tile_content(tileX + TILE_SIZE / 2,
                              tileY + TILE_SIZE / 2,
                              (int)(TILE_SIZE * 3 / 4 * scale),
                              tile->value);
        }
    } else if (tile->revealed || game->phase == GAME_PHASE_LEVEL_CLEARING ||
               game->phase == GAME_PHASE_LEVEL_CLEARED) {
        riv_draw_rect_fill(tileX, tileY, TILE_SIZE, TILE_SIZE, revealedColor);
        draw_tile_content(
            tileX + TILE_SIZE / 2, tileY + TILE_SIZE / 2, TILE_SIZE * 3 / 4, tile->value);
    } else {
        riv_draw_rect_fill(tileX, tileY, TILE_SIZE, TILE_SIZE, tileColor);
    }

    if (x == game->selectedX && y == game->selectedY) {
        bool scannerActive = scanner_is_available(game);
        uint32_t highlightColor = scannerActive ? RIV_COLOR_PEACH : RIV_COLOR_WHITE;
        riv_draw_rect_line(tileX, tileY, TILE_SIZE, TILE_SIZE, highlightColor);

        if (scannerActive && !game->scannerInUse && !tile->revealed) {
            draw_magnifying_glass(
                tileX + TILE_SIZE * 2 / 6, tileY + TILE_SIZE * 2 / 6, TILE_SIZE, RIV_COLOR_PEACH);
        }
    } else {
        riv_draw_rect_line(tileX, tileY, TILE_SIZE, TILE_SIZE, RIV_COLOR_BLACK);
    }
}

static void draw_board(const GameState *game,
                       int gridSize,
                       int gridOffsetX,
                       int gridOffsetY,
                       uint32_t tileColor,
                       uint32_t revealedColor) {
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            draw_tile(game, x, y, gridOffsetX, gridOffsetY, tileColor, revealedColor);
        }
    }

    for (int i = 0; i < game->scannerCount; i++) {
        int x = game->scannerX[i];
        int y = game->scannerY[i];
        const Tile *tile = &game->grid[y][x];
        if (tile->revealed && game->hasScanner) {
            int tileX = x * TILE_SIZE + gridOffsetX;
            int tileY = y * TILE_SIZE + gridOffsetY;
            riv_draw_rect_fill(tileX, tileY, TILE_SIZE, TILE_SIZE, RIV_COLOR_PEACH);
            draw_coin(tileX + TILE_SIZE / 2,
                      tileY + TILE_SIZE / 2,
                      TILE_SIZE * 3 / 4,
                      tile->value);
        }
    }
}

static void draw_clues(const GameState *game, int gridSize, int gridOffsetX, int gridOffsetY) {
    int boardSize = gridSize * TILE_SIZE;

    for (int i = 0; i < gridSize; i++) {
        char clueText[16];
        snprintf(clueText, sizeof(clueText), "%d:%d", game->rowTotals[i], game->rowBombs[i]);
        riv_draw_text(clueText,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_LEFT,
                      gridOffsetX + boardSize + 5,
                      gridOffsetY + i * TILE_SIZE + TILE_SIZE / 2,
                      1,
                      RIV_COLOR_WHITE);

        snprintf(
            clueText, sizeof(clueText), "%d:%d", game->columnTotals[i], game->columnBombs[i]);
        riv_draw_text(clueText,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      gridOffsetX + i * TILE_SIZE + TILE_SIZE / 2,
                      gridOffsetY + boardSize + 5,
                      1,
                      RIV_COLOR_WHITE);
    }
}

static void draw_status(const GameState *game) {
    char text[32];
    snprintf(text, sizeof(text), "Level: %d", game->level);
    riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 5, 5, 1, RIV_COLOR_WHITE);

    int totalSeconds = (int)game->timeRemaining;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    snprintf(text, sizeof(text), "Time: %02d:%02d", minutes, seconds);
    uint32_t timerColor = game->timeRemaining <= TIME_WARNING_THRESHOLD &&
                                  game->frameCount % 30 < 15
                              ? RIV_COLOR_RED
                              : RIV_COLOR_WHITE;
    riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 5, 15, 1, timerColor);

    snprintf(text, sizeof(text), "Total Coins: %d", game->totalCoins);
    riv_draw_text(text,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPRIGHT,
                  SCREEN_SIZE - 5,
                  5,
                  1,
                  RIV_COLOR_WHITE);

    snprintf(text, sizeof(text), "Level Coins: %d", game->levelCoins);
    riv_draw_text(text,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_BOTTOMLEFT,
                  5,
                  SCREEN_SIZE - 5,
                  1,
                  RIV_COLOR_YELLOW);
}

static void draw_explosion(const GameState *game, int gridSize) {
    if (game->explosionFrame <= 0) {
        return;
    }

    int offsetX = (SCREEN_SIZE - gridSize * TILE_SIZE) / 2;
    int offsetY = (SCREEN_SIZE - gridSize * TILE_SIZE) / 2;
    int explosionX = offsetX + game->explosionCellX * TILE_SIZE + TILE_SIZE / 2;
    int explosionY = offsetY + game->explosionCellY * TILE_SIZE + TILE_SIZE / 2;
    render_draw_pixelated_bomb(explosionX, explosionY, TILE_SIZE);

    if (game->explosionFrame > 5) {
        float progress = (float)(game->explosionFrame - 5) / EXPLOSION_DURATION;
        int radius = (int)(EXPLOSION_RADIUS * progress);
        uint32_t color = progress > 0.6f ? RIV_COLOR_YELLOW
                                        : (progress > 0.3f ? RIV_COLOR_ORANGE : RIV_COLOR_RED);
        riv_draw_circle_line(explosionX, explosionY, radius, color);
        if (radius >= 2) {
            riv_draw_circle_line(explosionX, explosionY, radius - 2, color);
        }
    }

    int boomY = explosionY - EXPLOSION_RADIUS - 15;
    float pulse = sinf(game->explosionFrame * 0.2f) * 0.2f + 1.0f;
    int width = (int)(80 * pulse);
    int height = (int)(40 * pulse);
    riv_draw_rect_fill(
        explosionX - width / 2, boomY - height / 2, width, height, RIV_COLOR_WHITE);
    riv_draw_rect_line(
        explosionX - width / 2, boomY - height / 2, width, height, RIV_COLOR_BLACK);
    riv_draw_triangle_fill(explosionX - 10,
                           boomY + height / 2,
                           explosionX + 10,
                           boomY + height / 2,
                           explosionX,
                           boomY + height / 2 + 20,
                           RIV_COLOR_WHITE);
    riv_draw_triangle_line(explosionX - 10,
                           boomY + height / 2,
                           explosionX + 10,
                           boomY + height / 2,
                           explosionX,
                           boomY + height / 2 + 20,
                           RIV_COLOR_BLACK);
    riv_draw_text(
        "BOOM!", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, explosionX, boomY, 2, RIV_COLOR_RED);
}

static void draw_level_cleared_panel(const GameState *game) {
    if (game->phase != GAME_PHASE_LEVEL_CLEARED) {
        return;
    }

    char heading[32];
    char cards[32];
    char time[32];
    char total[32];
    snprintf(heading, sizeof(heading), "Level %d Cleared!", game->level);
    snprintf(cards, sizeof(cards), "Coins Earned: %d", game->levelCoins - game->timeBonus);
    snprintf(time, sizeof(time), "Time Bonus: %d", game->timeBonus);
    snprintf(total, sizeof(total), "Total Earned: %d", game->levelCoins);

    riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLACK);
    riv_draw_text(heading,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 4,
                  2,
                  RIV_COLOR_YELLOW);
    riv_draw_text(cards,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 - 20,
                  1,
                  RIV_COLOR_WHITE);
    riv_draw_text(time,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2,
                  1,
                  RIV_COLOR_WHITE);
    riv_draw_text(total,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 + 20,
                  1,
                  RIV_COLOR_GREEN);
}

static void draw_scanner_overlay(const GameState *game) {
    char usesText[32];
    snprintf(usesText, sizeof(usesText), "Scanner Uses: %d", game->scannerUses);
    riv_draw_text(usesText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_BOTTOMLEFT,
                  5,
                  SCREEN_SIZE - 20,
                  1,
                  RIV_COLOR_PEACH);
    riv_draw_text("X / A2: use scanner",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_BOTTOMRIGHT,
                  SCREEN_SIZE - 5,
                  SCREEN_SIZE - 20,
                  1,
                  RIV_COLOR_PEACH);
}

void render_draw_game(const GameState *game) {
    uint32_t backgroundColor;
    uint32_t tileColor;
    uint32_t revealedColor;
    select_level_colors(game, &backgroundColor, &tileColor, &revealedColor);
    riv_clear(backgroundColor);

    int currentGridSize = board_grid_size(game->level);
    int gridPixels = currentGridSize * TILE_SIZE;
    int gridOffsetX = (SCREEN_SIZE - gridPixels) / 2;
    int gridOffsetY = (SCREEN_SIZE - gridPixels) / 2;
    draw_board(game, currentGridSize, gridOffsetX, gridOffsetY, tileColor, revealedColor);
    draw_clues(game, currentGridSize, gridOffsetX, gridOffsetY);
    draw_status(game);
    draw_explosion(game, currentGridSize);
    draw_level_cleared_panel(game);

    if (game->phase == GAME_PHASE_ACTIVE) {
        riv_draw_text("W/SELECT or F/R2: Fold",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_BOTTOMRIGHT,
                      SCREEN_SIZE - 5,
                      SCREEN_SIZE - 5,
                      1,
                      RIV_COLOR_WHITE);
    }
    if (game->phase == GAME_PHASE_ACTIVE && scanner_is_available(game)) {
        draw_scanner_overlay(game);
    }
}

static void draw_final_score(const GameState *game, int y) {
    char text[32];
    snprintf(text, sizeof(text), "Final Coins: %d", game->totalCoins);
    riv_draw_text(
        text, RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, SCREEN_SIZE / 2, y, 1, RIV_COLOR_WHITE);
}

void render_draw_end_screen(const GameState *game) {
    riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLACK);

    char mainMessage[64];
    char subMessage[64];
    char coinMessage[64] = "";
    uint32_t mainColor = RIV_COLOR_RED;

    switch (game->endState) {
    case GAME_END_COMPLETE:
        snprintf(mainMessage, sizeof(mainMessage), "CONGRATULATIONS!");
        snprintf(subMessage, sizeof(subMessage), "You've completed all levels!");
        mainColor = RIV_COLOR_YELLOW;
        break;
    case GAME_END_BOMB:
        snprintf(mainMessage, sizeof(mainMessage), "GAME OVER");
        snprintf(subMessage, sizeof(subMessage), "Current level coins went boom!");
        break;
    case GAME_END_FOLD:
        snprintf(mainMessage, sizeof(mainMessage), "YOU FOLDED!");
        snprintf(subMessage, sizeof(subMessage), "Level coins retained: %d", game->foldedCoins);
        mainColor = RIV_COLOR_YELLOW;
        break;
    case GAME_END_TIMEOUT:
        snprintf(mainMessage, sizeof(mainMessage), "TIME'S UP!");
        snprintf(subMessage, sizeof(subMessage), "All your coins went KABOOM!");
        break;
    case GAME_END_EASTER_EGG:
        snprintf(mainMessage, sizeof(mainMessage), "EASTER EGG!");
        snprintf(subMessage, sizeof(subMessage), "You found the secret explosion!");
        snprintf(coinMessage, sizeof(coinMessage), "Coin Penalty: -1000000");
        mainColor = RIV_COLOR_PURPLE;
        break;
    case GAME_END_PLAYING:
    default:
        snprintf(mainMessage, sizeof(mainMessage), "GAME OVER");
        subMessage[0] = '\0';
        break;
    }

    int mainY = SCREEN_SIZE / 4;
    int subY = SCREEN_SIZE / 2;
    int scoreY = 3 * SCREEN_SIZE / 4;
    riv_draw_text(mainMessage,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  mainY,
                  2,
                  mainColor);
    riv_draw_text(subMessage,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  subY,
                  1,
                  RIV_COLOR_WHITE);

    if (game->endState == GAME_END_EASTER_EGG) {
        riv_draw_text(coinMessage,
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      scoreY,
                      1,
                      RIV_COLOR_RED);
    } else {
        draw_final_score(game, scoreY);
    }
}

void render_draw_fold_dialog(int finalCoins) {
    char message[64];
    snprintf(message, sizeof(message), "Fold now for %d coins?", finalCoins);
    riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLACK);
    riv_draw_text(message,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 - 20,
                  1,
                  RIV_COLOR_WHITE);
    riv_draw_text("Z/A1: Confirm Fold",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 + 14,
                  1,
                  RIV_COLOR_WHITE);
    riv_draw_text("W/SELECT or F/R2: Back",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 2 + 28,
                  1,
                  RIV_COLOR_WHITE);
}
