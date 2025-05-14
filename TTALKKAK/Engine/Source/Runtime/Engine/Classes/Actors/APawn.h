#pragma once

#include "Classes/GameFramework/Actor.h"

class APawn : public AActor
{
    DECLARE_CLASS(APawn, AActor)

public:
    APawn();
    APawn(const APawn& Other);
    virtual ~APawn();

    // Actor
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Destroyed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool Destroy() override;

    // Duplicate
    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    virtual void PostDuplicate() override;

protected:
};
