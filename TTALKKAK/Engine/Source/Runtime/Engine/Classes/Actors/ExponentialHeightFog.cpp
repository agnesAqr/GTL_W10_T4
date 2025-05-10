#include "ExponentialHeightFog.h"
#include "Components/PrimitiveComponents/HeightFogComponent.h"

AExponentialHeightFogActor::AExponentialHeightFogActor()
    :Super()
{
    FogComponent = AddComponent<UHeightFogComponent>(EComponentOrigin::Constructor);
}

AExponentialHeightFogActor::AExponentialHeightFogActor(const AExponentialHeightFogActor& Other)
    :Super(Other)
{
}

AExponentialHeightFogActor::~AExponentialHeightFogActor()
{
}

void AExponentialHeightFogActor::BeginPlay()
{
    Super::BeginPlay();
}

void AExponentialHeightFogActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AExponentialHeightFogActor::Destroyed()
{
    Super::Destroyed();
}

void AExponentialHeightFogActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool AExponentialHeightFogActor::Destroy()
{
    return Super::Destroy();
}

UObject* AExponentialHeightFogActor::Duplicate(UObject* InOuter)
{
    AExponentialHeightFogActor* NewActor = FObjectFactory::ConstructObjectFrom<AExponentialHeightFogActor>(this, InOuter);
    NewActor->DuplicateSubObjects(this, InOuter);
    NewActor->PostDuplicate();
    return NewActor;
}

void AExponentialHeightFogActor::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    const AExponentialHeightFogActor* SourceHeightFogActor = FObjectFactory::ConstructObjectFrom<AExponentialHeightFogActor>(this, InOuter);
    if (SourceHeightFogActor == nullptr) return;
    FogComponent = Cast<UHeightFogComponent>(SourceHeightFogActor->FogComponent);
}

void AExponentialHeightFogActor::PostDuplicate()
{
    Super::PostDuplicate();
}

void AExponentialHeightFogActor::LoadAndConstruct(const TArray<std::unique_ptr<FActorComponentInfo>>& InfoArray)
{
    Super::LoadAndConstruct(InfoArray);
}

FActorInfo AExponentialHeightFogActor::GetActorInfo()
{
    return Super::GetActorInfo();
}
