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

private:
    UAnimSequence* CurrentAnimationSeq;
    float CurrentTime;

public:
    UAnimSequence* GetAnimation() const { return CurrentAnimationSeq; }
    void SetAnimation(UAnimSequence* AnimSequence);
};