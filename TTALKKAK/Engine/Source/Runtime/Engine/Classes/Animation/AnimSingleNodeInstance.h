#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
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
    UAnimSequence* AnimationSequence;
    float CurrentTime;
    bool bLooping;

public:
    UAnimSequence* GetAnimation() const { return AnimationSequence; }
    void SetAnimation(UAnimSequence* AnimSequence, bool bShouldLoop);

    void SetLooping(bool InValue) { bLooping = InValue; }
    bool IsLooping() const { return bLooping; }
};