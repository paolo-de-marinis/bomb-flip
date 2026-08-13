#include "title.h"

#include "render.h"
#include "state.h"

#include <math.h>
#include <riv.h>

#define PI 3.14159265358979323846f

enum {
    FRAME_RADIUS = 200,
    FRAME_THICKNESS = 50,
    TITLE_EASTER_EGG_DELAY_FRAMES = 3600,
    NUCLEAR_EXPLOSION_DURATION = 300,
    TRANSITION_FRAMES = 60,
    TRANSITION_EXTRA_FRAMES = 10
};

static const float FRAME_ANIMATION_SPEED = 0.05f;
static const float BOMB_BOUNCE_SPEED = 2.0f;
static const float BOMB_BOUNCE_AMPLITUDE = 5.0f;

void title_initialize(TitleState *title) {
    *title = (TitleState){0};
}

static void draw_standard_title(TitleState *title) {
    riv_clear(RIV_COLOR_BLUE);

    int centerX = SCREEN_SIZE / 2;
    int centerY = SCREEN_SIZE / 2;
    title->frameAnimation += FRAME_ANIMATION_SPEED;
    if (title->frameAnimation >= 1.0f) {
        title->frameAnimation -= 1.0f;
    }

    for (int i = 0; i < FRAME_THICKNESS; i++) {
        float progress = (float)i / FRAME_THICKNESS;
        float animatedProgress = fmodf(progress + title->frameAnimation, 1.0f);
        int radius = FRAME_RADIUS + i;
        uint32_t color = i % 2 == 0 ? RIV_COLOR_BLUE : RIV_COLOR_LIGHTBLUE;
        if (animatedProgress < 0.5f) {
            riv_draw_circle_line(centerX, centerY, radius, color);
        }
    }

    riv_draw_text("BOMB FLIP",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE / 3,
                  3,
                  RIV_COLOR_YELLOW);
    riv_draw_text("BETA VERSION",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  SCREEN_SIZE * 3 / 4 + 20,
                  1,
                  RIV_COLOR_RED);

    title->flashTimer++;
    if (title->flashTimer / 30 % 2 == 0) {
        riv_draw_text("Press Start",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      SCREEN_SIZE * 3 / 4,
                      1,
                      RIV_COLOR_WHITE);
    }

    title->bombYOffset =
        sinf(riv->frame * BOMB_BOUNCE_SPEED * 0.1f) * BOMB_BOUNCE_AMPLITUDE;
    render_draw_pixelated_bomb(
        SCREEN_SIZE / 2, SCREEN_SIZE / 2 + (int)title->bombYOffset, 48);

    title->titleScreenTimer++;
    if (title->titleScreenTimer >= TITLE_EASTER_EGG_DELAY_FRAMES) {
        title->exploded = true;
        title->explosionRadius = 1;
        audio_play_enhanced_explosion();
    }
}

static void draw_growing_explosion(TitleState *title) {
    riv_draw_circle_fill(
        SCREEN_SIZE / 2, SCREEN_SIZE / 2, title->explosionRadius, RIV_COLOR_RED);
    riv_draw_circle_fill(SCREEN_SIZE / 2,
                         SCREEN_SIZE / 2,
                         (int)(title->explosionRadius * 0.8f),
                         RIV_COLOR_ORANGE);
    riv_draw_circle_fill(SCREEN_SIZE / 2,
                         SCREEN_SIZE / 2,
                         (int)(title->explosionRadius * 0.6f),
                         RIV_COLOR_YELLOW);

    int previousRadius = title->explosionRadius;
    title->explosionRadius += 5;
    if (previousRadius / 50 != title->explosionRadius / 50) {
        audio_play_enhanced_explosion();
    }
}

static void draw_jungle_scene(void) {
    for (int i = 0; i < 5; i++) {
        int treeX = -10 + i * 60;
        int treeY = SCREEN_SIZE * 2 / 3;
        int treeHeight = 80 + (int)(sinf(i * 1.5f) * 20);
        int trunkWidth = 6 + i % 3;
        riv_draw_rect_fill(
            treeX - trunkWidth / 2, treeY - treeHeight, trunkWidth, treeHeight, RIV_COLOR_BROWN);

        int canopyWidth = treeHeight / 2;
        int canopyHeight = treeHeight * 2 / 3;
        for (int j = 0; j < 20; j++) {
            int leafX = treeX + (int)(riv_rand() % canopyWidth) - canopyWidth / 2;
            int leafY = treeY - treeHeight + (int)(riv_rand() % canopyHeight);
            int leafSize = 10 + (int)(riv_rand() % 10);
            uint32_t color = j % 2 == 0 ? RIV_COLOR_GREEN : RIV_COLOR_DARKGREEN;
            riv_draw_circle_fill(leafX, leafY, leafSize, color);
        }

        for (int j = 0; j < 5; j++) {
            int leafX = treeX + (int)(riv_rand() % canopyWidth) - canopyWidth / 2;
            int leafY = treeY - treeHeight + (int)(riv_rand() % canopyHeight);
            int leafSize = 5 + (int)(riv_rand() % 5);
            riv_draw_circle_fill(leafX, leafY, leafSize, RIV_COLOR_LIGHTGREEN);
        }
    }

    int arcadeX = SCREEN_SIZE - 70;
    int arcadeY = SCREEN_SIZE * 2 / 3 - 60;
    int arcadeWidth = 50;
    int arcadeHeight = 60;
    riv_draw_rect_fill(arcadeX, arcadeY, arcadeWidth, arcadeHeight, RIV_COLOR_PURPLE);
    riv_draw_rect_fill(
        arcadeX + 2, arcadeY + 2, arcadeWidth - 4, arcadeHeight - 4, RIV_COLOR_DARKPURPLE);
    riv_draw_rect_fill(
        arcadeX + 5, arcadeY + 5, arcadeWidth - 10, arcadeHeight / 2, RIV_COLOR_BLACK);
    riv_draw_rect_line(
        arcadeX + 4, arcadeY + 4, arcadeWidth - 8, arcadeHeight / 2 + 2, RIV_COLOR_WHITE);
    riv_draw_text("RIVES",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  arcadeX + arcadeWidth / 2,
                  arcadeY + arcadeHeight / 4 + 2,
                  1,
                  RIV_COLOR_GREEN);

    int controlY = arcadeY + arcadeHeight * 3 / 4;
    riv_draw_circle_fill(arcadeX + arcadeWidth / 3, controlY, 4, RIV_COLOR_RED);
    riv_draw_circle_fill(arcadeX + arcadeWidth * 2 / 3, controlY, 4, RIV_COLOR_RED);
    riv_draw_rect_fill(arcadeX + 5, controlY + 10, arcadeWidth - 10, 5, RIV_COLOR_BLACK);

    int benchY = SCREEN_SIZE * 2 / 3 - 20;
    riv_draw_rect_fill(SCREEN_SIZE - 150, benchY, 60, 5, RIV_COLOR_BROWN);
    riv_draw_rect_fill(SCREEN_SIZE - 145, benchY + 5, 5, 15, RIV_COLOR_BROWN);
    riv_draw_rect_fill(SCREEN_SIZE - 95, benchY + 5, 5, 15, RIV_COLOR_BROWN);
}

static void draw_nuclear_cloud(float progress) {
    float easedProgress = 1.0f - (1.0f - progress) * (1.0f - progress);
    int explosionSize = (int)(SCREEN_SIZE * 1.5f * easedProgress);
    int cloudBaseY = SCREEN_SIZE * 2 / 3;

    int shockwaveRadius = (int)(SCREEN_SIZE * 2 * progress);
    riv_draw_circle_line(SCREEN_SIZE / 2, cloudBaseY, shockwaveRadius, RIV_COLOR_WHITE);

    int stemHeight = explosionSize / 2;
    int cloudTopY = cloudBaseY - stemHeight;
    int stemWidth = 20 + (int)(20 * (1.0f - easedProgress));
    riv_draw_rect_fill(SCREEN_SIZE / 2 - stemWidth / 2,
                       cloudBaseY - stemHeight,
                       stemWidth,
                       stemHeight,
                       RIV_COLOR_GREY);

    int cloudRadius = explosionSize / 2;
    riv_draw_circle_fill(SCREEN_SIZE / 2, cloudTopY, cloudRadius, RIV_COLOR_GREY);
    for (int i = 0; i < 8; i++) {
        float angle = i * (PI / 4.0f);
        int puffX = SCREEN_SIZE / 2 + (int)(cosf(angle) * cloudRadius * 0.8f);
        int puffY = cloudTopY + (int)(sinf(angle) * cloudRadius * 0.8f);
        int puffRadius = (int)(cloudRadius * 0.4f);
        riv_draw_circle_fill(puffX, puffY, puffRadius, RIV_COLOR_GREY);
    }

    int lightRadius = cloudRadius / 2;
    riv_draw_circle_fill(SCREEN_SIZE / 2, cloudTopY, lightRadius, RIV_COLOR_YELLOW);
    riv_draw_circle_fill(SCREEN_SIZE / 2, cloudTopY, lightRadius * 2 / 3, RIV_COLOR_WHITE);

    for (int i = 0; i < 30; i++) {
        float debrisProgress = progress * 2.0f > 1.0f ? 1.0f : progress * 2.0f;
        int debrisX = SCREEN_SIZE / 2 +
                      (int)(sinf(i * 0.5f) * explosionSize / 2 * debrisProgress);
        int debrisY = cloudBaseY - i * 5 - (int)(stemHeight * debrisProgress);
        riv_draw_circle_fill(debrisX, debrisY, 2, RIV_COLOR_GREY);
    }
}

static bool draw_nuclear_explosion(TitleState *title) {
    float progress = (float)title->nuclearExplosionFrame / NUCLEAR_EXPLOSION_DURATION;

    if (title->nuclearExplosionFrame < 10) {
        uint32_t color =
            title->nuclearExplosionFrame % 2 == 0 ? RIV_COLOR_WHITE : RIV_COLOR_ORANGE;
        riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE, color);
    } else {
        riv_draw_rect_fill(0, 0, SCREEN_SIZE, SCREEN_SIZE * 2 / 3, RIV_COLOR_PEACH);
        riv_draw_rect_fill(0, SCREEN_SIZE * 2 / 3, SCREEN_SIZE, SCREEN_SIZE / 3, RIV_COLOR_BROWN);
        if (progress < 0.3f) {
            draw_jungle_scene();
        }
        draw_nuclear_cloud(progress);
    }

    if (title->nuclearExplosionFrame > NUCLEAR_EXPLOSION_DURATION / 2) {
        riv_draw_text("BOOM!",
                      RIV_SPRITESHEET_FONT_5X7,
                      RIV_CENTER,
                      SCREEN_SIZE / 2,
                      SCREEN_SIZE / 4,
                      3,
                      RIV_COLOR_RED);
    }

    if (title->nuclearExplosionFrame == 1 ||
        title->nuclearExplosionFrame == NUCLEAR_EXPLOSION_DURATION / 4 ||
        title->nuclearExplosionFrame == NUCLEAR_EXPLOSION_DURATION / 2 ||
        title->nuclearExplosionFrame == 3 * NUCLEAR_EXPLOSION_DURATION / 4) {
        audio_play_enhanced_explosion();
    }

    title->nuclearExplosionFrame++;
    return title->nuclearExplosionFrame >= NUCLEAR_EXPLOSION_DURATION;
}

bool title_draw_screen(TitleState *title) {
    if (!title->exploded) {
        draw_standard_title(title);
        return false;
    }
    if (title->explosionRadius < FRAME_RADIUS * 4) {
        draw_growing_explosion(title);
        return false;
    }
    return draw_nuclear_explosion(title);
}

void title_begin_transition(TitleState *title) {
    title->transitionFrame = 0;
    title->titleScreenTimer = 0;
    title->exploded = false;
    title->explosionRadius = 0;
    title->nuclearExplosionFrame = 0;
    audio_play_start_game();
}

void title_draw_transition_overlay(TitleState *title) {
    int overlayPosition = -((title->transitionFrame * SCREEN_SIZE) / TRANSITION_FRAMES);
    riv_draw_rect_fill(0, overlayPosition, SCREEN_SIZE, SCREEN_SIZE, RIV_COLOR_BLUE);

    int titleY = overlayPosition + SCREEN_SIZE / 2 - SCREEN_SIZE / 6;
    riv_draw_text("BOMB FLIP",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  SCREEN_SIZE / 2,
                  titleY,
                  3,
                  RIV_COLOR_YELLOW);

    title->bombYOffset =
        sinf(riv->frame * BOMB_BOUNCE_SPEED * 0.1f) * BOMB_BOUNCE_AMPLITUDE;
    int bombY = overlayPosition + SCREEN_SIZE / 2 + (int)title->bombYOffset;
    render_draw_pixelated_bomb(SCREEN_SIZE / 2, bombY, 48);

    if (overlayPosition > -SCREEN_SIZE / 2) {
        title->flashTimer++;
        if (title->flashTimer / 30 % 2 == 0) {
            riv_draw_text("Press Start",
                          RIV_SPRITESHEET_FONT_5X7,
                          RIV_CENTER,
                          SCREEN_SIZE / 2,
                          overlayPosition + SCREEN_SIZE - 20,
                          1,
                          RIV_COLOR_WHITE);
        }
    }
}

bool title_advance_transition(TitleState *title) {
    title->transitionFrame++;
    return title->transitionFrame >= TRANSITION_FRAMES + TRANSITION_EXTRA_FRAMES;
}
