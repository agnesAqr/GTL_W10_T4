#include "PawnMovementComponent.h"

UPawnMovementComponent::UPawnMovementComponent()
{
    MaxSpeed = 600.f;
    Acceleration = 2000.f;
    Deceleration = 2000.f;

    //bool bUseControllerDesiredRotation;
    //bool bOrientRotationToMovement;
    RotationRate=FRotator(0.f, 180.f, 0.f);
    GravityScale=1.f;
}

UPawnMovementComponent::UPawnMovementComponent(const UPawnMovementComponent& Other)
    : UMovementComponent(Other)
    , MaxSpeed(Other.MaxSpeed)
    , Acceleration(Other.Acceleration)
    , Deceleration(Other.Deceleration)
    , RotationRate(Other.RotationRate)
    , GravityScale(Other.GravityScale)
{
}

void UPawnMovementComponent::InitializeComponent()
{
}

void UPawnMovementComponent::TickComponent(float DeltaTime)
{
}

UObject* UPawnMovementComponent::Duplicate(UObject* InOuter)
{
    UPawnMovementComponent* NewComp = FObjectFactory::ConstructObjectFrom<UPawnMovementComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();
    return NewComp;
}

void UPawnMovementComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UMovementComponent::DuplicateSubObjects(Source, InOuter);
}

void UPawnMovementComponent::PostDuplicate()
{
    UMovementComponent::PostDuplicate();
}

std::unique_ptr<FActorComponentInfo> UPawnMovementComponent::GetComponentInfo()
{
    auto Info = std::make_unique<FPawnMovementComponentInfo>();
    SaveComponentInfo(*Info);

    return Info;
}

void UPawnMovementComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
{
    FPawnMovementComponentInfo* Info = static_cast<FPawnMovementComponentInfo*>(&OutInfo);
    Super::SaveComponentInfo(*Info);
    Info->MaxSpeed = MaxSpeed;
    Info->Acceleration=Acceleration;
    Info->Deceleration = Deceleration;
    Info->RotationRate=RotationRate;
    Info->GravityScale = GravityScale;
}

void UPawnMovementComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FPawnMovementComponentInfo& PawnMovementComponentInfo = static_cast<const FPawnMovementComponentInfo&>(Info);
    MaxSpeed = PawnMovementComponentInfo.MaxSpeed;
    Acceleration = PawnMovementComponentInfo.Acceleration;
    Deceleration = PawnMovementComponentInfo.Deceleration;
    RotationRate = PawnMovementComponentInfo.RotationRate;
    GravityScale = PawnMovementComponentInfo.GravityScale;
}
