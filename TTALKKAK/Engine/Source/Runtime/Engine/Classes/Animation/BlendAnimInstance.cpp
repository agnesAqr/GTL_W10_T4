#include "BlendAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h" // AnimSequence
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"
#include "FBXLoader.h"

UBlendAnimInstance::UBlendAnimInstance()
{
}

UBlendAnimInstance::UBlendAnimInstance(const UBlendAnimInstance& Other)
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
}

void UBlendAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UBlendAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Walking.fbx"));
    OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Sneak_Walking.fbx"));

    SetBlendAlpha(0.7f);
    InitializeForBlendAnimations(0, 1, CurrentPoseData);
}

void UBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    BlendAnimations(DeltaSeconds, 0, 1, CurrentPoseData);
}

void UBlendAnimInstance::InitializeForBlendAnimations(const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose)
{
    Super::InitializeForBlendAnimations(IndexA, IndexB, OutPose);
}

void UBlendAnimInstance::BlendAnimations(float DeltaSeconds, const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose)
{
    Super::BlendAnimations(DeltaSeconds, IndexA, IndexB, OutPose);
}
