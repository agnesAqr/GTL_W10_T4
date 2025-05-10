#pragma once
#include "PawnMovementComponent.h"

struct FPawnMovementComponentInfo;

enum class ECharacterMovementMode : uint8
{
    None,
    Walking,
    MAX
}; 

struct FCharacterMovementComponentInfo : public FPawnMovementComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FCharacterMovementComponentInfo);

    FCharacterMovementComponentInfo()
        : FPawnMovementComponentInfo()
        , MaxWalkSpeed(600.f)
        , MaxAcceleration(2048.f)
        , BrakingFriction(0.f)
        //, DefaultLandMovementMode(ECharacterMovementMode::Walking)
    {
        InfoType = TEXT("FCharacterMovementComponentInfo");
        ComponentClass = TEXT("UCharacterMovementComponent");
    }

    float MaxWalkSpeed;
    float MaxAcceleration;
    float BrakingFriction;

    //ECharacterMovementMode DefaultLandMovementMode;

    virtual void Serialize(FArchive& ar) const override
    {
        FPawnMovementComponentInfo::Serialize(ar);

        ar << MaxWalkSpeed;
        ar << MaxAcceleration;
        ar << BrakingFriction;

        //ar << DefaultLandMovementMode;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FPawnMovementComponentInfo::Deserialize(ar);

        ar >> MaxWalkSpeed;
        ar >> MaxAcceleration;
        ar >> BrakingFriction;

        //ar >> DefaultLandMovementMode;
    }
};
class UChracterMovementComponent : public UPawnMovementComponent
{
public:
    UChracterMovementComponent();
    UChracterMovementComponent(const UChracterMovementComponent& Other);

    void InitializeComponent() override;

    virtual void TickComponent(float DeltaTime) override;

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    std::unique_ptr<FActorComponentInfo> GetComponentInfo() override;

    float MaxWalkSpeed;
    float MaxAcceleration;
    float BrakingFriction;

public:
    virtual void SaveComponentInfo(FActorComponentInfo& OutInfo) override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info);
};

