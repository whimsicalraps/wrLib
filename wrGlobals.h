#pragma once

#define HALF24f     4194304.0f
#define MAX24f      8388607.0f
#define MAX24f2     16777215.0f
#define iMAX24f     (1 / MAX24f)

#define MAX24b2     0x00FFFFFF
#define MAX24b      0x007FFFFF
#define HALF24b     0x003FFFFF
#define MIN24b      0xFF800000

#define nFloor      0.00024 // -72dB
#define nFloorSub1  (1 - nFloor) // -0.01dB?
#define nMAX24f     0xFF800000

#define WR_PI       3.14159265358979323846264