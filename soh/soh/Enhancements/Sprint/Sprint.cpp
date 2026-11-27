#include <libultraship/bridge.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"
#include <ship/Context.h>
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

static bool SprintEnabled() {
    return CVarGetInteger(CVAR_SPRINT, 0) != 0;
}

static void ClearSprint() {
    sToggled = false;
    sActive = false;
    sStillFrames = 0;
    sDustTimer = 0;
}

static bool CanSprintNow(Player* player) {
    constexpr u32 blocked = PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_IN_WATER | PLAYER_STATE1_CLIMBING_LEDGE |
                            PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_TALKING |
                            PLAYER_STATE1_IN_CUTSCENE;
    return (player->stateFlags1 & blocked) == 0 && player->currentBoots != PLAYER_BOOTS_IRON &&
           (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) != 0;
}


// ---- Debug: records Link's real skeleton and run cycle to SOHR/sprint_dump.txt so a sprint animation can be
// authored against the true bone layout instead of guessing joint axes.
#define CVAR_SPRINT_DUMP CVAR_ENHANCEMENT("SprintDump")
static std::vector<std::string> sDumpFrames;
static bool sDumpHeaderDone = false;
static std::string sDumpHeader;

static void RecordRunDump() {
    if (gPlayState == nullptr) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    SkelAnime* sk = &player->skelAnime;
    if (!sDumpHeaderDone) {
        sDumpHeaderDone = true;
        char buf[256];
        snprintf(buf, sizeof(buf), "age=%d limbCount=%d\n", (int)gSaveContext.linkAge, (int)sk->limbCount);
        sDumpHeader = buf;
        for (int i = 0; i < sk->limbCount; i++) {
            StandardLimb* limb = (StandardLimb*)SEGMENTED_TO_VIRTUAL(sk->skeleton[i]);
            snprintf(buf, sizeof(buf), "limb %d pos %d %d %d child %d sibling %d\n", i, (int)limb->jointPos.x,
                     (int)limb->jointPos.y, (int)limb->jointPos.z, (int)limb->child, (int)limb->sibling);
            sDumpHeader += buf;
        }
    }
    if (player->linearVelocity < 4.0f || (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) == 0) {
        return;
    }
    std::string line;
    char buf[160];
    snprintf(buf, sizeof(buf), "frame anim=%p len=%.1f cur=%.2f speed=%.2f vel=%.2f joints=", sk->animation,
             sk->animLength, sk->curFrame, sk->playSpeed, player->linearVelocity);
    line = buf;
    for (int i = 0; i <= sk->limbCount; i++) {
        snprintf(buf, sizeof(buf), "%d,%d,%d;", (int)sk->jointTable[i].x, (int)sk->jointTable[i].y,
                 (int)sk->jointTable[i].z);
        line += buf;
    }
    sDumpFrames.push_back(line + "\n");
    if (sDumpFrames.size() >= 240) {
        std::string path = Ship::Context::GetPathRelativeToAppDirectory("sprint_dump.txt");
        if (FILE* f = fopen(path.c_str(), "w")) {
            fputs(sDumpHeader.c_str(), f);
            for (const auto& l : sDumpFrames) {
                fputs(l.c_str(), f);
            }
            fclose(f);
        }
        sDumpFrames.clear();
        sDumpHeaderDone = false;
        CVarSetInteger(CVAR_SPRINT_DUMP, 0);
    }
}

static void UpdateSprint() {
    if (CVarGetInteger(CVAR_SPRINT_DUMP, 0)) {
        RecordRunDump();
    } else if (!sDumpFrames.empty()) {
        sDumpFrames.clear();
        sDumpHeaderDone = false;
    }
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
    return std::clamp(CVarGetInteger(CVAR_SPRINT_SPEED, 130), 110, 300) / 100.0f;
}

extern "C" float Sprint_GetAnimFactor(Player* player) {
    if (!sActive || player != GET_PLAYER(gPlayState)) {
        return 1.0f;
    }
    // Speed already drives the run cycle; add a quicker, more urgent leg cadence on top.
    return 1.0f + (Sprint_GetRunFactor(player) - 1.0f) * 0.3f;
}

static void RegisterSprint() {
    ClearSprint();
    COND_HOOK(OnPlayerUpdate, SprintEnabled() || CVarGetInteger(CVAR_SPRINT_DUMP, 0), UpdateSprint);
    COND_HOOK(OnPlayDestroy, true, ClearSprint);
}

static RegisterShipInitFunc initFunc(RegisterSprint, { CVAR_SPRINT, CVAR_SPRINT_DUMP });
