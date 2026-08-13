#ifndef BOMB_FLIP_RENDER_H
#define BOMB_FLIP_RENDER_H

#include "state.h"

void render_draw_game(const GameState *game);
void render_draw_end_screen(const GameState *game);
void render_draw_fold_dialog(int finalCoins);
void render_draw_pixelated_bomb(int x, int y, int size);

#endif
