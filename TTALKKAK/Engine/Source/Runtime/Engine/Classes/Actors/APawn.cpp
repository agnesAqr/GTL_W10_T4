#include "APawn.h"

APawn::APawn()
    : AActor()
{

}

APawn::APawn(const APawn& Other)
    : AActor(Other)
{
}

APawn::~APawn() = default;

void APawn::BeginPlay()
{
    AActor::BeginPlay();
}

void APawn::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void APawn::Destroyed()
{
    AActor::Destroyed();
}

void APawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AActor::EndPlay(EndPlayReason);
}

bool APawn::Destroy()
{
    return AActor::Destroy();
}

UObject* APawn::Duplicate(UObject* InOuter)
{
    APawn* NewPawn = FObjectFactory::ConstructObjectFrom<APawn>(this, InOuter);
    NewPawn->DuplicateSubObjects(this, InOuter);
    NewPawn->PostDuplicate();
    return NewPawn;
}

void APawn::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    AActor::DuplicateSubObjects(Source, InOuter);
}

void APawn::PostDuplicate()
{
    AActor::PostDuplicate();
}
