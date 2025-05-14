#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

class UBlendAnimInstance : public UAnimInstance
{
    DECLARE_CLASS(UBlendAnimInstance, UAnimInstance)

public:
    UBlendAnimInstance();
    UBlendAnimInstance(const UBlendAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void TriggerAnimNotifies(float PrevTime, float CurrTime) override;

    virtual void InitializeForBlendAnimations(const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose) override;
    virtual void BlendAnimations(float DeltaSeconds, const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose) override;
private:

};

