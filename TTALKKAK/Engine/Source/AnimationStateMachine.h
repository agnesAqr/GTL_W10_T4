// Animation/AnimationStateMachine.h (제공된 코드 기반 확장)
#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Core/Container/Map.h"

#include "AnimCondition.h"

class UAnimAssetBase; // 전방 선언

class UAnimationStateMachine : public UObject
{
    DECLARE_CLASS(UAnimationStateMachine, UObject)

public:
    UAnimationStateMachine();
    UAnimationStateMachine(const UAnimationStateMachine& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    void RemoveState(FName StateName);

    bool AddTransition(FName FromStateName, FName ToStateName, FTransitionPredicate TransitionRule, float BlendDuration = 0.2f, int32 Priority = 0);

    void Tick(float DeltaTime, FAnimationConditionContext& Context);


    PROPERTY(FName, TransitionTargetStateName);

private:
    TMap<FName, FAnimationState> States;

    FName CurrentStateName;
    FName TransitionTargetStateName;
    FName BlendingFromStateName;

    bool bIsBlending;
    float CurrentBlendTime;
    float TotalBlendDuration;

    void EnterState(FName StateName, FAnimationConditionContext& Context);
    void ExitState(FName StateName, FAnimationConditionContext& Context);
    void UpdateState(FName StateName, float DeltaTime, FAnimationConditionContext& Context);

    void CopyStatesAndTransitions(const UAnimationStateMachine& Source);

public:
    FAnimationState* AddState(FName StateName, int32 AnimIndexInOwner = INDEX_NONE, float PlayRate = 1.0f, bool bLoop = true);
    FAnimationState* GetState(FName StateName);
    const FAnimationState* GetState(FName StateName) const;
    int32 GetOutputAnimIndex(int32& InOwningAnimIndex) const;
    const TMap<FName, FAnimationState>& GetAllStates() const { return States; }

    void SetInitialState(FName StateName);
    FName GetCurrentStateName() const { return CurrentStateName; }
    const FAnimationState* GetCurrentState() const;
    bool IsBlending() const { return bIsBlending; }
    float GetBlendAlpha() const;

};