#pragma once
#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif

// Ground run speed multiplier while sprinting (1.0 when not sprinting).
float Sprint_GetRunFactor(Player* player);
// Extra leg-cadence multiplier applied on top of the speed-driven run animation (1.0 when not sprinting).
float Sprint_GetAnimFactor(Player* player);

// Sprint pose strength, 0..1 (eased in and out together with the speed wind-up).
float Sprint_GetPose(Player* player);

// Pose amounts, measured against Link's real skeleton (run data FK analysis): angles in binang (65536 = 360 deg).
#define SPRINT_LEAN_RAD (22.0f * 3.14159265f / 180.0f) // extra forward torso lean
#define SPRINT_HEAD_PITCH 2548                         // 14 deg, head compensation
#define SPRINT_ELBOW_BEND 5825                         // 32 deg extra elbow flex
#define SPRINT_ELBOW_OUT 1820                          // 10 deg elbows out

#ifdef __cplusplus
}
#endif
