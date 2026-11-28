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

// Stride and arm swing are widened by scaling the run animation's swing around its own mean pose. The means are the
// circular means of the recorded run cycle (the run animation is shared by both ages).
#define SPRINT_STRIDE 1.15f    // +15% thigh swing -> longer steps
#define SPRINT_ARM_SWING 1.25f // +25% arm swing
#define SPRINT_MEAN_THIGH_R_Z (-4426)
#define SPRINT_MEAN_THIGH_L_Z (-3435)
#define SPRINT_MEAN_SHOULDER_L_Z (-30414)
#define SPRINT_MEAN_SHOULDER_R_Z (-30623)
#define SPRINT_MEAN_SHOULDER_L_X (-2460)
#define SPRINT_MEAN_SHOULDER_R_X 1373

// Scales (value - mean) by k, wrapping correctly around the s16 angle circle.
static inline s16 Sprint_Widen(s16 value, s16 mean, float k) {
    return (s16)(mean + (s16)(((s16)(value - mean)) * k));
}

#ifdef __cplusplus
}
#endif
