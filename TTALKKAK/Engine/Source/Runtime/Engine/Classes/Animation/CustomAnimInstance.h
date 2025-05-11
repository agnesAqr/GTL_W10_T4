#pragma once
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

class UAnimSequence;

class UCustomAnimInstance : public UAnimInstance // AnimSingleNodeInstance로 변경할 수도 있을듯
{
    DECLARE_CLASS(UCustomAnimInstance, UAnimInstance)

public:
    UCustomAnimInstance();
    UCustomAnimInstance(const UCustomAnimInstance& Other);

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

