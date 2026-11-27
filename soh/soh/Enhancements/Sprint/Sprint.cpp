#include <libultraship/bridge.h>
#include <algorithm>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"
#include "Sprint.h"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
}

#define CVAR_SPRINT CVAR_ENHANCEMENT("Sprint")
#define CVAR_SPRINT_MODE CVAR_ENHANCEMENT("SprintMode") // 0 = toggle, 1 = hold
constexpr float kSprintSpeed = 1.3f;     // fixed 130% top speed
constexpr float kSprintRampFrames = 20.0f; // wind-up: frames to reach full sprint speed

static bool sToggled = false;
static bool sActive = false;
static int sStillFrames = 0;
static int sDustTimer = 0;
static float sRamp = 0.0f; // 0..1 wind-up toward full sprint speed

static bool SprintEnabled() {
    return CVarGetInteger(CVAR_SPRINT, 0) != 0;
}

static void ClearSprint() {
    sToggled = false;
    sActive = false;
    sStillFrames = 0;
    sDustTimer = 0;
    sRamp = 0.0f;
}

static bool CanSprintNow(Player* player) {
    constexpr u32 blocked = PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_IN_WATER | PLAYER_STATE1_CLIMBING_LEDGE |
                            PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_TALKING |
                            PLAYER_STATE1_IN_CUTSCENE;
    return (player->stateFlags1 & blocked) == 0 && player->currentBoots != PLAYER_BOOTS_IRON &&
           (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) != 0;
}


static void UpdateSprint() {
    if (gPlayState == nullptr || !SprintEnabled()) {
        ClearSprint();
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    Input* input = &gPlayState->state.input[0];
    const bool held = CHECK_BTN_ALL(input->cur.button, BTN_CUSTOM_SPRINT);
    const bool pressed = CHECK_BTN_ALL(input->press.button, BTN_CUSTOM_SPRINT);
    const bool moving = player->linearVelocity > 3.0f;
    const bool allowed = CanSprintNow(player);

    if (CVarGetInteger(CVAR_SPRINT_MODE, 0) == 1) {
        sActive = held && moving && allowed;
    } else {
        if (pressed) {
            sToggled = !sToggled;
        }
        // Toggle ends when Link stops for a moment or can no longer sprint (water, ladders, talking...).
        sStillFrames = moving ? 0 : sStillFrames + 1;
        if (!allowed || sStillFrames > 10) {
            sToggled = false;
        }
        sActive = sToggled && moving && allowed;
    }

    // Ease into the sprint instead of snapping to speed; ease back out when it ends.
    sRamp = sActive ? std::min(1.0f, sRamp + 1.0f / kSprintRampFrames) : std::max(0.0f, sRamp - 2.0f / kSprintRampFrames);

    if (sActive) {
        // Kicked-up dust at the feet, a few frames apart, like the run-dust used when stopping.
        if (--sDustTimer <= 0) {
            sDustTimer = 4;
            Vec3f vel = { 0.0f, 0.5f, 0.0f };
            Vec3f accel = { 0.0f, 0.1f, 0.0f };
            Vec3f* foot = &player->actor.shape.feetPos[(gPlayState->gameplayFrames & 4) ? FOOT_LEFT : FOOT_RIGHT];
            Vec3f pos = *foot;
            func_800286CC(gPlayState, &pos, &vel, &accel, 60, 25);
        }
    } else {
        sDustTimer = 0;
    }
}

extern "C" float Sprint_GetRunFactor(Player* player) {
    if (sRamp <= 0.0f || player != GET_PLAYER(gPlayState)) {
        return 1.0f;
    }
    const float t = sRamp * sRamp * (3.0f - 2.0f * sRamp); // smoothstep
    return 1.0f + (kSprintSpeed - 1.0f) * t;
}

extern "C" float Sprint_GetAnimFactor(Player* player) {
    if (sRamp <= 0.0f || player != GET_PLAYER(gPlayState)) {
        return 1.0f;
    }
    // Run playback already scales with velocity. Like Dawnlight, play the run cycle at only half the speed bonus
    // (130% movement -> 115% playback) so the stride does not look sped up; divide out the velocity scaling.
    float run = Sprint_GetRunFactor(player);
    return (1.0f + (run - 1.0f) * 0.5f) / run;
}

static void RegisterSprint() {
    ClearSprint();
    COND_HOOK(OnPlayerUpdate, SprintEnabled(), UpdateSprint);
    COND_HOOK(OnPlayDestroy, true, ClearSprint);
}

static RegisterShipInitFunc initFunc(RegisterSprint, { CVAR_SPRINT });
