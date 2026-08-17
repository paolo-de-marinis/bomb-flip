#include "board.h"
#include "test_support.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int expected_scanner_count(int level) {
    if (level <= 3) {
        return 0;
    }
    return level <= 8 ? 1 : 2;
}

static void verify_level(int level) {
    GameState game = {0};
    game.level = level;
    test_rng_seed((uint64_t)level);
    board_initialize(&game);

    const LevelConfig *config = board_level_config(level);
    int gridSize = board_grid_size(level);
    int counts[4] = {0};
    int rowSum = 0;
    int columnSum = 0;
    int rowBombs = 0;
    int columnBombs = 0;

    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            int value = game.grid[y][x].value;
            assert(value >= 0 && value <= 3);
            counts[value]++;
        }
        rowSum += game.rowTotals[y];
        columnSum += game.columnTotals[y];
        rowBombs += game.rowBombs[y];
        columnBombs += game.columnBombs[y];
    }

    int cells = gridSize * gridSize;
    int expectedSum = cells - config->bombs + config->x2Cards + 2 * config->x3Cards;
    assert(counts[0] == config->bombs);
    assert(counts[1] == board_x1_count(level));
    assert(counts[2] == config->x2Cards);
    assert(counts[3] == config->x3Cards);
    assert(rowSum == expectedSum);
    assert(columnSum == expectedSum);
    assert(rowBombs == config->bombs);
    assert(columnBombs == config->bombs);
    assert(!board_all_high_cards_flipped(&game));

    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            if (game.grid[y][x].value >= 2) {
                game.grid[y][x].revealed = true;
            }
        }
    }
    assert(board_all_high_cards_flipped(&game));

    int expectedScanners = expected_scanner_count(level);
    assert(game.scannerCount == expectedScanners);
    assert(game.hasScanner == (expectedScanners > 0));
    for (int i = 0; i < game.scannerCount; i++) {
        assert(game.grid[game.scannerY[i]][game.scannerX[i]].value > 0);
        for (int j = 0; j < i; j++) {
            assert(game.scannerX[i] != game.scannerX[j] ||
                   game.scannerY[i] != game.scannerY[j]);
        }
    }
}

static void verify_scanner_is_absent_before_level_four(void) {
    GameState game = {.level = 3,
                      .hasScanner = true,
                      .scannerCount = MAX_SCANNERS};
    board_clear(&game, board_grid_size(game.level));

    board_assign_scanner_tiles(&game);

    assert(game.scannerCount == 0);
    assert(!game.hasScanner);
    for (int i = 0; i < MAX_SCANNERS; i++) {
        assert(game.scannerX[i] == -1);
        assert(game.scannerY[i] == -1);
        assert(!game.scannerRevealed[i]);
    }
}

static void verify_deterministic_generation(void) {
    GameState first = {.level = 12};
    GameState second = {.level = 12};

    test_rng_seed(42);
    board_initialize(&first);
    test_rng_seed(42);
    board_initialize(&second);

    assert(memcmp(first.grid, second.grid, sizeof(first.grid)) == 0);
    assert(first.scannerCount == second.scannerCount);
    assert(memcmp(first.scannerX, second.scannerX, sizeof(first.scannerX)) == 0);
    assert(memcmp(first.scannerY, second.scannerY, sizeof(first.scannerY)) == 0);
}

static void verify_scanner_fallback(void) {
    GameState game = {.level = 9};
    board_clear(&game, board_grid_size(game.level));
    game.grid[0][0].value = 0;

    test_rng_force(true, 0);
    board_assign_scanner_tiles(&game);
    test_rng_force(false, 0);

    assert(game.scannerCount == 2);
    assert(game.hasScanner);
    assert(game.scannerX[0] == 1 && game.scannerY[0] == 0);
    assert(game.scannerX[1] == 2 && game.scannerY[1] == 0);
}

int main(void) {
    test_runtime_reset();
    for (int level = 1; level <= MAX_LEVEL; level++) {
        verify_level(level);
    }
    verify_deterministic_generation();
    verify_scanner_is_absent_before_level_four();
    verify_scanner_fallback();
    puts("board invariants: ok");
    return 0;
}
