#pragma once

#include "Core/Container/Map.h"
#include <functional>
#include "CoreUObject/UObject/NameTypes.h" 
#include "Core/Container/Array.h" 

class UAnimAssetBase;

class FAnimationConditionContext
{
public:
    TMap<FName, bool> BoolParams;
    TMap<FName, float> FloatParams;
    TMap<FName, int32> IntParams;

    void SetBool(FName Name, bool Value) { BoolParams.FindOrAdd(Name) = Value; }
    bool GetBool(FName Name, bool DefaultValue = false) const
    {
        const bool* Found = BoolParams.Find(Name);
        return Found ? *Found : DefaultValue;
    }

    void SetFloat(FName Name, float Value) { FloatParams.FindOrAdd(Name) = Value; }
    float GetFloat(FName Name, float DefaultValue = 0.0f) const
    {
        const float* Found = FloatParams.Find(Name);
        return Found ? *Found : DefaultValue;
    }

    void SetInt(FName Name, int32 Value) { IntParams.FindOrAdd(Name) = Value; }
    int32 GetInt(FName Name, int32 DefaultValue = 0) const
    {
        const int32* Found = IntParams.Find(Name);
        return Found ? *Found : DefaultValue;
    }
};

using FTransitionPredicate = std::function<bool(const FAnimationConditionContext& Context)>;

struct FAnimationTransition
{
    FName TargetStateName;
    FTransitionPredicate TransitionRule;
    float BlendDuration;
    int32 Priority;

    FAnimationTransition()
        : TargetStateName(""), BlendDuration(0.2f), Priority(0) {
    }

    FAnimationTransition(FName InTargetStateName, FTransitionPredicate InTransitionRule, float InBlendDuration = 0.2f, int32 InPriority = 0)
        : TargetStateName(InTargetStateName), TransitionRule(InTransitionRule), BlendDuration(InBlendDuration), Priority(InPriority) {
    }

    bool IsValid() const { return TargetStateName != "" && TransitionRule != nullptr; }
};



struct FAnimationState
{
    FName StateName;
    int32 OwningAnimIndex;
    TArray<FAnimationTransition> Transitions;

    // 상태 진입/유지/탈출 시 호출될 델리게이트 (선택 사항)
     std::function<void(FAnimationConditionContext& Context)> OnEnter;
     std::function<void(FAnimationConditionContext& Context, float DeltaTime)> OnUpdate;
     std::function<void(FAnimationConditionContext& Context)> OnExit;

    float PlayRate;
    bool bLooping;

    FAnimationState()
        : StateName(""), OwningAnimIndex(INDEX_NONE), PlayRate(1.0f), bLooping(true) {
    }
    FAnimationState(FName InName, int32 InAnimIndex = INDEX_NONE, float InPlayRate = 1.0f, bool bInLoop = true)
        : StateName(InName), OwningAnimIndex(InAnimIndex), PlayRate(InPlayRate), bLooping(bInLoop) {}

    void AddTransition(const FAnimationTransition& Transition)
    {
        Transitions.Add(Transition);
        // 우선순위에 따라 정렬
        Transitions.Sort([](const FAnimationTransition& A, const FAnimationTransition& B) {return A.Priority > B.Priority;});
    }
};