#include "AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"
#include "CoreUObject/UObject/Casts.h"
#include "FBX/FBXDefine.h"
#include "Math/MathUtility.h"
#include "FBXLoader.h"
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"

UAnimSingleNodeInstance::UAnimSingleNodeInstance()
    : CurrentTime(0.0f)
    , bLooping(true)
{
}

UAnimSingleNodeInstance::UAnimSingleNodeInstance(const UAnimSingleNodeInstance& Other)
    : CurrentTime(Other.CurrentTime)
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
        //this->AnimationSequence = Origin->AnimationSequence;
    }
}

void UAnimSingleNodeInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UAnimSingleNodeInstance::SetAnimation(UAnimSequence* AnimSequence, bool bShouldLoop)
{
    //AnimationSequence = AnimSequence;
    CurrentTime = 0.0f;
    PreviousTime = 0.0f;
    bLooping = bShouldLoop;

    if (OwningAnimSequences[0] && TargetSkeleton)
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
        OwningAnimSequences[0]->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
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

void UAnimSingleNodeInstance::SetPosition(float InTime, bool bFireNotifies /*= false*/)
{
    // ▶ PlayAsset이나 Skeleton이 없으면 아무 것도 안 함
    if (!OwningAnimSequences[0] || !TargetSkeleton)
    {
        return;
    }

    // ▶ 이전 시간 보관
    PreviousTime = CurrentTime;

    // ▶ 요청된 시간(InTime)으로 세팅하되, 루핑/클램프 처리
    const float PlayLength = OwningAnimSequences[0]->GetPlayLength();
    float NewTime = InTime;

    if (bLooping && PlayLength > KINDA_SMALL_NUMBER)
    {
        NewTime = FMath::Fmod(NewTime, PlayLength);
        if (NewTime < 0.0f)
        {
            NewTime += PlayLength;
        }
    }
    else
    {
        NewTime = FMath::Clamp(NewTime, 0.0f, PlayLength);
    }
    CurrentTime = NewTime;

    // ▶ 노티파이 트리거 (구간 Prev→Curr 사이 이벤트 발생)
    if (bFireNotifies)
    {
        TriggerAnimNotifies(PreviousTime, CurrentTime);
    }

    // ▶ 포즈 데이터 초기화
    CurrentPoseData.Reset();
    CurrentPoseData.Skeleton = TargetSkeleton;
    const int32 NumBones = TargetSkeleton->BoneTree.Num();
    CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);

    // ▶ 해당 시간의 포즈 샘플링
    OwningAnimSequences[0]->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
}

void UAnimSingleNodeInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CurrentTime = 0.0f;
    PreviousTime = 0.0f;

    OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Walking.fbx"));
    SetAnimation(OwningAnimSequences[0], true);
}


void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    OwningAnimSequences[0]->SetSkeletal(TargetSkeleton);
    if (OwningAnimSequences[0] && TargetSkeleton)
    {
        PreviousTime = CurrentTime;
        
        const float PlayLength = OwningAnimSequences[0]->GetPlayLength();
        float ActualRateScale = 1.0f;
        if (OwningAnimSequences[0]) ActualRateScale = OwningAnimSequences[0]->GetRateScale();

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
        TriggerAnimNotifies(PreviousTime, CurrentTime);
        OwningAnimSequences[0]->SamplePoseAtTime(CurrentTime, TargetSkeleton, CurrentPoseData);
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

void UAnimSingleNodeInstance::TriggerAnimNotifies(float PrevTime, float CurrTime)
{
    const float Length = OwningAnimSequences[0]->GetPlayLength();
    bool bWrapped = CurrTime < PrevTime;

    for (auto& Notify : OwningAnimSequences[0]->GetNotifies())
    {
        if (!bWrapped)
        {
                
        }
        else
        {
            
        }
    }
}
