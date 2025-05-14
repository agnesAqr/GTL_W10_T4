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
    OwningAnimSequences.Empty();

    InitializeForAnimationStateMachine();
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

    // 1. 스테이트 머신 컨텍스트 업데이트 (게임 로직과 연동)
    // 이 부분은 실제 게임의 캐릭터 상태에 따라 채워져야 합니다.
    // 예시:
    // USkeletalMeshComponent* SkelComp = GetSkelMeshComponent();
    // AActor* OwnerActor = SkelComp ? SkelComp->GetOwner() : nullptr;
    // if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
    // {
    //     float Speed = Character->GetVelocity().Size();
    //     StateMachineContext.SetFloat(FName("Speed"), Speed);
    //     StateMachineContext.SetBool(FName("IsMoving"), Speed > KINDA_SMALL_NUMBER);
    //     StateMachineContext.SetBool(FName("IsFalling"), Character->GetCharacterMovement()->IsFalling());
    //     // ... 기타 필요한 변수들 설정 ...
    // }

    StateMachine->Tick(DeltaSeconds, StateMachineContext);

    CurrentMainAnimIndex = StateMachine->GetOutputAnimIndex(CurrentPrevAnimIndexForBlend);
    BlendAlpha = StateMachine->GetBlendAlpha();

    if (OwningAnimSequences.IsValidIndex(CurrentMainAnimIndex))
    {
        if (StateMachine->IsBlending() && OwningAnimSequences.IsValidIndex(CurrentPrevAnimIndexForBlend))
        {
            UpdateBlendedAnimations(DeltaSeconds, CurrentPrevAnimIndexForBlend, CurrentMainAnimIndex, BlendAlpha, CurrentPoseData);
        }
        else
        {
            SampleSingleAnimationByIndex(DeltaSeconds, CurrentMainAnimIndex, CurrentPoseData);
        }
    }
    else
    {
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            CurrentPoseData.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
        NormalizedTime = 0.0f;
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
    if (!StateMachine)
    {
        StateMachine = FObjectFactory::ConstructObject<UAnimationStateMachine>(this);
    }

    if (StateMachine)
    {
        // FIX-ME: Debug Code
        OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Idle.fbx"));
        OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Walking.fbx"));

        FAnimationState* IdleState = StateMachine->AddState(FName("Idle") /*, IdleAnimSequenceAsset */);
        FAnimationState* WalkState = StateMachine->AddState(FName("Walking") /*, WalkAnimSequenceAsset */);

        StateMachine->AddTransition(FName("Idle"), FName("Walk"),
            [](const FAnimationConditionContext& Context) { return Context.GetBool(FName("IsMoving")); });
        StateMachine->AddTransition(FName("Walk"), FName("Idle"),
            [](const FAnimationConditionContext& Context) { return !Context.GetBool(FName("IsMoving")); });

        StateMachine->SetInitialState(FName("Idle"));
    }

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

void UAnimInstance::SampleSingleAnimationByIndex(float DeltaSeconds, int32 AnimIndex, FPoseData& OutPose)
{
    if (!OwningAnimSequences.IsValidIndex(AnimIndex) || !TargetSkeleton)
    {
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num()); // 바인드 포즈
            OutPose.Skeleton = TargetSkeleton;
        }
        NormalizedTime = 0.0f;
        return;
    }

    UAnimSequence* Seq = OwningAnimSequences[AnimIndex];
    if (!Seq)
    {
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num()); // 바인드 포즈
            OutPose.Skeleton = TargetSkeleton;
        }
        NormalizedTime = 0.0f;
        return;
    }


    float PlayLength = Seq->GetPlayLength();
    float RateScale = 1.0f;
    bool bLoop = true;

    const FAnimationState* FsmState = nullptr;
    if (StateMachine)
    {
        if (StateMachine->IsBlending())
        {
            const FAnimationState* TargetFsmState = StateMachine->GetState(StateMachine->GetTransitionTargetStateName()); 
            const FAnimationState* CurrentActiveFsmState = StateMachine->GetCurrentState();

            for (const auto& Pair : StateMachine->GetAllStates())
            {
                if (Pair.Value.OwningAnimIndex == AnimIndex)
                {
                    FsmState = &Pair.Value;
                    break;
                }
            }

        }
        else
        {
            FsmState = StateMachine->GetCurrentState();
        }
    }


    if (FsmState)
    {
        RateScale = FsmState->PlayRate;
        bLoop = FsmState->bLooping;
    }

    if (PlayLength > MINIMUM_ANIMATION_LENGTH && RateScale > KINDA_SMALL_NUMBER)
    {
        NormalizedTime += (DeltaSeconds * RateScale) / PlayLength;
        if (bLoop)
        {
            NormalizedTime = FMath::Fmod(NormalizedTime, 1.0f);
        }
        else
        {
            NormalizedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
            // 루프 안 하면 마지막 프레임에서 멈춤.
            // 애니메이션 종료 이벤트 발생 로직 등을 여기에 추가 가능.
            // if (NormalizedTime >= 1.0f && !bLoop) { /* Trigger Animation End Event */ }
        }
    }
    else if (PlayLength > MINIMUM_ANIMATION_LENGTH)
    {
        if (!bLoop && NormalizedTime >= 1.0f) NormalizedTime = 1.0f;
        else if (!bLoop && NormalizedTime <= 0.0f) NormalizedTime = 0.0f;
    }
    else
    {
        NormalizedTime = 0.0f;
    }

    if (TargetSkeleton->BoneTree.Num() > 0)
    {
        OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
    }
    OutPose.Skeleton = TargetSkeleton;
    Seq->SamplePoseAtTime(NormalizedTime * PlayLength, TargetSkeleton, OutPose);
}

void UAnimInstance::UpdateBlendedAnimations(float DeltaSeconds, int32 IndexA, int32 IndexB, float InBlendAlpha, FPoseData& OutPose)
{
    if (!OwningAnimSequences.IsValidIndex(IndexA) || !OwningAnimSequences.IsValidIndex(IndexB) ||
        !OwningAnimSequences[IndexA] || !OwningAnimSequences[IndexB] ||
        !TargetSkeleton || TargetSkeleton->BoneTree.Num() == 0)
    {
        if (OwningAnimSequences.IsValidIndex(IndexB)) {
            SampleSingleAnimationByIndex(DeltaSeconds, IndexB, OutPose);
        }
        else if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0) {
            OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        }
        return;
    }

    UAnimSequence* SeqA = OwningAnimSequences[IndexA];
    UAnimSequence* SeqB = OwningAnimSequences[IndexB];

    float PlayLengthA = SeqA->GetPlayLength();
    float PlayLengthB = SeqB->GetPlayLength();

    float RateScaleA = 1.0f;
    bool  bLoopA = true;
    float RateScaleB = 1.0f;
    bool  bLoopB = true;

    // 각 애니메이션에 해당하는 FSM 상태를 찾아 재생 속도/루프 설정 가져오기
    if (StateMachine)
    {
        for (const auto& Pair : StateMachine->GetAllStates())
        {
            if (Pair.Value.OwningAnimIndex == IndexA)
            {
                RateScaleA = Pair.Value.PlayRate;
                bLoopA = Pair.Value.bLooping;
            }
            if (Pair.Value.OwningAnimIndex == IndexB)
            {
                RateScaleB = Pair.Value.PlayRate;
                bLoopB = Pair.Value.bLooping;
            }
        }
    }


    // 두 애니메이션 중 하나라도 유효한 길이가 있어야 함
    if (PlayLengthA <= MINIMUM_ANIMATION_LENGTH && PlayLengthB <= MINIMUM_ANIMATION_LENGTH)
    {
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num()); // 바인드 포즈
        }
        return;
    }

    float SafePlayLengthA = FMath::Max(PlayLengthA, MINIMUM_ANIMATION_LENGTH);
    float SafePlayLengthB = FMath::Max(PlayLengthB, MINIMUM_ANIMATION_LENGTH);

    float EffectivePlayLength = FMath::Lerp(SafePlayLengthA, SafePlayLengthB, InBlendAlpha);
    float EffectiveRateScale = FMath::Lerp(RateScaleA, RateScaleB, InBlendAlpha);
    bool EffectiveLoop = (InBlendAlpha < 0.5f) ? bLoopA : bLoopB; 
    EffectiveLoop = bLoopB;

    if (EffectivePlayLength > MINIMUM_ANIMATION_LENGTH && EffectiveRateScale > KINDA_SMALL_NUMBER)
    {
        NormalizedTime += (DeltaSeconds * EffectiveRateScale) / EffectivePlayLength;
        if (EffectiveLoop)
        {
            NormalizedTime = FMath::Fmod(NormalizedTime, 1.0f);
        }
        else
        {
            NormalizedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
        }
    }

    FPoseData PoseA, PoseB;
    if (TargetSkeleton->BoneTree.Num() > 0)
    {
        PoseA.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
        PoseB.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num());
    }
    PoseA.Skeleton = TargetSkeleton;
    PoseB.Skeleton = TargetSkeleton;

    SeqA->SamplePoseAtTime(NormalizedTime * PlayLengthA, TargetSkeleton, PoseA);
    SeqB->SamplePoseAtTime(NormalizedTime * PlayLengthB, TargetSkeleton, PoseB);


    if (PoseA.LocalBoneTransforms.Num() == 0 && PoseB.LocalBoneTransforms.Num() > 0)
    {
        OutPose = PoseB; // B만 유효하면 B 사용
    }
    else if (PoseB.LocalBoneTransforms.Num() == 0 && PoseA.LocalBoneTransforms.Num() > 0)
    {
        OutPose = PoseA; // A만 유효하면 A 사용
    }
    else if (PoseA.LocalBoneTransforms.Num() > 0 && PoseB.LocalBoneTransforms.Num() > 0)
    {
        AnimationUtils::BlendPoses(PoseA, PoseB, InBlendAlpha, OutPose);
    }
    else // 둘 다 샘플링 실패
    {
        if (TargetSkeleton && TargetSkeleton->BoneTree.Num() > 0)
        {
            OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), TargetSkeleton->BoneTree.Num()); // 바인드 포즈
        }
    }
}