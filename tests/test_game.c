#include "game.h"
#include "board.h"
#include "controls.h"
#include "test_support.h"

#include <assert.h>
#include <math.h>
#include <riv.h>
#include <stdio.h>
#include <string.h>

static GameState active_game(int level) {
    GameState game = {0};
    game.level = level;
    game.phase = GAME_PHASE_ACTIVE;
    game.endState = GAME_END_PLAYING;
    game.timeRemaining = 30.0f;
    int gridSize = board_grid_size(level);
    board_clear(&game, gridSize);
    return game;
}

static void clear_keys(void) {
    for (int i = 0; i < RIV_NUM_KEYCODE; i++) {
        riv->keys[i].press = false;
    }
}

static void verify_outcomes(void) {
    AudioState audio = {0};

    GameState bomb = active_game(3);
    bomb.totalCoins = 1000;
    bomb.levelCoins = 500;
    game_finish(&bomb, GAME_END_BOMB, &audio);
    assert(bomb.totalCoins == 1000);
    assert(bomb.levelCoins == 500);

    GameState timeout = active_game(3);
    timeout.totalCoins = 1000;
    timeout.levelCoins = 500;
    timeout.timeRemaining = -0.25f;
    game_finish(&timeout, GAME_END_TIMEOUT, &audio);
    assert(timeout.totalCoins == 0);
    assert(timeout.levelCoins == 0);
    assert(timeout.timeRemaining == 0.0f);

    GameState fold = active_game(3);
    fold.totalCoins = 1000;
    fold.levelCoins = 501;
    game_finish(&fold, GAME_END_FOLD, &audio);
    assert(fold.foldedCoins == 250);
    assert(fold.totalCoins == 1250);
}

static void verify_initial_outcard(void) {
    GameState game;
    game_initialize(&game);
    assert(game.level == 1);
    assert(game.timeRemaining == BASE_TIME_PER_LEVEL);
    assert(strcmp((const char *)riv->outcard,
                  "JSON{\"score\":0,\"level\":1,\"cards_flipped\":0,"
                  "\"time_remaining\":45.00}") == 0);
}

static void verify_second_scanner_is_independent(void) {
    GameState game = active_game(9);
    game.hasScanner = true;
    game.scannerCount = 2;
    game.scannerX[0] = 0;
    game.scannerY[0] = 0;
    game.scannerX[1] = 1;
    game.scannerY[1] = 0;
    game.scannerRevealed[0] = false;
    game.scannerRevealed[1] = true;
    game.scannerUses = 2;
    game.grid[2][2].value = 2;
    assert(game_scanner_is_available(&game));
}

static void verify_bomb_freezes_timer(void) {
    AudioState audio = {0};
    GameState game = active_game(1);
    game.phase = GAME_PHASE_BOMB_REVEAL;
    game.timeRemaining = 0.001f;
    game.bombRevealFrame = 1;
    game.explosionCellX = 0;
    game.explosionCellY = 0;
    game.grid[0][0].value = 0;
    float initialTime = game.timeRemaining;

    for (int frame = 0; frame < 100 && game.phase != GAME_PHASE_FINISHED; frame++) {
        game_update(&game, &audio);
    }

    assert(game.phase == GAME_PHASE_FINISHED);
    assert(game.endState == GAME_END_BOMB);
    assert(fabsf(game.timeRemaining - initialTime) < 0.000001f);
}

static void verify_terminal_reveal_blocks_fold(void) {
    AudioState audio = {0};
    GameState bomb = active_game(1);
    bomb.grid[0][0].value = 0;
    bomb.totalCoins = 700;
    bomb.levelCoins = 300;
    riv->keys[CONTROL_REVEAL].press = true;
    riv->keys[CONTROL_FOLD_MENU].press = true;
    game_update(&bomb, &audio);
    assert(bomb.phase == GAME_PHASE_BOMB_REVEAL);
    assert(bomb.endState == GAME_END_PLAYING);
    assert(bomb.foldedCoins == 0);
    assert(bomb.totalCoins == 700);

    clear_keys();
    GameState completion = active_game(1);
    completion.grid[0][0].value = 2;
    riv->keys[CONTROL_REVEAL].press = true;
    riv->keys[CONTROL_FOLD_MENU_ALT].press = true;
    game_update(&completion, &audio);
    assert(completion.phase == GAME_PHASE_LEVEL_CLEARING);
    assert(completion.endState == GAME_END_PLAYING);
    assert(completion.levelCoins == 200);
    assert(completion.totalCoins == 200);
    assert(completion.foldedCoins == 0);
    clear_keys();
}

static void verify_timeout_is_clamped(void) {
    test_runtime_reset();
    AudioState audio = {0};
    GameState game = active_game(1);
    game.timeRemaining = 0.001f;
    game.totalCoins = 900;
    game.grid[0][0].value = 0;
    game_update(&game, &audio);
    assert(game.phase == GAME_PHASE_TIMEOUT_CHAIN);
    assert(game.timeRemaining == 0.0f);
    assert(game.grid[0][0].revealed);
    assert(test_present_count() == 0);

    for (int frame = 0; frame < 100 && game.phase != GAME_PHASE_FINISHED; frame++) {
        game_update(&game, &audio);
    }

    assert(game.endState == GAME_END_TIMEOUT);
    assert(game.phase == GAME_PHASE_FINISHED);
    assert(game.timeRemaining == 0.0f);
    assert(game.totalCoins == 0);
    assert(test_present_count() == 0);
}

static void verify_modal_scanner_does_not_consume_extra_time(void) {
    test_runtime_reset();
    AudioState audio = {0};
    GameState game = active_game(1);
    game.hasScanner = true;
    game.scannerUses = 1;
    game.grid[0][0].value = 1;
    game.grid[0][1].value = 2;
    float initialTime = game.timeRemaining;

    riv->keys[CONTROL_SCANNER].press = true;
    game_update(&game, &audio);

    assert(test_present_count() == 59);
    assert(fabsf(game.timeRemaining - (initialTime - 1.0f / TARGET_FPS)) < 0.000001f);
    assert(game.scannerUses == 0);
    assert(!game.grid[0][0].revealed);
    assert(game.grid[0][0].flipFrame == 0);
    assert(test_audio_poll_count() == 0);
    clear_keys();
}

static void verify_modal_fold_confirm_and_cancel(void) {
    AudioState audio = {0};

    test_runtime_reset();
    GameState confirm = active_game(3);
    confirm.totalCoins = 1000;
    confirm.levelCoins = 501;
    float confirmTime = confirm.timeRemaining;
    riv->keys[CONTROL_FOLD_MENU].press = true;
    test_present_with_key(CONTROL_REVEAL);
    game_update(&confirm, &audio);
    assert(confirm.phase == GAME_PHASE_FINISHED);
    assert(confirm.endState == GAME_END_FOLD);
    assert(confirm.foldedCoins == 250);
    assert(confirm.totalCoins == 1250);
    assert(fabsf(confirm.timeRemaining - (confirmTime - 1.0f / TARGET_FPS)) < 0.000001f);
    assert(test_present_count() == 1);
    assert(test_audio_poll_count() == 0);

    test_runtime_reset();
    GameState cancel = active_game(3);
    cancel.totalCoins = 1000;
    cancel.levelCoins = 501;
    float cancelTime = cancel.timeRemaining;
    strcpy((char *)riv->outcard, "modal-sentinel");
    riv->keys[CONTROL_FOLD_MENU_ALT].press = true;
    const int cancelKeys[] = {CONTROL_START, CONTROL_FOLD_MENU_ALT};
    test_present_with_keys(cancelKeys, 2);
    game_update(&cancel, &audio);
    assert(cancel.phase == GAME_PHASE_ACTIVE);
    assert(cancel.endState == GAME_END_PLAYING);
    assert(cancel.foldedCoins == 0);
    assert(cancel.totalCoins == 1000);
    assert(fabsf(cancel.timeRemaining - (cancelTime - 1.0f / TARGET_FPS)) < 0.000001f);
    assert(strcmp((char *)riv->outcard, "modal-sentinel") == 0);
    assert(test_present_count() == 2);
    assert(test_audio_poll_count() == 0);
    clear_keys();
}

#if CHEATS_ENABLED
static void verify_cheat_clear_does_not_double_count(void) {
    AudioState audio = {0};
    GameState game = active_game(1);
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            game.grid[y][x].value = 0;
        }
    }
    game.grid[0][0].value = 1;
    game.grid[0][0].revealed = true;
    game.grid[0][1].value = 2;
    game.levelCoins = 100;

    game_cheat_complete_level(&game, &audio);
    assert(game.levelCoins == 300);
    assert(game.totalCoins == 300);
    assert(game.totalCardsFlipped == 1);
}
#endif

int main(void) {
    test_runtime_reset();
    verify_initial_outcard();
    verify_outcomes();
    verify_second_scanner_is_independent();
    verify_bomb_freezes_timer();
    verify_terminal_reveal_blocks_fold();
    verify_timeout_is_clamped();
    verify_modal_scanner_does_not_consume_extra_time();
    verify_modal_fold_confirm_and_cancel();
#if CHEATS_ENABLED
    verify_cheat_clear_does_not_double_count();
#endif
    puts("game outcomes: ok");
    return 0;
}
