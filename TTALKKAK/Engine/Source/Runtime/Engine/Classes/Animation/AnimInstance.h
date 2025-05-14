#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "FBX/FBXDefine.h" // FIX-ME
#include "Classes/Engine/Assets/Animation/AnimTypes.h"
#include "AnimCondition.h"

class UAnimSequence;
class USkeletalMeshComponent;

class UAnimationStateMachine;

class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)

public:
    UAnimInstance() = default;
    UAnimInstance(const UAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    virtual void NativeInitializeAnimation();
    virtual void NativeUpdateAnimation(float DeltaSeconds);
    virtual void TriggerAnimNotifies(float PrevTime, float CurrTime);

    const FRefSkeletal* TargetSkeleton;
    virtual const FPoseData& GetCurrentPose() const;
    USkeletalMeshComponent* GetSkelMeshComponent() const;

    virtual void InitializeForBlendAnimations(const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose);
    virtual void BlendAnimations(float DeltaSeconds, const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose);

    // FIX-ME: Hard coded
    virtual void InitializeForAnimationStateMachine();

    PROPERTY(float, BlendAlpha);

    UAnimationStateMachine* GetStateMachine() const { return StateMachine; }
    FAnimationConditionContext& GetStateMachineContext() { return StateMachineContext; }
    const FAnimationConditionContext& GetStateMachineContext() const { return StateMachineContext; }

protected:
    FPoseData CurrentPoseData;
    
    TArray<UAnimSequence*> OwningAnimSequences; // FIX-ME: UAnimAsset
    TArray<FPoseData> OwningPoseData;

    float BlendAlpha;
    float NormalizedTime;

    UAnimationStateMachine* StateMachine;
    FAnimationConditionContext StateMachineContext;

    int32 CurrentMainAnimIndex;
    int32 CurrentPrevAnimIndexForBlend; // 블렌딩 시에만 유효 (INDEX_NONE 가능)

    // FIX-ME: Hard coded
    void SampleSingleAnimationByIndex(float DeltaSeconds, int32 AnimIndex, FPoseData& OutPose);
    void UpdateBlendedAnimations(float DeltaSeconds, int32 IndexA, int32 IndexB, float InBlendAlpha, FPoseData& OutPose);

};