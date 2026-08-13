#ifndef BOMB_FLIP_AUDIO_H
#define BOMB_FLIP_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

typedef struct seqt_source seqt_source;

typedef struct {
    seqt_source *backgroundMusic;
    uint64_t backgroundMusicId;
    bool backgroundMusicPlaying;
} AudioState;

bool audio_initialize(AudioState *audio);
void audio_destroy(AudioState *audio);
void audio_poll(AudioState *audio);
void audio_start_background(AudioState *audio);
void audio_stop_background(AudioState *audio);

void audio_play_reveal(void);
void audio_play_game_over(void);
void audio_play_explosion(void);
void audio_play_level_clear(void);
void audio_play_start_game(void);
void audio_play_timer_tick(void);
void audio_play_enhanced_explosion(void);

#endif
