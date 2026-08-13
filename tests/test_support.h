#ifndef BOMB_FLIP_TEST_SUPPORT_H
#define BOMB_FLIP_TEST_SUPPORT_H

#include <stdbool.h>
#include <stdint.h>

void test_runtime_reset(void);
void test_rng_seed(uint64_t seed);
void test_rng_force(bool enabled, uint64_t value);
void test_present_with_key(int key);
void test_present_with_keys(const int *keys, int count);
int test_present_count(void);
int test_audio_poll_count(void);

#endif
