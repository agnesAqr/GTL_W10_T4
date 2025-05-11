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
    TArray<float> KeyTimes;    // 각 키의 시간(초)
    TArray<FVector> PosKeys;
    TArray<FQuat> RotKeys;
    TArray<FVector> ScaleKeys;
};

struct FAnimWeight
{
    /** 가중치가 유효한(적용 가능한)지 판단하는 헬퍼 함수 */
    static FORCEINLINE bool IsRelevant(const float InWeight)
    {
        return (InWeight > ZERO_ANIMWEIGHT_THRESH);
    }

    /** 정규화된 가중치가 최대치(1.0)에 근접했는지 판단하는 헬퍼 함수 */
    static FORCEINLINE bool IsFullWeight(const float InWeight)
    {
        return (InWeight >= (1.f - ZERO_ANIMWEIGHT_THRESH));
    }

    /** 최소한으로 유효한 가중치를 반환하는 함수 */
    static FORCEINLINE float GetSmallestRelevantWeight()
    {
        return 2.f * ZERO_ANIMWEIGHT_THRESH;
    }
};

enum class EAnimInterpolationType : uint8
{
    /** 키 사이 값을 조회할 때 선형 보간(Linear Interpolation) */
    Linear,

    /** 키 사이 값을 조회할 때 단계별 보간(Step Interpolation) */
    Step,
};


