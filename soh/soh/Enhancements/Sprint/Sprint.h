#pragma once
#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif

// Ground run speed multiplier while sprinting (1.0 when not sprinting).
float Sprint_GetRunFactor(Player* player);
// Extra leg-cadence multiplier applied on top of the speed-driven run animation (1.0 when not sprinting).
float Sprint_GetAnimFactor(Player* player);

#ifdef __cplusplus
}
#endif
