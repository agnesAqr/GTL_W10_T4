#include "AnimInstance.h"
#include "CoreUObject/UObject/Casts.h"

UAnimInstance::UAnimInstance(const UAnimInstance& Other)
    : UObject(Other)
    , TargetSkeleton(Other.TargetSkeleton)
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
    // 여기서 스켈레톤 하나 가져오는 방식으로 하는게 좋을듯.
    //TargetSkeleton=
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
}
