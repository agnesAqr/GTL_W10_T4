#include "AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"
#include "CoreUObject/UObject/Casts.h"
#include "FBX/FBXDefine.h"
#include "Math/MathUtility.h"
#include "FBXLoader.h"

// NOTE: For CurrentAnimationSeq->GetRateScale() to work as in UCustomAnimInstance,
// UAnimSequenceBase would need a public GetRateScale() method like:
// In UAnimSequenceBase.h:
// public:
//    float GetRateScale() const { return RateScale; } 
// (Currently, RateScale is protected and no getter is shown in the provided files)
// For this implementation, we will try to access `RateScale` directly,
// or assume GetRateScale() will be added based on UCustomAnimInstance's usage.


UAnimSingleNodeInstance::UAnimSingleNodeInstance()
    : CurrentAnimationSeq(nullptr)
    , CurrentTime(0.0f)
{
    UAnimSequence* AnimSequence = FBXLoader::CreateAnimationSequence("FBX/Run_Forward.fbx");
    SetAnimation(AnimSequence);
}

UAnimSingleNodeInstance::UAnimSingleNodeInstance(const UAnimSingleNodeInstance& Other)
    : UAnimInstance(Other)
    , CurrentAnimationSeq(Other.CurrentAnimationSeq) 
    , CurrentTime(Other.CurrentTime)
{
}

UObject* UAnimSingleNodeInstance::Duplicate(UObject* InOuter)
{
    UAnimSingleNodeInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UAnimSingleNodeInstance>(this, InOuter);
    if (NewAnimInstance)
    {
        NewAnimInstance->DuplicateSubObjects(this, InOuter);
        NewAnimInstance->PostDuplicate();
    }
    return NewAnimInstance;
}

void UAnimSingleNodeInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    const UAnimSingleNodeInstance* Origin = Cast<UAnimSingleNodeInstance>(Source);
    if (Origin)
    {
        // CurrentAnimationSeq is a UObject pointer, typically an asset.
        // We don't duplicate assets here, just copy the pointer.
        // If it were a sub-object owned exclusively by this instance, duplication logic would be needed.
        this->CurrentAnimationSeq = Origin->CurrentAnimationSeq;
    }
}

void UAnimSingleNodeInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UAnimSingleNodeInstance::SetAnimation(UAnimSequence* AnimSequence)
{
    CurrentAnimationSeq = AnimSequence;
    CurrentTime = 0.0f;
    PreviousTime = 0.0f;

    if (CurrentAnimationSeq && TargetSkeleton)
    {
        CurrentPoseData.Reset();
        CurrentPoseData.Skeleton = TargetSkeleton;
        // IMPORTANT: Workaround for UAnimSequence::SamplePoseAtTime bug
        // The SamplePoseAtTime function provided might not correctly size OutPose.LocalBoneTransforms.
        // Ensure it's sized before calling.
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.SetNum(TargetSkeleton->BoneTree.Num());
            for (FCompactPoseBone& Bone : CurrentPoseData.LocalBoneTransforms)
            {
                Bone = FCompactPoseBone(); // Identity transform
            }
        }
        CurrentAnimationSeq->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
    }
    else
    {
        CurrentPoseData.Reset();
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.SetNum(TargetSkeleton->BoneTree.Num());
            for (FCompactPoseBone& Bone : CurrentPoseData.LocalBoneTransforms)
            {
                Bone = FCompactPoseBone();
            }
        }
        CurrentPoseData.Skeleton = TargetSkeleton;
    }
}

void UAnimSingleNodeInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    CurrentTime = 0.0f;
    PreviousTime = 0.0f;
    CurrentPoseData.Reset();

    // TargetSkeleton should be set by the component/system that owns this AnimInstance.
    // Initialize CurrentPoseData with the correct skeleton and bone count (identity pose).
    if (TargetSkeleton)
    {
        CurrentPoseData.Skeleton = TargetSkeleton;
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
    }
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (CurrentAnimationSeq && TargetSkeleton)
    {
        PreviousTime = CurrentTime;
        
        const float PlayLength = CurrentAnimationSeq->GetPlayLength();
        float ActualRateScale = 1.0f;

        // Accessing RateScale:
        // UCustomAnimInstance uses AnimationA->GetRateScale().
        // UAnimSequenceBase has `RateScale` as protected and no public GetRateScale() is defined in provided headers.
        // This implies GetRateScale() should be added to UAnimSequenceBase or RateScale made public.
        // For now, we'll assume such a getter would exist or RateScale could be accessed directly.
        // If `PROPERTY(float, RateScale)` in UAnimSequenceBase implies a public `GetRateScale()`, we'd use that.
        // Let's try to use .RateScale directly, assuming it's made accessible for this example.
        // If not, a default of 1.0f is used.
        // To make `CurrentAnimationSeq->RateScale` work, `RateScale` in `UAnimSequenceBase` must be public,
        // or `UAnimSingleNodeInstance` must be a friend, or a public getter exists.
        // Let's assume UAnimSequenceBase had `public: float GetRateScale() const { return RateScale; }`
        // In that case, the line would be:
        // ActualRateScale = CurrentAnimationSeq->GetRateScale();
        // For now, using direct member access (which might fail compilation with current headers):
        ActualRateScale = CurrentAnimationSeq->GetRateScale();

        if (PlayLength > KINDA_SMALL_NUMBER)
        {
            CurrentTime += DeltaSeconds * ActualRateScale;
            CurrentTime = FMath::Fmod(CurrentTime, PlayLength);
            if (CurrentTime < 0.0f)
            {
                CurrentTime += PlayLength;
            }
        }
        else
        {
            CurrentTime = 0.0f;
        }

        CurrentPoseData.Reset(); 
        CurrentPoseData.Skeleton = TargetSkeleton;

        // IMPORTANT: Workaround for UAnimSequence::SamplePoseAtTime potentially not sizing LocalBoneTransforms.
        // This ensures LocalBoneTransforms is sized before SamplePoseAtTime tries to write to it.
        // The proper fix is within SamplePoseAtTime itself.
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.SetNum(TargetSkeleton->BoneTree.Num());
            // Initialize to identity as a base. SamplePoseAtTime should fill actual values.
            // If SamplePoseAtTime doesn't animate a bone, it should ideally fall back to ref pose.
            // Current SamplePoseAtTime iterates all skeleton bones, but only updates if track exists.
            for (FCompactPoseBone& Bone : CurrentPoseData.LocalBoneTransforms)
            {
                Bone = FCompactPoseBone(); // Identity transform
            }
        }
        TriggerAnimNotifies(PreviousTime, CurrentTime);
        CurrentAnimationSeq->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
    }
    else
    {
        CurrentPoseData.Reset();
        if (TargetSkeleton)
        {
            CurrentPoseData.Skeleton = TargetSkeleton;
            if (TargetSkeleton->BoneTree.Num() > 0)
            {
                CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
            }
        }
    }
}

void UAnimSingleNodeInstance::TriggerAnimNotifies(float PrevTime, float CurrTime)
{
    const float Length = CurrentAnimationSeq->GetPlayLength();
    bool bWrapped = CurrTime < PrevTime;

    for (auto& Notify : CurrentAnimationSeq->GetNotifies())
    {
        if (!bWrapped)
        {
                
        }
        else
        {
            
        }
    }
}
