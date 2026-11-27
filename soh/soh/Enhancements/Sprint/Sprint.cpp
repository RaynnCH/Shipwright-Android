#include <libultraship/bridge.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include <ship/Context.h>
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


<<<<<<< HEAD
=======
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

>>>>>>> 4f7e6643d (Sprint: add run data recorder for accurate animation authoring)
// ---- Temporary: records Link's real skeleton and run cycle to SOHR/sprint_dump.txt.
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
    // skelAnime.limbCount counts the root translation slot too; the skeleton itself has limbCount - 1 limbs.
    const int limbs = sk->limbCount - 1;
    if (sk->skeleton == nullptr || sk->jointTable == nullptr || limbs <= 0 || limbs > 40) {
        return;
    }
    if (!sDumpHeaderDone) {
        sDumpHeaderDone = true;
        char buf[256];
        snprintf(buf, sizeof(buf), "age=%d limbs=%d\n", (int)gSaveContext.linkAge, limbs);
        sDumpHeader = buf;
        for (int i = 0; i < limbs; i++) {
            StandardLimb* limb = (StandardLimb*)SEGMENTED_TO_VIRTUAL(sk->skeleton[i]);
            if (limb == nullptr) {
                continue;
            }
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
    snprintf(buf, sizeof(buf), "frame len=%.1f cur=%.2f speed=%.2f vel=%.2f joints=", sk->animLength, sk->curFrame,
             sk->playSpeed, player->linearVelocity);
    line = buf;
    for (int i = 0; i < sk->limbCount; i++) {
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
    COND_HOOK(OnPlayerUpdate, SprintEnabled() || CVarGetInteger(CVAR_SPRINT_DUMP, 0), UpdateSprint);
    COND_HOOK(OnPlayDestroy, true, ClearSprint);
}

static RegisterShipInitFunc initFunc(RegisterSprint, { CVAR_SPRINT, CVAR_SPRINT_DUMP });
