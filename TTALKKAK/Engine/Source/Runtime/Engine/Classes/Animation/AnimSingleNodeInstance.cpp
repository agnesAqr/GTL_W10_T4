#include "AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"
#include "CoreUObject/UObject/Casts.h"
#include "FBX/FBXDefine.h"
#include "Math/MathUtility.h"
#include "FBXLoader.h"
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"

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
    , bLooping(true)
{
}

UAnimSingleNodeInstance::UAnimSingleNodeInstance(const UAnimSingleNodeInstance& Other)
    : UAnimInstance(Other)
    , CurrentAnimationSeq(Other.CurrentAnimationSeq) 
    , CurrentTime(Other.CurrentTime)
    , bLooping(Other.bLooping)
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

void UAnimSingleNodeInstance::SetAnimation(UAnimSequence* AnimSequence, bool bShouldLoop)
{
    CurrentAnimationSeq = AnimSequence;
    CurrentTime = 0.0f;
    bLooping = bShouldLoop;

    if (CurrentAnimationSeq && TargetSkeleton)
    {
        CurrentPoseData.Reset();
        CurrentPoseData.Skeleton = TargetSkeleton;
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
    CurrentPoseData.Reset();

    if (TargetSkeleton)
    {
        CurrentPoseData.Skeleton = TargetSkeleton;
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
    }

    UAnimSequence* AnimSequence = FBXLoader::CreateAnimationSequence("FBX/Capoeira.fbx");
    //AnimSequence->GetPlayLength();// 이거 왜 0으로 뽑히지
    SetAnimation(AnimSequence, true);
}


void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    CurrentAnimationSeq->SetSkeletal(TargetSkeleton);
    if (CurrentAnimationSeq && TargetSkeleton)
    {
        const float PlayLength = CurrentAnimationSeq->GetPlayLength(); // Begin Test
        //const float PlayLength = CurrentAnimationSeq->GetAnimDataModel()->PlayLength; // 이걸 이런식으로 접근해야 하는거면 다른 것들은 왜 있는거지
        float ActualRateScale = 1.0f;
        if (CurrentAnimationSeq) ActualRateScale = CurrentAnimationSeq->GetRateScale();

        UE_LOG(LogLevel::Display, "UAnimSingleNodeInstance-ActualRateScale: %f", ActualRateScale);
        UE_LOG(LogLevel::Display, "UAnimSingleNodeInstance-PlayLength: %f", PlayLength);

        if (PlayLength > KINDA_SMALL_NUMBER)
        {
            CurrentTime += DeltaSeconds * ActualRateScale;
            CurrentTime = FMath::Fmod(CurrentTime, PlayLength);
            if (CurrentTime < 0.0f) CurrentTime += PlayLength;
        }
        else
        {
            CurrentTime = 0.0f;
        }

        this->CurrentPoseData.Reset();
        this->CurrentPoseData.Skeleton = TargetSkeleton;
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            this->CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
        //UE_LOG(LogLevel::Display, "UAnimSingleNodeInstance-CurrentTime: %f", CurrentTime);
        CurrentAnimationSeq->SamplePoseAtTime(CurrentTime, TargetSkeleton, this->CurrentPoseData);
    }
    else
    {
        // 애니메이션이나 스켈레톤 없으면 CurrentPoseData를 참조 포즈 또는 항등 포즈로 리셋
        this->CurrentPoseData.Reset();
        if (TargetSkeleton)
        {
            this->CurrentPoseData.Skeleton = TargetSkeleton;
            if (TargetSkeleton->BoneTree.Num() > 0)
            {
                this->CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
            }
        }
    }
}