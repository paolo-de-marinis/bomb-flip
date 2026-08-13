#include "board.h"

#include <assert.h>
#include <riv.h>

static const LevelConfig LEVEL_CONFIGS[MAX_LEVEL] = {
    {.x2Cards = 3, .x3Cards = 1, .bombs = 6, .legacyMinimumReward = 24, .legacyMaximumReward = 48},
    {.x2Cards = 4, .x3Cards = 2, .bombs = 7, .legacyMinimumReward = 54, .legacyMaximumReward = 108},
    {.x2Cards = 5, .x3Cards = 3, .bombs = 8, .legacyMinimumReward = 96, .legacyMaximumReward = 192},
    {.x2Cards = 6, .x3Cards = 3, .bombs = 8, .legacyMinimumReward = 192, .legacyMaximumReward = 384},
    {.x2Cards = 7, .x3Cards = 4, .bombs = 10, .legacyMinimumReward = 288, .legacyMaximumReward = 576},
    {.x2Cards = 8, .x3Cards = 4, .bombs = 10, .legacyMinimumReward = 480, .legacyMaximumReward = 960},
    {.x2Cards = 8, .x3Cards = 5, .bombs = 10, .legacyMinimumReward = 720, .legacyMaximumReward = 1440},
    {.x2Cards = 10, .x3Cards = 5, .bombs = 10, .legacyMinimumReward = 1080, .legacyMaximumReward = 2160},
    {.x2Cards = 7, .x3Cards = 3, .bombs = 13, .legacyMinimumReward = 1500, .legacyMaximumReward = 3000},
    {.x2Cards = 8, .x3Cards = 3, .bombs = 14, .legacyMinimumReward = 2000, .legacyMaximumReward = 4000},
    {.x2Cards = 9, .x3Cards = 3, .bombs = 15, .legacyMinimumReward = 2500, .legacyMaximumReward = 5000},
    {.x2Cards = 10, .x3Cards = 3, .bombs = 16, .legacyMinimumReward = 3000, .legacyMaximumReward = 6000},
};

const LevelConfig *board_level_config(int level) {
    assert(level >= 1 && level <= MAX_LEVEL);
    return &LEVEL_CONFIGS[level - 1];
}

int board_grid_size(int level) {
    assert(level >= 1 && level <= MAX_LEVEL);
    return level <= 8 ? 5 : 6;
}

int board_x1_count(int level) {
    const LevelConfig *config = board_level_config(level);
    int gridSize = board_grid_size(level);
    return gridSize * gridSize - config->bombs - config->x2Cards - config->x3Cards;
}

void board_clear(GameState *game, int gridSize) {
    for (int i = 0; i < MAX_GRID_SIZE; i++) {
        game->rowTotals[i] = 0;
        game->columnTotals[i] = 0;
        game->rowBombs[i] = 0;
        game->columnBombs[i] = 0;
    }

    for (int y = 0; y < MAX_GRID_SIZE; y++) {
        for (int x = 0; x < MAX_GRID_SIZE; x++) {
            Tile *tile = &game->grid[y][x];
            tile->value = x < gridSize && y < gridSize ? 1 : -1;
            tile->revealed = false;
            tile->flipFrame = 0;
        }
    }
}

static void place_level_cards(GameState *game,
                              int gridSize,
                              int bombCount,
                              int x2Count,
                              int x3Count) {
    while (bombCount > 0 || x2Count > 0 || x3Count > 0) {
        int x = (int)riv_rand_uint((uint64_t)(gridSize - 1));
        int y = (int)riv_rand_uint((uint64_t)(gridSize - 1));

        if (game->grid[y][x].value != 1) {
            continue;
        }

        if (bombCount > 0) {
            game->grid[y][x].value = 0;
            bombCount--;
        } else if (x2Count > 0) {
            game->grid[y][x].value = 2;
            x2Count--;
        } else {
            game->grid[y][x].value = 3;
            x3Count--;
        }
    }
}

void board_calculate_clues(GameState *game, int gridSize) {
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            int value = game->grid[y][x].value;
            game->rowTotals[y] += value;
            game->columnTotals[x] += value;
            if (value == 0) {
                game->rowBombs[y]++;
                game->columnBombs[x]++;
            }
        }
    }
}

static bool scanner_cell_is_available(const GameState *game, int x, int y) {
    if (game->grid[y][x].value <= 0) {
        return false;
    }

    for (int i = 0; i < game->scannerCount; i++) {
        if (game->scannerX[i] == x && game->scannerY[i] == y) {
            return false;
        }
    }
    return true;
}

static void store_scanner_cell(GameState *game, int x, int y) {
    int index = game->scannerCount;
    assert(index >= 0 && index < MAX_SCANNERS);
    game->scannerX[index] = x;
    game->scannerY[index] = y;
    game->scannerRevealed[index] = false;
    game->scannerCount++;
}

static bool assign_random_scanner_cell(GameState *game, int gridSize) {
    for (int attempt = 0; attempt < 100; attempt++) {
        int x = (int)riv_rand_uint((uint64_t)(gridSize - 1));
        int y = (int)riv_rand_uint((uint64_t)(gridSize - 1));
        if (scanner_cell_is_available(game, x, y)) {
            store_scanner_cell(game, x, y);
            return true;
        }
    }
    return false;
}

static bool assign_fallback_scanner_cell(GameState *game, int gridSize) {
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            if (scanner_cell_is_available(game, x, y)) {
                store_scanner_cell(game, x, y);
                return true;
            }
        }
    }
    return false;
}

void board_assign_scanner_tiles(GameState *game) {
    int gridSize = board_grid_size(game->level);
    int scannersToAssign = game->level >= 9 ? 2 : 1;

    game->scannerCount = 0;
    for (int i = 0; i < scannersToAssign; i++) {
        bool assigned = assign_random_scanner_cell(game, gridSize);
        if (!assigned) {
            assigned = assign_fallback_scanner_cell(game, gridSize);
        }
        assert(assigned);
    }
    game->hasScanner = game->scannerCount == scannersToAssign;
}

void board_initialize(GameState *game) {
    const LevelConfig *config = board_level_config(game->level);
    int gridSize = board_grid_size(game->level);

    board_clear(game, gridSize);
    place_level_cards(game, gridSize, config->bombs, config->x2Cards, config->x3Cards);
    board_calculate_clues(game, gridSize);
    board_assign_scanner_tiles(game);
}

bool board_all_high_cards_flipped(const GameState *game) {
    int gridSize = board_grid_size(game->level);
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            const Tile *tile = &game->grid[y][x];
            if ((tile->value == 2 || tile->value == 3) && !tile->revealed) {
                return false;
            }
        }
    }
    return true;
}
