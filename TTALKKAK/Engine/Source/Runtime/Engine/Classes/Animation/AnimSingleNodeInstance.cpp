#include "AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"
#include "CoreUObject/UObject/Casts.h"
#include "FBX/FBXDefine.h"
#include "Math/MathUtility.h"
#include "FBXLoader.h"
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"

UAnimSingleNodeInstance::UAnimSingleNodeInstance()
    : AnimationSequence(nullptr)
    , CurrentTime(0.0f)
    , bLooping(true)
{
    // 이거를 삭제하라고 하는디 아무리 생각해도 여기 맞긴 함
    AnimationSequence = FBXLoader::GetAnimationSequence("FBX/Sneak_Walking.fbx");
}

UAnimSingleNodeInstance::UAnimSingleNodeInstance(const UAnimSingleNodeInstance& Other)
    : UAnimInstance(Other)
    , AnimationSequence(Other.AnimationSequence) 
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
        this->AnimationSequence = Origin->AnimationSequence;
    }
}

void UAnimSingleNodeInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UAnimSingleNodeInstance::SetAnimation(UAnimSequence* AnimSequence, bool bShouldLoop)
{
    AnimationSequence = AnimSequence;
    CurrentTime = 0.0f;
    bLooping = bShouldLoop;

    if (AnimationSequence && TargetSkeleton)
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
        AnimationSequence->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
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

    if (TargetSkeleton)
    {
        CurrentPoseData.Skeleton = TargetSkeleton;
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
    }

    if (AnimationSequence && TargetSkeleton)
    {
        AnimationSequence->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
    }

    //SetAnimation(AnimationSequence, true);
}


void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    AnimationSequence->SetSkeletal(TargetSkeleton);
    if (AnimationSequence && TargetSkeleton)
    {
        const float PlayLength = AnimationSequence->GetPlayLength();
        float ActualRateScale = 1.0f;
        if (AnimationSequence) ActualRateScale = AnimationSequence->GetRateScale();

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
        AnimationSequence->SamplePoseAtTime(CurrentTime, TargetSkeleton, this->CurrentPoseData);
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