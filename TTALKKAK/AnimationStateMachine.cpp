#include "AnimationStateMachine.h"
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

UAnimationStateMachine::UAnimationStateMachine()
    : CurrentStateName("")
    , TransitionTargetStateName("")
    , BlendingFromStateName("")
    , bIsBlending(false)
    , CurrentBlendTime(0.0f)
    , TotalBlendDuration(0.0f)
{
}

UAnimationStateMachine::UAnimationStateMachine(const UAnimationStateMachine& Other)
    : UObject(Other)
{
    CopyStatesAndTransitions(Other);
    CurrentStateName = Other.CurrentStateName;
    TransitionTargetStateName = Other.TransitionTargetStateName;
    BlendingFromStateName = Other.BlendingFromStateName;
    bIsBlending = Other.bIsBlending;
    CurrentBlendTime = Other.CurrentBlendTime;
    TotalBlendDuration = Other.TotalBlendDuration;
}

void UAnimationStateMachine::CopyStatesAndTransitions(const UAnimationStateMachine& Source)
{
    States.Empty();
    for (const auto& Pair : Source.States)
    {
        const FAnimationState& SourceState = Pair.Value;
        FAnimationState NewState = SourceState;

        NewState.Transitions.Empty();
        for(const FAnimationTransition& SourceTrans : SourceState.Transitions)
        {
            NewState.Transitions.Add(SourceTrans);
        }
        States.Add(Pair.Key, NewState);
    }
}

UObject* UAnimationStateMachine::Duplicate(UObject* InOuter)
{
    UAnimationStateMachine* NewStateMachine = FObjectFactory::ConstructObjectFrom<UAnimationStateMachine>(this, InOuter);
    NewStateMachine->DuplicateSubObjects(this, InOuter);
    NewStateMachine->PostDuplicate();
    return NewStateMachine;
}

void UAnimationStateMachine::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
}

void UAnimationStateMachine::PostDuplicate()
{
    Super::PostDuplicate();
}

FAnimationState* UAnimationStateMachine::AddState(FName StateName, int32 AnimIndexInOwner, float PlayRate, bool bLoop)
{
    if (StateName == "") return nullptr;
    if (States.Contains(StateName))
    {
        return &States[StateName];
    }
    return &States.Emplace(StateName, FAnimationState(StateName, AnimIndexInOwner, PlayRate, bLoop));
}

FAnimationState* UAnimationStateMachine::GetState(FName StateName)
{
    return States.Find(StateName);
}

const FAnimationState* UAnimationStateMachine::GetState(FName StateName) const
{
    return States.Find(StateName);
}

// FIX-ME: 실행 중인 상태 삭제 시 문제 발생 가능
// 현재 상태이거나 전환 중인 상태는 제거하지 않도록 예외 처리 필요함
void UAnimationStateMachine::RemoveState(FName StateName)
{
    States.Remove(StateName);
}

bool UAnimationStateMachine::AddTransition(FName FromStateName, FName ToStateName, FTransitionPredicate TransitionRule, float BlendDuration, int32 Priority)
{
    FAnimationState* FromState = GetState(FromStateName);
    if (!FromState || !States.Contains(ToStateName) || !TransitionRule)
    {
        return false;
    }

    FromState->AddTransition(FAnimationTransition(ToStateName, TransitionRule, BlendDuration, Priority));
    return true;
}

void UAnimationStateMachine::SetInitialState(FName StateName)
{
    if (FAnimationState* State = GetState(StateName))
    {
        CurrentStateName = StateName;   
        TransitionTargetStateName = "";
        BlendingFromStateName = "";
        bIsBlending = false;
        CurrentBlendTime = 0.0f;
        TotalBlendDuration = 0.0f;

        //State->OnEnter = [](FAnimationConditionContext& Context) {
        //    UE_LOG(LogLevel::Display, TEXT("State->OnEnter"));
        //    Context.SetBool(FName("IsMoving"), true);
        //    };
            

        // 초기 진입 시 컨텍스트 (필요 시 외부에서 받아오도록 수정)
        FAnimationConditionContext EmptyContext;
        EnterState(CurrentStateName, EmptyContext);
    }
    else
    {
        CurrentStateName = "";
    }
}

const FAnimationState* UAnimationStateMachine::GetCurrentState() const
{
    return GetState(CurrentStateName);
}

float UAnimationStateMachine::GetBlendAlpha() const
{
    if (bIsBlending && TotalBlendDuration > 0.0f)
    {
        float Alpha = CurrentBlendTime / TotalBlendDuration;
        return (Alpha < 0.0f) ? 0.0f : ((Alpha > 1.0f) ? 1.0f : Alpha);
    }
    return bIsBlending ? 1.0f : 0.0f;
}

void UAnimationStateMachine::Tick(float DeltaTime, FAnimationConditionContext& Context)
{
    if (CurrentStateName == "") return;

    const FAnimationState* ActiveState = GetCurrentState();
    if (!ActiveState) return;

    if (bIsBlending)
    {
        CurrentBlendTime += DeltaTime;
        if (CurrentBlendTime >= TotalBlendDuration)
        {
            // 블렌딩 완료
            bIsBlending = false;
            CurrentBlendTime = 0.0f;

            ExitState(BlendingFromStateName, Context);
            CurrentStateName = TransitionTargetStateName;
            TransitionTargetStateName = "";
            BlendingFromStateName = "";
        }
    }

    UpdateState(CurrentStateName, DeltaTime, Context);
    if (bIsBlending && BlendingFromStateName != "")
    {
        UpdateState(BlendingFromStateName, DeltaTime, Context);
    }


    if (!bIsBlending)
    {
        for (const FAnimationTransition& Transition : ActiveState->Transitions)
        {
            if (!Transition.IsValid() || !Transition.TransitionRule(Context)) continue;

            if (GetState(Transition.TargetStateName))
            {
                ExitState(CurrentStateName, Context);

                BlendingFromStateName = CurrentStateName;
                TransitionTargetStateName = Transition.TargetStateName;
                TotalBlendDuration = Transition.BlendDuration;
                CurrentBlendTime = 0.0f;

                EnterState(TransitionTargetStateName, Context);

                if (TotalBlendDuration > MINIMUM_ANIMATION_LENGTH)
                {
                    bIsBlending = true;
                }
                else
                {
                    bIsBlending = false;
                    CurrentStateName = TransitionTargetStateName;
                    TransitionTargetStateName = "";
                    BlendingFromStateName = "";
                }
                return;
            }
        }
    }
}

int32 UAnimationStateMachine::GetOutputAnimIndex(int32& InOwningAnimIndex) const
{
    InOwningAnimIndex = INDEX_NONE;
    const FAnimationState* MainFSMState = nullptr;
    const FAnimationState* PrevFSMState = nullptr;

    if (bIsBlending)
    {
        // 블렌딩 중
        PrevFSMState = GetState(BlendingFromStateName);
        MainFSMState = GetState(TransitionTargetStateName);

        if (PrevFSMState) InOwningAnimIndex = PrevFSMState->OwningAnimIndex;
        return MainFSMState ? MainFSMState->OwningAnimIndex : INDEX_NONE;
    }
    else
    {
        // 블렌딩 중 아님
        MainFSMState = GetCurrentState();
        return MainFSMState ? MainFSMState->OwningAnimIndex : INDEX_NONE;
    }
}


void UAnimationStateMachine::EnterState(FName StateName, FAnimationConditionContext& Context)
{
    FAnimationState* State = GetState(StateName);
    if (State)
    {
        //UE_LOG(LogLevel::Display, TEXT("Entering State: %s"), *StateName.ToString());
        if (State->OnEnter) State->OnEnter(Context);
    }
}

void UAnimationStateMachine::ExitState(FName StateName, FAnimationConditionContext& Context)
{
    FAnimationState* State = GetState(StateName);
    if (State)
    {
        //UE_LOG(LogLevel::Display, TEXT("Exiting State: %s"), *StateName.ToString());
        if (State->OnExit) State->OnExit(Context);
    }
} 

void UAnimationStateMachine::UpdateState(FName StateName, float DeltaTime, FAnimationConditionContext& Context)
{
    FAnimationState* State = GetState(StateName);
    if (State)
    {
        //UE_LOG(LogLevel::Display, TEXT("Updating State: %s"), *StateName.ToString());
        if (State->OnUpdate) State->OnUpdate(Context, DeltaTime);
    }
}