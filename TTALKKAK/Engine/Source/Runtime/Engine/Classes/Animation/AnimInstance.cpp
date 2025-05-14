#include "AnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "FBXLoader.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"

#include "AnimationStateMachine.h"

UAnimInstance::UAnimInstance(const UAnimInstance& Other)
    : UObject(Other)
    , TargetSkeleton(Other.TargetSkeleton)
    , CurrentPoseData(Other.CurrentPoseData)
    , BlendAlpha(Other.BlendAlpha)
    , StateMachine(nullptr)
    , StateMachineContext(Other.StateMachineContext)
{
}

UObject* UAnimInstance::Duplicate(UObject* InOuter)
{
    UAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UAnimInstance>(this, InOuter);
    NewAnimInstance->DuplicateSubObjects(this, InOuter);
    NewAnimInstance->PostDuplicate();
    return NewAnimInstance;
}

void UAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);

    const UAnimInstance* Origin = Cast<const UAnimInstance>(Source);

    if (Origin && Origin->StateMachine)
    {
        this->StateMachine = Cast<UAnimationStateMachine>(Origin->StateMachine->Duplicate(InOuter));
        if (this->StateMachine)
        {
            // this->StateMachine->SetOuter(this); // 새 AnimInstance를 Outer로 설정 (필요시)
        }
    }
    else
    {
        this->StateMachine = nullptr;
    }

    if (Origin)
    {
        this->OwningAnimSequences = Origin->OwningAnimSequences;
    }
}

void UAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UAnimInstance::NativeInitializeAnimation()
{
    USkeletalMeshComponent* SkelComp = GetSkelMeshComponent();
    TargetSkeleton = SkelComp->GetSkeletalMesh()->GetRefSkeletal();

    const int32 NumBones = TargetSkeleton->BoneTree.Num();
    for (auto PoseData : OwningPoseData)
    {
        PoseData.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);
    }
    CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);
    //OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/mixmix2_2.fbx"));
    //SetAnimation
    OwningAnimSequences.Empty();

}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    if (!StateMachine || !TargetSkeleton)
    {
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
        return;
    }
}

const FPoseData& UAnimInstance::GetCurrentPose() const
{
    return CurrentPoseData;
}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const
{
    return Cast<USkeletalMeshComponent>(GetOuter());
}

void UAnimInstance::InitializeForBlendAnimations(const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose)
{
    if (TargetSkeleton)
    {
        CurrentPoseData.Skeleton = TargetSkeleton;
        if (TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
    }

    if (OwningAnimSequences[IndexA] && OwningAnimSequences[IndexB] && TargetSkeleton)
    {
        // FIX-ME
        float SampleTimeA = NormalizedTime * OwningAnimSequences[IndexA]->GetPlayLength();
        float SampleTimeB = NormalizedTime * OwningAnimSequences[IndexB]->GetPlayLength();
        FPoseData PoseA;
        FPoseData PoseB;
        OwningAnimSequences[IndexA]->SamplePoseAtTime(SampleTimeA, TargetSkeleton, PoseA);
        OwningPoseData.Add(PoseA);
        OwningAnimSequences[IndexB]->SamplePoseAtTime(SampleTimeB, TargetSkeleton, PoseB);
        OwningPoseData.Add(PoseB);

        AnimationUtils::BlendPoses(OwningPoseData[IndexA], OwningPoseData[IndexB], BlendAlpha, CurrentPoseData);
    }
}

void UAnimInstance::BlendAnimations(float DeltaSeconds, const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose)
{
    if (!OwningAnimSequences[IndexA] || !OwningAnimSequences[IndexB] || !TargetSkeleton || TargetSkeleton->BoneTree.Num() == 0)
    {
        return;
    }

    float PlayLengthA = OwningAnimSequences[IndexA]->GetPlayLength();
    float PlayLengthB = OwningAnimSequences[IndexB]->GetPlayLength();

    float RateScaleA = OwningAnimSequences[IndexA]->GetRateScale();
    float RateScaleB = OwningAnimSequences[IndexB]->GetRateScale();

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
    OwningPoseData[IndexA].LocalBoneTransforms.Empty();
    OwningPoseData[IndexB].LocalBoneTransforms.Empty();

    OwningPoseData[IndexA].LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);
    OwningPoseData[IndexB].LocalBoneTransforms.Init(FCompactPoseBone(), NumBones);

    OwningAnimSequences[IndexA]->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, OwningPoseData[IndexA]);
    OwningAnimSequences[IndexB]->SamplePoseAtTime(CurrentTimeB, TargetSkeleton, OwningPoseData[IndexB]);

    if (OwningPoseData[IndexA].LocalBoneTransforms.Num() == 0 && OwningPoseData[IndexB].LocalBoneTransforms.Num() > 0)
    {
        CurrentPoseData = OwningPoseData[IndexB]; // B만 유효하면 B 사용
    }
    else if (OwningPoseData[IndexB].LocalBoneTransforms.Num() == 0 && OwningPoseData[IndexA].LocalBoneTransforms.Num() > 0)
    {
        CurrentPoseData = OwningPoseData[IndexA]; // A만 유효하면 A 사용
    }
    else if (OwningPoseData[IndexA].LocalBoneTransforms.Num() > 0 && OwningPoseData[IndexB].LocalBoneTransforms.Num() > 0)
    {
        AnimationUtils::BlendPoses(OwningPoseData[IndexA], OwningPoseData[IndexB], BlendAlpha, CurrentPoseData);
    }
    else
    {
        CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), NumBones); // 예: 바인드 포즈로 초기화
    }
}

void UAnimInstance::InitializeForAnimationStateMachine()
{
    NormalizedTime = 0.0f;
    CurrentMainAnimIndex = INDEX_NONE;
    CurrentPrevAnimIndexForBlend = INDEX_NONE;
    if (StateMachine && StateMachine->GetCurrentState())
    {
        CurrentMainAnimIndex = StateMachine->GetCurrentState()->OwningAnimIndex;
    }
}

void UAnimInstance::TriggerAnimNotifies(float PrevTime, float CurrTime)
{
}
