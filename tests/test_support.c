#include "test_support.h"

#include "audio.h"
#include "render.h"

#include <assert.h>
#include <riv.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static riv_context TEST_CONTEXT;
riv_context *riv = &TEST_CONTEXT;

static uint64_t rngState = 1;
static bool forceRng = false;
static uint64_t forcedRngValue = 0;
static int presentCount = 0;
static int presentedKeys[8];
static int presentedKeyCount = 0;
static int presentedKeyIndex = 0;
static int audioPollCount = 0;

void test_runtime_reset(void) {
    memset(&TEST_CONTEXT, 0, sizeof(TEST_CONTEXT));
    TEST_CONTEXT.target_fps = TARGET_FPS;
    rngState = 1;
    forceRng = false;
    forcedRngValue = 0;
    presentCount = 0;
    presentedKeyCount = 0;
    presentedKeyIndex = 0;
    audioPollCount = 0;
}

void test_rng_seed(uint64_t seed) {
    rngState = seed == 0 ? 1 : seed;
    forceRng = false;
}

void test_rng_force(bool enabled, uint64_t value) {
    forceRng = enabled;
    forcedRngValue = value;
}

uint64_t riv_rand_uint(uint64_t high) {
    uint64_t value;
    if (forceRng) {
        value = forcedRngValue;
    } else {
        rngState += UINT64_C(0x9e3779b97f4a7c15);
        value = rngState;
        value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
        value ^= value >> 31;
    }
    return value % (high + 1);
}

uint64_t riv_snprintf(char *buffer, uint64_t size, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(buffer, (size_t)size, format, arguments);
    va_end(arguments);
    return written < 0 ? 0 : (uint64_t)written;
}

bool riv_present(void) {
    presentCount++;
    if (presentedKeyIndex < presentedKeyCount) {
        for (int key = 0; key < RIV_NUM_KEYCODE; key++) {
            riv->keys[key].press = false;
        }
        riv->keys[presentedKeys[presentedKeyIndex]].press = true;
        presentedKeyIndex++;
        return true;
    }
    return false;
}

void test_present_with_key(int key) {
    test_present_with_keys(&key, 1);
}

void test_present_with_keys(const int *keys, int count) {
    assert(count >= 0 && count <= (int)(sizeof(presentedKeys) / sizeof(presentedKeys[0])));
    for (int i = 0; i < count; i++) {
        presentedKeys[i] = keys[i];
    }
    presentedKeyCount = count;
    presentedKeyIndex = 0;
}

int test_present_count(void) {
    return presentCount;
}

int test_audio_poll_count(void) {
    return audioPollCount;
}

bool audio_initialize(AudioState *audio) {
    memset(audio, 0, sizeof(*audio));
    return true;
}

void audio_destroy(AudioState *audio) {
    (void)audio;
}

void audio_poll(AudioState *audio) {
    (void)audio;
    audioPollCount++;
}

void audio_start_background(AudioState *audio) {
    audio->backgroundMusicPlaying = true;
}

void audio_stop_background(AudioState *audio) {
    audio->backgroundMusicPlaying = false;
    audio->backgroundMusicId = 0;
}

void audio_play_reveal(void) {}
void audio_play_game_over(void) {}
void audio_play_explosion(void) {}
void audio_play_level_clear(void) {}
void audio_play_start_game(void) {}
void audio_play_timer_tick(void) {}
void audio_play_enhanced_explosion(void) {}

void render_draw_game(const GameState *game) {
    (void)game;
}

void render_draw_end_screen(const GameState *game) {
    (void)game;
}

void render_draw_fold_dialog(int finalCoins) {
    (void)finalCoins;
}

void render_draw_pixelated_bomb(int x, int y, int size) {
    (void)x;
    (void)y;
    (void)size;
}
