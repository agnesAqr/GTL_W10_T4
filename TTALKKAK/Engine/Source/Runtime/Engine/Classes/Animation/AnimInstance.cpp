#include "AnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"


UAnimInstance::UAnimInstance(const UAnimInstance& Other)
    : UObject(Other)
    , TargetSkeleton(Other.TargetSkeleton)
    , CurrentPoseData(Other.CurrentPoseData)
{
}

UObject* UAnimInstance::Duplicate(UObject* InOuter)
{
    UAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UAnimInstance>(this, InOuter);
    NewAnimInstance->DuplicateSubObjects(this, InOuter);
    NewAnimInstance->PostDuplicate();
    return NewAnimInstance;
}

void UAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    UAnimInstance* Origin = Cast<UAnimInstance>(Source);
}

void UAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UAnimInstance::NativeInitializeAnimation()
{
    USkeletalMeshComponent* SkelComp = GetSkelMeshComponent();
    TargetSkeleton = SkelComp->GetSkeletalMesh()->GetRefSkeletal();
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
}

const FPoseData& UAnimInstance::GetCurrentPose() const
{
    return CurrentPoseData;
}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const
{
    return Cast<USkeletalMeshComponent>(GetOuter());
}

void UAnimInstance::TriggerAnimNotifies(float PrevTime, float CurrTime)
{
}
