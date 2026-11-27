#pragma once
#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif

// Ground run speed multiplier while sprinting (1.0 when not sprinting).
float Sprint_GetRunFactor(Player* player);
// Extra leg-cadence multiplier applied on top of the speed-driven run animation (1.0 when not sprinting).
float Sprint_GetAnimFactor(Player* player);

// Procedural run pose, blended in while sprinting (binary angles, 0 when not sprinting):
// forward hunch added to the upper body (positive = forward) and elbow bend added to both forearms.
s16 Sprint_GetLean(Player* player);
s16 Sprint_GetArmBend(Player* player);

#ifdef __cplusplus
}
#endif
