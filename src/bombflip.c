#include "audio.h"
#include "controls.h"
#include "game.h"
#include "render.h"
#include "state.h"
#include "title.h"

#include <riv.h>

typedef struct {
    AppMode mode;
    GameState game;
    TitleState title;
    AudioState audio;
} Application;

static void configure_console(void) {
    riv->width = SCREEN_SIZE;
    riv->height = SCREEN_SIZE;
    riv->target_fps = TARGET_FPS;

    riv->tracked_keys[RIV_GAMEPAD_UP] = true;
    riv->tracked_keys[RIV_GAMEPAD_DOWN] = true;
    riv->tracked_keys[RIV_GAMEPAD_LEFT] = true;
    riv->tracked_keys[RIV_GAMEPAD_RIGHT] = true;
    riv->tracked_keys[CONTROL_REVEAL] = true;
    riv->tracked_keys[CONTROL_SCANNER] = true;
    riv->tracked_keys[CONTROL_FOLD_MENU] = true;
    riv->tracked_keys[CONTROL_FOLD_MENU_ALT] = true;
    riv->tracked_keys[CONTROL_START] = true;

#if CHEATS_ENABLED
    riv->tracked_keys[CONTROL_CHEAT_COMPLETE_LEVEL] = true;
#endif
}

static void initialize_application(Application *app) {
    app->mode = APP_MODE_TITLE;
    game_initialize(&app->game);
    title_initialize(&app->title);
}

static void begin_game_transition(Application *app) {
    /* The first board is generated here, exactly when the run starts. */
    game_begin_run(&app->game);
    title_begin_transition(&app->title);
    app->mode = APP_MODE_TRANSITION;
}

static void draw_transition(Application *app) {
    render_draw_game(&app->game);
    title_draw_transition_overlay(&app->title);
    if (title_advance_transition(&app->title)) {
        app->mode = APP_MODE_PLAYING;
        audio_start_background(&app->audio);
    }
}

static void draw_current_frame(Application *app) {
    switch (app->mode) {
    case APP_MODE_TITLE:
        if (riv->keys[CONTROL_REVEAL].press || riv->keys[CONTROL_START].press) {
            begin_game_transition(app);
            draw_transition(app);
        } else if (title_draw_screen(&app->title)) {
            game_finish(&app->game, GAME_END_EASTER_EGG, &app->audio);
            app->mode = APP_MODE_GAME_OVER;
        }
        break;

    case APP_MODE_TRANSITION:
        draw_transition(app);
        break;

    case APP_MODE_PLAYING:
        game_update(&app->game, &app->audio);
        if (app->game.phase == GAME_PHASE_FINISHED) {
            app->mode = APP_MODE_GAME_OVER;
            render_draw_end_screen(&app->game);
        } else {
            render_draw_game(&app->game);
        }
        game_update_outcard(&app->game);
        break;

    case APP_MODE_GAME_OVER:
        render_draw_end_screen(&app->game);
        break;
    }
}

int main(void) {
    Application app;
    configure_console();
    initialize_application(&app);
    if (!audio_initialize(&app.audio)) {
        return 1;
    }

    while (riv_present()) {
        draw_current_frame(&app);
        audio_poll(&app.audio);
    }

    audio_destroy(&app.audio);
    return 0;
}
