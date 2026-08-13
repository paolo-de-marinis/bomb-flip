#include "game.h"

#include "board.h"
#include "controls.h"
#include "render.h"

#include <riv.h>
#include <stdio.h>
#include <string.h>

#define EASTER_EGG_COIN_PENALTY (-1000000)

#if DEBUG_MODE
static void debug_log(const char *message) {
    printf("DEBUG: %s\n", message);
}
#endif

static void reset_level_state(GameState *game) {
    game->levelCoins = 0;
    game->phase = GAME_PHASE_ACTIVE;
    game->endState = GAME_END_PLAYING;
    game->explosionFrame = 0;
    game->explosionCellX = -1;
    game->explosionCellY = -1;
    game->bombRevealFrame = 0;
    game->timeoutNextCell = 0;
    game->timeoutDelayFrames = 0;
    game->levelClearDelay = 0;
    game->levelClearedTimer = 0;
    game->timeBonus = 0;
    game->foldedCoins = 0;
    game->frameCount = 0;
    game->hasScanner = false;
    game->scannerUses = 0;
    game->scannerCount = 0;
    game->scannerInUse = false;

    for (int i = 0; i < MAX_SCANNERS; i++) {
        game->scannerX[i] = -1;
        game->scannerY[i] = -1;
        game->scannerRevealed[i] = false;
    }
}

static void initialize_level(GameState *game) {
    reset_level_state(game);
    board_initialize(game);
    game->timeRemaining = (float)(BASE_TIME_PER_LEVEL + (game->level - 1) * 5);

#if DEBUG_MODE
    char message[128];
    snprintf(message,
             sizeof(message),
             "Level %d initialized; scanners: (%d,%d), (%d,%d)",
             game->level,
             game->scannerX[0],
             game->scannerY[0],
             game->scannerX[1],
             game->scannerY[1]);
    debug_log(message);
#endif
}

void game_initialize(GameState *game) {
    memset(game, 0, sizeof(*game));
    game->level = 1;
    game->timeRemaining = BASE_TIME_PER_LEVEL;
    game->phase = GAME_PHASE_ACTIVE;
    game->endState = GAME_END_PLAYING;
    game->explosionCellX = -1;
    game->explosionCellY = -1;
    for (int i = 0; i < MAX_SCANNERS; i++) {
        game->scannerX[i] = -1;
        game->scannerY[i] = -1;
    }
    game_update_outcard(game);
}

void game_begin_run(GameState *game) {
    game->level = 1;
    game->totalCoins = 0;
    game->totalCardsFlipped = 0;
    game->selectedX = 0;
    game->selectedY = 0;
    initialize_level(game);
    game_update_outcard(game);
}

void game_update_outcard(const GameState *game) {
    riv->outcard_len =
        (uint32_t)riv_snprintf((char *)riv->outcard,
                               RIV_SIZE_OUTCARD,
                               "JSON{\"score\":%d,\"level\":%d,\"cards_flipped\":%d,"
                               "\"time_remaining\":%.2f}",
                               game->totalCoins,
                               game->level,
                               game->totalCardsFlipped,
                               game->timeRemaining);
}

void game_finish(GameState *game, GameEndState state, AudioState *audio) {
    game->endState = state;
    game->phase = GAME_PHASE_FINISHED;
    audio_stop_background(audio);
    riv->quit_frame = riv->frame + 3 * riv->target_fps;

    switch (state) {
    case GAME_END_COMPLETE:
        audio_play_level_clear();
        break;
    case GAME_END_BOMB:
        audio_play_game_over();
        break;
    case GAME_END_FOLD:
        game->foldedCoins = game->levelCoins / 2;
        game->totalCoins += game->foldedCoins;
        audio_play_reveal();
        break;
    case GAME_END_TIMEOUT:
        game->timeRemaining = 0.0f;
        game->totalCoins = 0;
        game->levelCoins = 0;
        audio_play_game_over();
        break;
    case GAME_END_EASTER_EGG:
        game->totalCoins += EASTER_EGG_COIN_PENALTY;
        game->levelCoins = EASTER_EGG_COIN_PENALTY;
        audio_play_game_over();
        break;
    case GAME_END_PLAYING:
    default:
        break;
    }

    game_update_outcard(game);
}

static void update_flip_animations(GameState *game) {
    int gridSize = board_grid_size(game->level);
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            int *frame = &game->grid[y][x].flipFrame;
            if (*frame > 0 && *frame < FLIP_ANIMATION_FRAMES) {
                (*frame)++;
            }
        }
    }
}

static void update_level_clearing(GameState *game) {
    int gridSize = board_grid_size(game->level);
    bool allCardsRevealed = true;
    update_flip_animations(game);

    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            Tile *tile = &game->grid[y][x];
            if (!tile->revealed) {
                tile->revealed = true;
                tile->flipFrame = 1;
                allCardsRevealed = false;
            } else if (tile->flipFrame < FLIP_ANIMATION_FRAMES) {
                allCardsRevealed = false;
            }
        }
    }

    if (!allCardsRevealed) {
        return;
    }
    if (game->levelClearDelay > 0) {
        game->levelClearDelay--;
        return;
    }

    game->phase = GAME_PHASE_LEVEL_CLEARED;
    game->levelClearedTimer = LEVEL_CLEARED_PANEL_FRAMES;
    game->timeBonus = (int)(game->timeRemaining * 10.0f);
    game->totalCoins += game->timeBonus;
    game->levelCoins += game->timeBonus;
}

static void update_cleared_level(GameState *game, AudioState *audio) {
    if (game->levelClearedTimer > 0) {
        game->levelClearedTimer--;
    } else if (game->level == MAX_LEVEL) {
        game_finish(game, GAME_END_COMPLETE, audio);
    } else {
        game->phase = GAME_PHASE_NEXT_LEVEL;
    }
}

static void start_next_level(GameState *game, AudioState *audio) {
    game->level++;
    initialize_level(game);
    audio_start_background(audio);
}

static void update_timeout_chain(GameState *game, AudioState *audio) {
    if (game->timeoutDelayFrames > 0) {
        game->timeoutDelayFrames--;
        game->explosionFrame++;
        return;
    }

    int gridSize = board_grid_size(game->level);
    int cellCount = gridSize * gridSize;
    while (game->timeoutNextCell < cellCount) {
        int cell = game->timeoutNextCell++;
        int x = cell % gridSize;
        int y = cell / gridSize;
        if (game->grid[y][x].value != 0) {
            continue;
        }

        game->grid[y][x].revealed = true;
        game->explosionCellX = x;
        game->explosionCellY = y;
        game->explosionFrame = 1;
        game->timeoutDelayFrames = CHAIN_EXPLOSION_DELAY_FRAMES - 1;
        audio_play_explosion();
        return;
    }

    game_finish(game, GAME_END_TIMEOUT, audio);
}

static void begin_timeout_chain(GameState *game, AudioState *audio) {
    game->timeRemaining = 0.0f;
    game->phase = GAME_PHASE_TIMEOUT_CHAIN;
    game->timeoutNextCell = 0;
    game->timeoutDelayFrames = 0;
    audio_stop_background(audio);
    update_timeout_chain(game, audio);
}

static bool update_timer(GameState *game, AudioState *audio) {
    float previousTime = game->timeRemaining;
    game->timeRemaining -= 1.0f / TARGET_FPS;

    if (game->timeRemaining <= 0.0f) {
        begin_timeout_chain(game, audio);
        return false;
    }

    if (game->timeRemaining <= TIME_WARNING_THRESHOLD) {
        audio_stop_background(audio);
        if ((int)previousTime != (int)game->timeRemaining) {
            audio_play_timer_tick();
        }
    } else if (!audio->backgroundMusicPlaying) {
        audio_start_background(audio);
    }
    return true;
}

static void move_selection(GameState *game) {
    int gridSize = board_grid_size(game->level);
    if (riv->keys[RIV_GAMEPAD_UP].press) {
        game->selectedY = (game->selectedY - 1 + gridSize) % gridSize;
    } else if (riv->keys[RIV_GAMEPAD_DOWN].press) {
        game->selectedY = (game->selectedY + 1) % gridSize;
    } else if (riv->keys[RIV_GAMEPAD_LEFT].press) {
        game->selectedX = (game->selectedX - 1 + gridSize) % gridSize;
    } else if (riv->keys[RIV_GAMEPAD_RIGHT].press) {
        game->selectedX = (game->selectedX + 1) % gridSize;
    }
}

void game_reveal_selected(GameState *game, AudioState *audio) {
    Tile *tile = &game->grid[game->selectedY][game->selectedX];
    if (tile->revealed || game->phase != GAME_PHASE_ACTIVE) {
        return;
    }

    tile->revealed = true;
    tile->flipFrame = 1;
    game->totalCardsFlipped++;

#if DEBUG_MODE
    char message[100];
    snprintf(message,
             sizeof(message),
             "Tile revealed at (%d,%d), value %d",
             game->selectedX,
             game->selectedY,
             tile->value);
    debug_log(message);
#endif

    for (int i = 0; i < game->scannerCount; i++) {
        if (game->selectedX == game->scannerX[i] && game->selectedY == game->scannerY[i]) {
            game->scannerRevealed[i] = true;
            game->scannerUses += tile->value;
#if DEBUG_MODE
            snprintf(message,
                     sizeof(message),
                     "Scanner tile %d revealed; uses: %d",
                     i + 1,
                     game->scannerUses);
            debug_log(message);
#endif
            break;
        }
    }

    if (tile->value == 0) {
        game->phase = GAME_PHASE_BOMB_REVEAL;
        game->bombRevealFrame = 1;
        game->explosionCellX = game->selectedX;
        game->explosionCellY = game->selectedY;
        audio_stop_background(audio);
        return;
    }

    game->levelCoins += tile->value * 100;
    audio_play_reveal();
    game->timeRemaining += TIME_BONUS_PER_CARD * tile->value;
    if (game->timeRemaining > MAX_TIME) {
        game->timeRemaining = MAX_TIME;
    }

    if (board_all_high_cards_flipped(game)) {
        game->totalCoins += game->levelCoins;
        game->phase = GAME_PHASE_LEVEL_CLEARING;
        game->levelClearDelay = LEVEL_CLEAR_DELAY_FRAMES;
        audio_stop_background(audio);
        audio_play_level_clear();
    }
}

static bool fold_menu_pressed(void) {
    return riv->keys[CONTROL_FOLD_MENU].press || riv->keys[CONTROL_FOLD_MENU_ALT].press;
}

static bool handle_fold_input(GameState *game, AudioState *audio) {
    if (!fold_menu_pressed()) {
        return false;
    }

    int foldedCoins = game->levelCoins / 2;
    int finalCoins = game->totalCoins + foldedCoins;

    /*
     * The dialog owns presentation while it is open. This intentionally pauses
     * the countdown and background-music polling so that considering a fold
     * never consumes game time.
     */
    while (riv_present()) {
        render_draw_fold_dialog(finalCoins);
        if (riv->keys[CONTROL_REVEAL].press) {
            game_finish(game, GAME_END_FOLD, audio);
            return true;
        }
        if (fold_menu_pressed()) {
            return true;
        }
    }
    return false;
}

static bool handle_board_input(GameState *game, AudioState *audio) {
    move_selection(game);
    if (riv->keys[CONTROL_REVEAL].press) {
        game_reveal_selected(game, audio);
        if (game->phase != GAME_PHASE_ACTIVE) {
            return true;
        }
    }
    return handle_fold_input(game, audio);
}

static void update_bomb_animation(GameState *game) {
    if (game->bombRevealFrame < FLIP_ANIMATION_FRAMES) {
        game->bombRevealFrame++;
    } else if (game->bombRevealFrame == FLIP_ANIMATION_FRAMES) {
        game->explosionFrame = 1;
        game->bombRevealFrame++;
        audio_play_explosion();
    }
}

static void update_explosion_animation(GameState *game, AudioState *audio) {
    if (game->explosionFrame <= 0) {
        return;
    }
    if (game->explosionFrame < EXPLOSION_DURATION) {
        game->explosionFrame++;
    } else {
        game_finish(game, GAME_END_BOMB, audio);
    }
}

bool game_scanner_is_available(const GameState *game) {
    return game->phase == GAME_PHASE_ACTIVE && game->hasScanner && game->scannerUses > 0 &&
           !board_all_high_cards_flipped(game);
}

static void use_scanner(GameState *game, int x, int y) {
    Tile *tile = &game->grid[y][x];
    if (tile->revealed) {
        return;
    }

    game->scannerInUse = true;
    tile->flipFrame = 1;

    /*
     * Like the fold dialog, the preview is deliberately modal: neither the
     * timer nor music sequencing advances while the information bonus is used.
     */
    while (tile->flipFrame < FLIP_ANIMATION_FRAMES) {
        tile->flipFrame++;
        render_draw_game(game);
        riv_present();
    }
    for (int frame = 0; frame < SCANNER_PREVIEW_FRAMES; frame++) {
        render_draw_game(game);
        riv_present();
    }
    while (tile->flipFrame > 0) {
        tile->flipFrame--;
        render_draw_game(game);
        riv_present();
    }

    game->scannerUses--;
    game->scannerInUse = false;
}

static void handle_scanner_input(GameState *game) {
    if (!game_scanner_is_available(game) || !riv->keys[CONTROL_SCANNER].press) {
        return;
    }

#if DEBUG_MODE
    char message[100];
    snprintf(message, sizeof(message), "Scanner used; available uses: %d", game->scannerUses);
    debug_log(message);
#endif
    use_scanner(game, game->selectedX, game->selectedY);
}

#if CHEATS_ENABLED
void game_cheat_complete_level(GameState *game, AudioState *audio) {
    int gridSize = board_grid_size(game->level);
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            Tile *tile = &game->grid[y][x];
            if (tile->value > 0 && !tile->revealed) {
                tile->revealed = true;
                tile->flipFrame = FLIP_ANIMATION_FRAMES;
                game->levelCoins += tile->value * 100;
                game->totalCardsFlipped++;
            }
        }
    }
    game->totalCoins += game->levelCoins;
    game->phase = GAME_PHASE_LEVEL_CLEARING;
    game->levelClearDelay = LEVEL_CLEAR_DELAY_FRAMES;
    audio_stop_background(audio);
    audio_play_level_clear();
}
#endif

void game_update(GameState *game, AudioState *audio) {
    game->frameCount++;

    switch (game->phase) {
    case GAME_PHASE_BOMB_REVEAL:
        /* The countdown is frozen once a bomb has been selected. */
        update_flip_animations(game);
        update_bomb_animation(game);
        update_explosion_animation(game, audio);
        return;
    case GAME_PHASE_TIMEOUT_CHAIN:
        update_timeout_chain(game, audio);
        return;
    case GAME_PHASE_LEVEL_CLEARING:
        update_level_clearing(game);
        return;
    case GAME_PHASE_LEVEL_CLEARED:
        update_cleared_level(game, audio);
        return;
    case GAME_PHASE_NEXT_LEVEL:
        start_next_level(game, audio);
        return;
    case GAME_PHASE_FINISHED:
        return;
    case GAME_PHASE_ACTIVE:
        break;
    }

    if (!update_timer(game, audio)) {
        return;
    }

    bool inputHandled = handle_board_input(game, audio);
    if (game->phase == GAME_PHASE_BOMB_REVEAL) {
        update_flip_animations(game);
        update_bomb_animation(game);
        return;
    }
    if (game->phase != GAME_PHASE_ACTIVE || inputHandled) {
        return;
    }

    update_flip_animations(game);

#if CHEATS_ENABLED
    if (riv->keys[CONTROL_CHEAT_COMPLETE_LEVEL].press) {
        game_cheat_complete_level(game, audio);
        return;
    }
#endif

    handle_scanner_input(game);
}
