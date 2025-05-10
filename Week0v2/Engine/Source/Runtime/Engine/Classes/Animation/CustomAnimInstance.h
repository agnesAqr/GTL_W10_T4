#pragma once
#include "Animation/AnimInstance.h"

class UAnimSequence;

class UCustomAnimInstance : public UAnimInstance
{
    DECLARE_CLASS(UCustomAnimInstance, UAnimInstance)

public:
    UCustomAnimInstance() = default;
    UCustomAnimInstance(const UCustomAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    UAnimSequence* AnimationA;
    UAnimSequence* AnimationB;

    PROPERTY(float, BlendAlpha);

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    float BlendAlpha = 1.f;
};

