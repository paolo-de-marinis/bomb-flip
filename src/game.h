#ifndef BOMB_FLIP_GAME_H
#define BOMB_FLIP_GAME_H

#include "audio.h"
#include "state.h"

void game_initialize(GameState *game);
void game_begin_run(GameState *game);
void game_update(GameState *game, AudioState *audio);
void game_update_outcard(const GameState *game);

bool game_scanner_is_available(const GameState *game);
void game_reveal_selected(GameState *game, AudioState *audio);
void game_finish(GameState *game, GameEndState state, AudioState *audio);

#if CHEATS_ENABLED
void game_cheat_complete_level(GameState *game, AudioState *audio);
#endif

#endif
