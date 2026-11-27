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
#define CVAR_SPRINT_SPEED CVAR_ENHANCEMENT("SprintSpeed") // percent

static bool sToggled = false;
static bool sActive = false;
static int sStillFrames = 0;
static int sDustTimer = 0;
static float sBlend = 0.0f; // 0..1 ease in/out of the sprint pose

static bool SprintEnabled() {
    return CVarGetInteger(CVAR_SPRINT, 0) != 0;
}

static void ClearSprint() {
    sToggled = false;
    sActive = false;
    sStillFrames = 0;
    sDustTimer = 0;
    sBlend = 0.0f;
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

    // Ease the pose in and out so the hunch and arm pump do not pop.
    sBlend = sActive ? std::min(1.0f, sBlend + 0.12f) : std::max(0.0f, sBlend - 0.10f);

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
    if (!sActive || player != GET_PLAYER(gPlayState)) {
        return 1.0f;
    }
    return std::clamp(CVarGetInteger(CVAR_SPRINT_SPEED, 170), 110, 300) / 100.0f;
}

extern "C" float Sprint_GetAnimFactor(Player* player) {
    if (!sActive || player != GET_PLAYER(gPlayState)) {
        return 1.0f;
    }
    // Speed already drives the run cycle; add a quicker, more urgent leg cadence on top.
    return 1.0f + (Sprint_GetRunFactor(player) - 1.0f) * 0.3f;
}

static s16 DegToBinang(int degrees) {
    return static_cast<s16>(degrees * (0x8000 / 180.0f));
}

extern "C" s16 Sprint_GetLean(Player* player) {
    if (sBlend <= 0.0f || gPlayState == nullptr || player != GET_PLAYER(gPlayState)) {
        return 0;
    }
    return static_cast<s16>(DegToBinang(std::clamp(CVarGetInteger(CVAR_ENHANCEMENT("SprintLean"), 14), -40, 40)) *
                            sBlend);
}

extern "C" s16 Sprint_GetArmBend(Player* player) {
    if (sBlend <= 0.0f || gPlayState == nullptr || player != GET_PLAYER(gPlayState)) {
        return 0;
    }
    return static_cast<s16>(
        DegToBinang(std::clamp(CVarGetInteger(CVAR_ENHANCEMENT("SprintArmBend"), 60), -100, 100)) * sBlend);
}

static void RegisterSprint() {
    ClearSprint();
    COND_HOOK(OnPlayerUpdate, SprintEnabled(), UpdateSprint);
    COND_HOOK(OnPlayDestroy, true, ClearSprint);
}

static RegisterShipInitFunc initFunc(RegisterSprint, { CVAR_SPRINT });
