#include "CustomAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h" // AnimSequence
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"
#include "FBXLoader.h"
#include "AnimationStateMachine.h"

#include "LaunchEngineLoop.h"
#include "ImGuiManager.h"

UCustomAnimInstance::UCustomAnimInstance()
{
}

UCustomAnimInstance::UCustomAnimInstance(const UCustomAnimInstance& Other)
{
}

UObject* UCustomAnimInstance::Duplicate(UObject* InOuter)
{
    UCustomAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UCustomAnimInstance>(this, InOuter);
    if (NewAnimInstance)
    {
        NewAnimInstance->DuplicateSubObjects(this, InOuter);
        NewAnimInstance->PostDuplicate();
    }
    return NewAnimInstance;
}

void UCustomAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    const UCustomAnimInstance* Origin = Cast<UCustomAnimInstance>(Source);
}

void UCustomAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UCustomAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    // FIX-ME: Hard coded
    OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Idle.fbx"));
    OwningAnimSequences.Add(FBXLoader::GetAnimationSequence("FBX/Walking.fbx"));

    InitializeForAnimationStateMachine();

    // 입력 델리게이트 등록
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();
    if (Handler)
    {
        // 기존 핸들 제거 (중복 방지)
        for (const FDelegateHandle& Handle : InputDelegateHandles)
        {
            Handler->OnKeyDownDelegate.Remove(Handle);
            // 필요시 다른 델리게이트에서도 제거
        }
        InputDelegateHandles.Empty();

        // Spacebar 키 눌림 이벤트에 대한 델리게이트 등록
        InputDelegateHandles.Add(
            Handler->OnKeyDownDelegate.AddUObject(this, &UCustomAnimInstance::HandleSpacebarPressed)
        );
        UE_LOG(LogLevel::Error, TEXT("UCustomAnimInstance: Spacebar input delegate registered."));
    }
    else
    {
        UE_LOG(LogLevel::Error, TEXT("UCustomAnimInstance: FSlateAppMessageHandler is null. Cannot register input delegates."));
    }
}

void UCustomAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // Spacebar를 누르면 스테이트 머신 컨텍스트를 업데이트 하는 테스트 코드 작성

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

void UCustomAnimInstance::SampleSingleAnimationByIndex(float DeltaSeconds, int32 AnimIndex, FPoseData& OutPose)
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
            if (NormalizedTime >= 1.0f && !bLoop) { /* Trigger Animation End Event */ }
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

void UCustomAnimInstance::UpdateBlendedAnimations(float DeltaSeconds, int32 IndexA, int32 IndexB, float InBlendAlpha, FPoseData& OutPose)
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

void UCustomAnimInstance::InitializeForAnimationStateMachine()
{
    if (!StateMachine)
    {
        StateMachine = FObjectFactory::ConstructObject<UAnimationStateMachine>(this);
    }

    StateMachine->AddState("Idle", 0);
    StateMachine->AddState("Walking", 1);

    Super::InitializeForAnimationStateMachine();

    if (StateMachine)
    {
        // FIX-ME: Debug Code
        StateMachine->AddTransition(FName("Idle"), FName("Walking"),
            [](const FAnimationConditionContext& Context) {
                bool bIsMoving = Context.GetBool(FName("IsMoving"));
                UE_LOG(LogLevel::Display, TEXT("Idle->Walking Check: IsMoving = %s"), bIsMoving ? TEXT("true") : TEXT("false"));
                return bIsMoving;
            });

        StateMachine->AddTransition(FName("Walking"), FName("Idle"),
            [](const FAnimationConditionContext& Context) {
                bool bIsMoving = Context.GetBool(FName("IsMoving"));
                UE_LOG(LogLevel::Display, TEXT("Walking->Idle Check: IsMoving = %s"), bIsMoving ? TEXT("true") : TEXT("false"));
                return !bIsMoving;
            });

        StateMachine->SetInitialState(FName("Idle"));
    }

}

void UCustomAnimInstance::HandleSpacebarPressed(const FKeyEvent& InKeyEvent, HWND AppWnd)
{
    if (ImGuiManager::Get().GetWantCaptureKeyboard(AppWnd))
    {
        return;
    }
    if (InKeyEvent.GetKeyCode() == VK_SPACE)
    {
        if (InKeyEvent.GetInputEvent() == IE_Pressed)
        {
            bool bCurrentIsMoving = StateMachineContext.GetBool(FName("IsMoving"));
            StateMachineContext.SetBool(FName("IsMoving"), !bCurrentIsMoving);
            UE_LOG(LogLevel::Display, TEXT("Spacebar Pressed: StateMachineContext 'IsMoving' toggled to: %s"), !bCurrentIsMoving ? TEXT("true") : TEXT("false"));
            NormalizedTime = 0.0f;
        }
    }
}
