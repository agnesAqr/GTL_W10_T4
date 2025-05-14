#pragma once
#include "Container/Array.h"
#include "Math/Quat.h"
#include "Math/Vector.h"
#include "UObject/NameTypes.h"
#include "Math/Transform.h"
#include "FBX/FBXDefine.h"

#define DEFAULT_SAMPLERATE			30.f
#define MINIMUM_ANIMATION_LENGTH	(1/DEFAULT_SAMPLERATE)
/**
* Below this weight threshold, animations won't be blended in.
*/
#define ZERO_ANIMWEIGHT_THRESH (0.00001f)

// Enable this if you want to locally measure detailed anim perf.  Disabled by default as it is introduces a lot of additional profile markers and associated overhead.
#define ENABLE_VERBOSE_ANIM_PERF_TRACKING 0

#define MAX_ANIMATION_TRACKS 65535

struct FCompactPoseBone // 한 뼈의 로컬 변환
{
    FQuat Rotation;
    FVector Translation;
    FVector Scale3D;

    FCompactPoseBone() : Rotation(FQuat::Identity()), Translation(FVector::ZeroVector), Scale3D(FVector::OneVector) {}

    FTransform ToTransform() const
    {
        return FTransform(Rotation, Translation, Scale3D);
    }
};

struct FPoseData
{
    FPoseData() { LocalBoneTransforms = {}, Skeleton = nullptr; }
    TArray<FCompactPoseBone> LocalBoneTransforms;
    TMap<FName, FCompactPoseBone> LocalBoneTransformMap;

    const FRefSkeletal* Skeleton;

    void Reset()
    {
        LocalBoneTransforms.Empty();
        LocalBoneTransformMap.Empty();
        //LocalBoneTransforms.AddDefaulted(NumBones); // 기본값(Identity)으로 초기화
    }

    void Init()
    {
        for (int32 i = 0; i < LocalBoneTransformMap.Num(); i++)
        {
            LocalBoneTransforms[i] = FCompactPoseBone();
        }
    }
};

namespace AnimationUtils
{
    inline void BlendPoses(TArray<FName> AnimationTrackNames, const FPoseData& PoseA, const FPoseData& PoseB, float BlendAlpha, FPoseData& OutBlendedPose)
    {
        // 포즈의 뼈 개수가 다르면 블렌딩 불가 (에러 처리 또는 경고)
        if (PoseA.LocalBoneTransforms.Num() != PoseB.LocalBoneTransforms.Num())
        {
            if (PoseA.LocalBoneTransforms.Num() > 0)
            {
                OutBlendedPose = PoseA; // A를 그대로 사용
            }
            else if (PoseB.LocalBoneTransforms.Num() > 0)
            {
                OutBlendedPose = PoseB; // B를 그대로 사용
            }

            return;
        }

        const int32 NumBones = PoseA.LocalBoneTransforms.Num();
        OutBlendedPose.Reset();
        OutBlendedPose.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);
        OutBlendedPose.Init();

        for (int32 i = 0; i < NumBones; ++i)
        {
            const FCompactPoseBone& TransformA = PoseA.LocalBoneTransforms[i];
            const FCompactPoseBone& TransformB = PoseB.LocalBoneTransforms[i];           
            FCompactPoseBone& BlendedTransform = OutBlendedPose.LocalBoneTransforms[i];

            BlendedTransform.Translation = FMath::Lerp(TransformA.Translation, TransformB.Translation, BlendAlpha);
            BlendedTransform.Rotation = FQuat::Slerp(TransformA.Rotation, TransformB.Rotation, BlendAlpha);
            BlendedTransform.Rotation.Normalize();
            BlendedTransform.Scale3D = FMath::Lerp(TransformA.Scale3D, TransformB.Scale3D, BlendAlpha);

            if (0 <= i && i < AnimationTrackNames.Num())
            {
                OutBlendedPose.LocalBoneTransformMap[AnimationTrackNames[i]] = BlendedTransform;
            }
        }
    }
}

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


