#include "ChracterMovementComponent.h"

UChracterMovementComponent::UChracterMovementComponent()
{
    MaxWalkSpeed = 600.f;
    MaxAcceleration = 2048.f;
    BrakingFriction = 0.f;
}

UChracterMovementComponent::UChracterMovementComponent(const UChracterMovementComponent& Other)
    : UPawnMovementComponent(Other)
    , MaxWalkSpeed(Other.MaxWalkSpeed)
    , MaxAcceleration(Other.MaxAcceleration)
    , BrakingFriction(Other.BrakingFriction)
{
}

void UChracterMovementComponent::InitializeComponent()
{
}

void UChracterMovementComponent::TickComponent(float DeltaTime)
{
}

UObject* UChracterMovementComponent::Duplicate(UObject* InOuter)
{
    UChracterMovementComponent* NewComp = FObjectFactory::ConstructObjectFrom<UChracterMovementComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();
    return NewComp;
}

void UChracterMovementComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UMovementComponent::DuplicateSubObjects(Source, InOuter);
}

void UChracterMovementComponent::PostDuplicate()
{
    UMovementComponent::PostDuplicate();
}

std::unique_ptr<FActorComponentInfo> UChracterMovementComponent::GetComponentInfo()
{
    auto Info = std::make_unique<FCharacterMovementComponentInfo>();
    SaveComponentInfo(*Info);

    return Info;
}

void UChracterMovementComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
{
    FCharacterMovementComponentInfo* Info = static_cast<FCharacterMovementComponentInfo*>(&OutInfo);
    Super::SaveComponentInfo(*Info);
    Info->MaxWalkSpeed = MaxWalkSpeed;
    Info->MaxAcceleration = MaxAcceleration;
    Info->BrakingFriction = BrakingFriction;
}

void UChracterMovementComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FCharacterMovementComponentInfo& CharacterMovementComponentInfo = static_cast<const FCharacterMovementComponentInfo&>(Info);
    MaxWalkSpeed = CharacterMovementComponentInfo.MaxWalkSpeed;
    MaxAcceleration = CharacterMovementComponentInfo.MaxAcceleration;
    BrakingFriction = CharacterMovementComponentInfo.BrakingFriction;
}
