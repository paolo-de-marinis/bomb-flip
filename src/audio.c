#include "audio.h"

#include <riv.h>
#include <stdio.h>

#define SEQT_IMPL
#include "seqt.h"

static riv_waveform_desc REVEAL_SOUND = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f,
    .decay = 0.01f,
    .sustain = 0.1f,
    .release = 0.01f,
    .start_frequency = RIV_NOTE_C4,
    .end_frequency = RIV_NOTE_C5,
    .amplitude = 0.25f,
    .sustain_level = 0.5f,
};

static riv_waveform_desc GAME_OVER_SOUND = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f,
    .decay = 0.01f,
    .sustain = 0.2f,
    .release = 0.1f,
    .start_frequency = RIV_NOTE_A3,
    .end_frequency = RIV_NOTE_A2,
    .amplitude = 0.5f,
    .sustain_level = 0.5f,
};

static riv_waveform_desc EXPLOSION_SOUND = {
    .type = RIV_WAVEFORM_NOISE,
    .attack = 0.01f,
    .decay = 0.1f,
    .sustain = 0.2f,
    .release = 0.3f,
    .start_frequency = 100.0f,
    .end_frequency = 50.0f,
    .amplitude = 0.5f,
    .sustain_level = 0.3f,
};

static riv_waveform_desc LEVEL_CLEAR_SOUNDS[] = {
    {.id = 201,
     .type = RIV_WAVEFORM_TRIANGLE,
     .attack = 0.05f,
     .decay = 0.1f,
     .sustain = 0.2f,
     .release = 0.1f,
     .start_frequency = 523.25f,
     .end_frequency = 523.25f,
     .amplitude = 0.25f,
     .sustain_level = 0.5f,
     .duty_cycle = 0.5f,
     .pan = -0.2f},
    {.id = 202,
     .type = RIV_WAVEFORM_TRIANGLE,
     .attack = 0.05f,
     .decay = 0.1f,
     .sustain = 0.2f,
     .release = 0.1f,
     .start_frequency = 659.25f,
     .end_frequency = 659.25f,
     .amplitude = 0.25f,
     .sustain_level = 0.5f,
     .duty_cycle = 0.5f,
     .pan = 0.2f},
    {.id = 203,
     .type = RIV_WAVEFORM_TRIANGLE,
     .attack = 0.05f,
     .decay = 0.1f,
     .sustain = 0.3f,
     .release = 0.2f,
     .start_frequency = 783.99f,
     .end_frequency = 783.99f,
     .amplitude = 0.25f,
     .sustain_level = 0.5f,
     .duty_cycle = 0.5f,
     .pan = 0.0f},
    {.id = 204,
     .type = RIV_WAVEFORM_TRIANGLE,
     .attack = 0.05f,
     .decay = 0.1f,
     .sustain = 0.4f,
     .release = 0.3f,
     .start_frequency = 1046.50f,
     .end_frequency = 1046.50f,
     .amplitude = 0.25f,
     .sustain_level = 0.5f,
     .duty_cycle = 0.5f,
     .pan = 0.0f},
};

static riv_waveform_desc START_GAME_SOUND = {
    .type = RIV_WAVEFORM_SQUARE,
    .attack = 0.01f,
    .decay = 0.05f,
    .sustain = 0.1f,
    .release = 0.1f,
    .start_frequency = RIV_NOTE_C5,
    .end_frequency = RIV_NOTE_G5,
    .amplitude = 0.3f,
    .sustain_level = 0.5f,
};

static riv_waveform_desc ENHANCED_EXPLOSION_SOUNDS[] = {
    {.type = RIV_WAVEFORM_NOISE,
     .attack = 0.01f,
     .decay = 0.1f,
     .sustain = 0.3f,
     .release = 0.5f,
     .start_frequency = 100.0f,
     .end_frequency = 50.0f,
     .amplitude = 0.6f,
     .sustain_level = 0.4f},
    {.type = RIV_WAVEFORM_NOISE,
     .attack = 0.005f,
     .decay = 0.05f,
     .sustain = 0.2f,
     .release = 0.3f,
     .start_frequency = 200.0f,
     .end_frequency = 100.0f,
     .amplitude = 0.5f,
     .sustain_level = 0.3f},
    {.type = RIV_WAVEFORM_NOISE,
     .attack = 0.02f,
     .decay = 0.15f,
     .sustain = 0.4f,
     .release = 0.6f,
     .start_frequency = 80.0f,
     .end_frequency = 40.0f,
     .amplitude = 0.7f,
     .sustain_level = 0.5f},
};

bool audio_initialize(AudioState *audio) {
    seqt_init();
    audio->backgroundMusic = seqt_make_source_from_file("songs/gameplay.rivcard");
    audio->backgroundMusicId = 0;
    audio->backgroundMusicPlaying = false;
    if (audio->backgroundMusic == NULL) {
        printf("Failed to load background music\n");
        return false;
    }
    return true;
}

void audio_destroy(AudioState *audio) {
    if (audio->backgroundMusic != NULL) {
        seqt_destroy_source(audio->backgroundMusic);
        audio->backgroundMusic = NULL;
    }
}

void audio_poll(AudioState *audio) {
    if (audio->backgroundMusicPlaying) {
        seqt_poll();
    }
}

void audio_start_background(AudioState *audio) {
    if (audio->backgroundMusicPlaying) {
        return;
    }
    audio->backgroundMusicId = seqt_play(audio->backgroundMusic, -1);
    audio->backgroundMusicPlaying = true;
}

void audio_stop_background(AudioState *audio) {
    audio->backgroundMusicPlaying = false;
    if (audio->backgroundMusicId != 0) {
        seqt_stop(audio->backgroundMusicId);
        audio->backgroundMusicId = 0;
    }
}

void audio_play_reveal(void) {
    riv_waveform(&REVEAL_SOUND);
}

void audio_play_game_over(void) {
    riv_waveform(&GAME_OVER_SOUND);
}

void audio_play_explosion(void) {
    riv_waveform(&EXPLOSION_SOUND);
}

void audio_play_level_clear(void) {
    int count = (int)(sizeof(LEVEL_CLEAR_SOUNDS) / sizeof(LEVEL_CLEAR_SOUNDS[0]));
    for (int i = 0; i < count; i++) {
        riv_waveform_desc note = LEVEL_CLEAR_SOUNDS[i];
        note.delay = i * 0.2f;
        riv_waveform(&note);
    }
}

void audio_play_start_game(void) {
    riv_waveform(&START_GAME_SOUND);
}

void audio_play_timer_tick(void) {
    riv_waveform_desc tick = {
        .type = RIV_WAVEFORM_SQUARE,
        .attack = 0.01f,
        .decay = 0.01f,
        .sustain = 0.05f,
        .release = 0.01f,
        .start_frequency = 1000.0f,
        .end_frequency = 1000.0f,
        .amplitude = 0.2f,
        .sustain_level = 0.2f,
    };
    riv_waveform(&tick);
}

void audio_play_enhanced_explosion(void) {
    int count = (int)(sizeof(ENHANCED_EXPLOSION_SOUNDS) /
                      sizeof(ENHANCED_EXPLOSION_SOUNDS[0]));
    for (int i = 0; i < count; i++) {
        riv_waveform_desc sound = ENHANCED_EXPLOSION_SOUNDS[i];
        sound.delay = i * 0.1f;
        riv_waveform(&sound);
    }
}
