#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

#include "Delegates/DelegateCombination.h"
#include "SlateCore/Input/Events.h"

class UCustomAnimInstance : public UAnimInstance
{
    DECLARE_CLASS(UCustomAnimInstance, UAnimInstance)

public:
    UCustomAnimInstance();
    UCustomAnimInstance(const UCustomAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void InitializeForAnimationStateMachine() override;

private:

    // FIX-ME: Hard coded
    void SampleSingleAnimationByIndex(float DeltaSeconds, int32 AnimIndex, FPoseData& OutPose);
    void UpdateBlendedAnimations(float DeltaSeconds, int32 IndexA, int32 IndexB, float InBlendAlpha, FPoseData& OutPose);


    TArray<FDelegateHandle> InputDelegateHandles;
    void HandleSpacebarPressed(const FKeyEvent& InKeyEvent, HWND AppWnd);
};

