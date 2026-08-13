#ifndef BOMB_FLIP_BOARD_H
#define BOMB_FLIP_BOARD_H

#include "state.h"

typedef struct {
    int x2Cards;
    int x3Cards;
    int bombs;
} LevelConfig;

const LevelConfig *board_level_config(int level);
int board_grid_size(int level);
int board_x1_count(int level);

void board_initialize(GameState *game);
void board_clear(GameState *game, int gridSize);
void board_calculate_clues(GameState *game, int gridSize);
void board_assign_scanner_tiles(GameState *game);
bool board_all_high_cards_flipped(const GameState *game);

#endif
