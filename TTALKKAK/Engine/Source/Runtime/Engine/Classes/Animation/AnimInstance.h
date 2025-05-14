#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "FBX/FBXDefine.h" // FIX-ME
#include "Classes/Engine/Assets/Animation/AnimTypes.h"
class UAnimSequence;

class USkeletalMeshComponent;

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
    virtual void TriggerAnimNotifies(float PrevTime, float CurrTime);

    const FRefSkeletal* TargetSkeleton;
    virtual const FPoseData& GetCurrentPose() const;
    USkeletalMeshComponent* GetSkelMeshComponent() const;

    virtual void InitializeForBlendAnimations(const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose);
    virtual void BlendAnimations(float DeltaSeconds, const uint32 IndexA, const uint32 IndexB, FPoseData& OutPose);

    PROPERTY(float, BlendAlpha);


protected:
    FPoseData CurrentPoseData;

    TArray<UAnimSequence*> OwningAnimSequences;
    TArray<FPoseData> OwningPoseData;

    float BlendAlpha;
    float NormalizedTime;

private:
};

