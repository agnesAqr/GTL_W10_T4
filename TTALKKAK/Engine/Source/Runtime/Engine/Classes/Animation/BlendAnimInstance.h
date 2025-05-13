#pragma once
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

class UAnimSequence;

class UBlendAnimInstance : public UAnimInstance
{
    DECLARE_CLASS(UBlendAnimInstance, UAnimInstance)

public:
    UBlendAnimInstance();
    UBlendAnimInstance(const UBlendAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    PROPERTY(float, BlendAlpha);

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    UAnimSequence* AnimationA;
    UAnimSequence* AnimationB;
    float BlendAlpha;

    FPoseData FinalBlendedPose;

    float CurrentTimeA;
    float CurrentTimeB;
};

