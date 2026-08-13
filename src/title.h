#ifndef BOMB_FLIP_TITLE_H
#define BOMB_FLIP_TITLE_H

#include "audio.h"

#include <stdbool.h>

typedef struct {
    int flashTimer;
    int titleScreenTimer;
    bool exploded;
    int explosionRadius;
    int nuclearExplosionFrame;
    int transitionFrame;
    float bombYOffset;
    float frameAnimation;
} TitleState;

void title_initialize(TitleState *title);
bool title_draw_screen(TitleState *title);
void title_begin_transition(TitleState *title);
void title_draw_transition_overlay(TitleState *title);
bool title_advance_transition(TitleState *title);

#endif
