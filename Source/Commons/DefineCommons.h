#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"


#define DBG_SCREEN(KEY, TIME, COLOR, FMT, ...) \
do { \
    if (GEngine) { \
        GEngine->AddOnScreenDebugMessage((KEY), (TIME), (COLOR), FString::Printf(TEXT(FMT), ##__VA_ARGS__)); \
    } \
} while(0)

class PJ_QUIET_PROTOCOL_API DefineCommons
{
};