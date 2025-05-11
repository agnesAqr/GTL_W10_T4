#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "FBX/FBXDefine.h" // FIX-ME

class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)

public:
    UAnimInstance() = default;
    UAnimInstance(const UAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

//protected:
    virtual void NativeInitializeAnimation();
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    // 
    // 프록시 타입

protected:
    FRefSkeletal* TargetSkeleton;
};

