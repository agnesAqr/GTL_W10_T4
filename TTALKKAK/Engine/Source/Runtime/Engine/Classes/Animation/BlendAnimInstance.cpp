#include "BlendAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h" // AnimSequence
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"
#include "FBXLoader.h"

UBlendAnimInstance::UBlendAnimInstance()
    : AnimationA(nullptr)
    , AnimationB(nullptr)
    , BlendAlpha(1.f)
    , CurrentTimeA(0.f)
    , CurrentTimeB(0.f)
{
    AnimationA = FBXLoader::GetAnimationSequence("FBX/Walking.fbx");
    AnimationB = FBXLoader::GetAnimationSequence("FBX/Sneak_Walking.fbx");
}

UBlendAnimInstance::UBlendAnimInstance(const UBlendAnimInstance& Other)
    : AnimationA(Other.AnimationA)
    , AnimationB(Other.AnimationB)
    , BlendAlpha(Other.BlendAlpha)
    , CurrentTimeA(Other.CurrentTimeA)
    , CurrentTimeB(Other.CurrentTimeB)
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

    CurrentTimeA = 0.0f;
    CurrentTimeB = 0.0f;

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
        AnimationA->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, PoseDataA);
        AnimationB->SamplePoseAtTime(CurrentTimeB, TargetSkeleton, PoseDataB);
    }

    SetBlendAlpha(0.5f);
}

void UBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    AnimationA->SetSkeletal(TargetSkeleton);
    AnimationB->SetSkeletal(TargetSkeleton);

    if (AnimationA && AnimationB && TargetSkeleton)
    {
        if (AnimationA->GetPlayLength() > 0.f)
        {
            CurrentTimeA += DeltaSeconds * AnimationA->GetRateScale();
            CurrentTimeA = FMath::Fmod(CurrentTimeA, AnimationA->GetPlayLength());
        }
        if (AnimationB->GetPlayLength() > 0.f)
        {
            CurrentTimeB += DeltaSeconds * AnimationB->GetRateScale();
            CurrentTimeB = FMath::Fmod(CurrentTimeB, AnimationB->GetPlayLength());
        }

        // TargetSkeleton에서 뼈 개수를 가져와서 Reset 호출해야하는데 일단 지금 본 갯수 빼고 했음. 문제 생기면 이게 문제일 확률이 매우 높음.
        PoseDataA.Reset();
        PoseDataB.Reset();

        // FIX-ME
        AnimationA->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, PoseDataA);
        AnimationB->SamplePoseAtTime(CurrentTimeB, TargetSkeleton, PoseDataB);

        AnimationUtils::BlendPoses(PoseDataA, PoseDataB, BlendAlpha, CurrentPoseData);\
    }
}
