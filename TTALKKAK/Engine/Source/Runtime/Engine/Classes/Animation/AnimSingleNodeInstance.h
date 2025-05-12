#pragma once
#include "Animation/AnimInstance.h"

class UAnimSequence;
struct FRefSkeletal;

class UAnimSingleNodeInstance : public UAnimInstance
{
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
    UAnimSingleNodeInstance();
    UAnimSingleNodeInstance(const UAnimSingleNodeInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void TriggerAnimNotifies(float PrevTime, float CurrTime) override;
    
private:
    UAnimSequence* CurrentAnimationSeq;
    float CurrentTime;
    float PreviousTime;

public:
    UAnimSequence* GetAnimation() const { return CurrentAnimationSeq; }
    void SetAnimation(UAnimSequence* AnimSequence);
};