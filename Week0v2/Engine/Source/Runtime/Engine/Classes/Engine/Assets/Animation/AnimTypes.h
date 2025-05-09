#pragma once
#include "Container/Array.h"
#include "Math/Quat.h"
#include "Math/Vector.h"
#include "UObject/NameTypes.h"

#define DEFAULT_SAMPLERATE			30.f
#define MINIMUM_ANIMATION_LENGTH	(1/DEFAULT_SAMPLERATE)
/**
* Below this weight threshold, animations won't be blended in.
*/
#define ZERO_ANIMWEIGHT_THRESH (0.00001f)

// Enable this if you want to locally measure detailed anim perf.  Disabled by default as it is introduces a lot of additional profile markers and associated overhead.
#define ENABLE_VERBOSE_ANIM_PERF_TRACKING 0

#define MAX_ANIMATION_TRACKS 65535

struct FRawAnimSequenceTrack
{
    TArray<FVector> PosKeys;
    TArray<FQuat> RotKeys;
    TArray<FVector> ScaleKeys;
};

struct FAnimWeight
{
    /** Helper function to determine if a weight is relevant. */
    static FORCEINLINE bool IsRelevant(float InWeight)
    {
        return (InWeight > ZERO_ANIMWEIGHT_THRESH);
    }

    /** Helper function to determine if a normalized weight is considered full weight. */
    static FORCEINLINE bool IsFullWeight(float InWeight)
    {
        return (InWeight >= (1.f - ZERO_ANIMWEIGHT_THRESH));
    }

    /** Get a small relevant weight for ticking */
    static FORCEINLINE float GetSmallestRelevantWeight()
    {
        return 2.f * ZERO_ANIMWEIGHT_THRESH;
    }
};

enum class EAnimInterpolationType : uint8
{
    /** Linear interpolation when looking up values between keys. */
    Linear,

    /** Step interpolation when looking up values between keys. */
    Step,
};


