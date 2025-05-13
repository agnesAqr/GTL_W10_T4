#include "BlendAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h" // AnimSequence
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"
#include "FBXLoader.h"

UBlendAnimInstance::UBlendAnimInstance()
    : AnimationA(nullptr)
    , AnimationB(nullptr)
    , BlendAlpha(0.8f)
    , NormalizedTime(0.f)
{
    AnimationA = FBXLoader::GetAnimationSequence("FBX/Walking.fbx");
    AnimationB = FBXLoader::GetAnimationSequence("FBX/Sneak_Walking.fbx");
}

UBlendAnimInstance::UBlendAnimInstance(const UBlendAnimInstance& Other)
    : AnimationA(Other.AnimationA)
    , AnimationB(Other.AnimationB)
    , BlendAlpha(Other.BlendAlpha)
    , NormalizedTime(Other.NormalizedTime)
{
}

UObject* UBlendAnimInstance::Duplicate(UObject* InOuter)
{
    UBlendAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UBlendAnimInstance>(this, InOuter);
    if (NewAnimInstance)
    {
        NewAnimInstance->DuplicateSubObjects(this, InOuter);
        NewAnimInstance->PostDuplicate();
    }
    return NewAnimInstance;
}

void UBlendAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    const UBlendAnimInstance* Origin = Cast<UBlendAnimInstance>(Source);
    if (Origin)
    {
        this->AnimationA = Origin->AnimationA;
        this->AnimationB = Origin->AnimationB;
    }
}

void UBlendAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UBlendAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    NormalizedTime = 0.0f;

    PoseDataA.Reset();
    PoseDataB.Reset();

    if (TargetSkeleton)
    {
        CurrentPoseData.Skeleton = TargetSkeleton;
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
    }

    if (AnimationA&&AnimationB && TargetSkeleton)
    {

        float SampleTimeA = NormalizedTime * AnimationA->GetPlayLength();
        float SampleTimeB = NormalizedTime * AnimationB->GetPlayLength();

        AnimationA->SamplePoseAtTime(SampleTimeA, TargetSkeleton, PoseDataA);
        AnimationB->SamplePoseAtTime(SampleTimeB, TargetSkeleton, PoseDataB);

        AnimationUtils::BlendPoses(PoseDataA, PoseDataB, BlendAlpha, CurrentPoseData);
    }

}

void UBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!AnimationA || !AnimationB || !TargetSkeleton || TargetSkeleton->BoneTree.Num() == 0)
    {
        return;
    }


    float PlayLengthA = AnimationA->GetPlayLength();
    float PlayLengthB = AnimationB->GetPlayLength();
    float RateScaleA = AnimationA->GetRateScale();
    float RateScaleB = AnimationB->GetRateScale();

    // 두 애니메이션 중 하나라도 유효한 길이가 있어야 함
    if (PlayLengthA <= MINIMUM_ANIMATION_LENGTH && PlayLengthB <= MINIMUM_ANIMATION_LENGTH)
    {
        return;
    }

    float SafePlayLengthA = FMath::Max(PlayLengthA, MINIMUM_ANIMATION_LENGTH);
    float SafePlayLengthB = FMath::Max(PlayLengthB, MINIMUM_ANIMATION_LENGTH);

    float EffectivePlayLength = FMath::Lerp(SafePlayLengthA, SafePlayLengthB, BlendAlpha);
    float EffectiveRateScale = FMath::Lerp(RateScaleA, RateScaleB, BlendAlpha);

    if (EffectivePlayLength > MINIMUM_ANIMATION_LENGTH && EffectiveRateScale > KINDA_SMALL_NUMBER)
    {
        NormalizedTime += (DeltaSeconds * EffectiveRateScale) / EffectivePlayLength;
        NormalizedTime = FMath::Fmod(NormalizedTime, 1.0f);
    }
    else if (PlayLengthA > MINIMUM_ANIMATION_LENGTH && RateScaleA > KINDA_SMALL_NUMBER) // B가 문제일 때 A 기준
    {
        NormalizedTime += (DeltaSeconds * RateScaleA) / PlayLengthA;
        NormalizedTime = FMath::Fmod(NormalizedTime, 1.0f);
    }
    else if (PlayLengthB > MINIMUM_ANIMATION_LENGTH && RateScaleB > KINDA_SMALL_NUMBER) // A가 문제일 때 B 기준
    {
        NormalizedTime += (DeltaSeconds * RateScaleB) / PlayLengthB;
        NormalizedTime = FMath::Fmod(NormalizedTime, 1.0f);
    }

    float CurrentTimeA = NormalizedTime * PlayLengthA;
    float CurrentTimeB = NormalizedTime * PlayLengthB;

    const int32 NumBones = TargetSkeleton->BoneTree.Num();
    PoseDataA.LocalBoneTransforms.Empty();
    PoseDataB.LocalBoneTransforms.Empty();

    PoseDataA.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);
    PoseDataB.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);

    AnimationA->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, PoseDataA);
    AnimationB->SamplePoseAtTime(CurrentTimeB, TargetSkeleton, PoseDataB);

    if (PoseDataA.LocalBoneTransforms.Num() == 0 && PoseDataB.LocalBoneTransforms.Num() > 0)
    {
        CurrentPoseData = PoseDataB; // B만 유효하면 B 사용
    }
    else if (PoseDataB.LocalBoneTransforms.Num() == 0 && PoseDataA.LocalBoneTransforms.Num() > 0)
    {
        CurrentPoseData = PoseDataA; // A만 유효하면 A 사용
    }
    else if (PoseDataA.LocalBoneTransforms.Num() > 0 && PoseDataB.LocalBoneTransforms.Num() > 0)
    {
        AnimationUtils::BlendPoses(PoseDataA, PoseDataB, BlendAlpha, CurrentPoseData);
    }
    else
    {
        CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones); // 예: 바인드 포즈로 초기화
    }
}