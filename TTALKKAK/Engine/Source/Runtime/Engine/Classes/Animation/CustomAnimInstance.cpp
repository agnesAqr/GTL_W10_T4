#include "CustomAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"

UCustomAnimInstance::UCustomAnimInstance()
{
    AnimationA = nullptr;
    AnimationB = nullptr;
    BlendAlpha = 1.f;
}

UCustomAnimInstance::UCustomAnimInstance(const UCustomAnimInstance& Other)
    : AnimationA(Other.AnimationA)
    , AnimationB(Other.AnimationB)
    , BlendAlpha(Other.BlendAlpha)
{
}

UObject* UCustomAnimInstance::Duplicate(UObject* InOuter)
{
    UCustomAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UCustomAnimInstance>(this, InOuter);
    NewAnimInstance->DuplicateSubObjects(this, InOuter);
    NewAnimInstance->PostDuplicate();
    return NewAnimInstance;
}

void UCustomAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    UCustomAnimInstance* Origin = Cast<UCustomAnimInstance>(Source);
}

void UCustomAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UCustomAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    // 애님 시퀀스 가져오는 함수 호출
    //AnimationA;
    //AnimationB;

    SetBlendAlpha(1.f);
}

void UCustomAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 여기에 애니메이션 블렌딩하는 부분을 추가할 예정
}
