#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

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


    // 스켈레톤을 넣어야 하는데 스켈레톤이 없이 그냥 처리하고 있지 않나?

    // 
    // 프록시 타입
};

