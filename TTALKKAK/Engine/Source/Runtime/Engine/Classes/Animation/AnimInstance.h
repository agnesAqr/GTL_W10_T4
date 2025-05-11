#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "FBX/FBXDefine.h" // FIX-ME
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)

public:
    UAnimInstance() = default;
    UAnimInstance(const UAnimInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    virtual void NativeInitializeAnimation();
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    FRefSkeletal* TargetSkeleton;
    virtual const FPoseData& GetCurrentPose() const;

protected:
    FPoseData CurrentPoseData;
};

