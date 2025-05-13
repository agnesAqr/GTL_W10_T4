#pragma once
#include "MovementComponent.h"

struct FPawnMovementComponentInfo : public FMovementComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FPawnMovementComponentInfo);
    FPawnMovementComponentInfo()
        : FMovementComponentInfo()
        , MaxSpeed(600.f)         
        , Acceleration(2000.f)    
        , Deceleration(2000.f)    
        //, bUseControllerDesiredRotation(false)
        //, bOrientRotationToMovement(true)
        , RotationRate(FRotator(0.f, 180.f, 0.f))
        , GravityScale(1.0f)
    {
        InfoType = TEXT("FPawnMovementComponentInfo");
        ComponentClass = TEXT("UPawnMovementComponent");
    }

    float MaxSpeed;
    float Acceleration;
    float Deceleration;

    //bool bUseControllerDesiredRotation;
    //bool bOrientRotationToMovement;
    FRotator RotationRate;
    float GravityScale;

    virtual void Serialize(FArchive& ar) const override
    {
        FMovementComponentInfo::Serialize(ar);
        ar << MaxSpeed;
        ar << Acceleration;
        ar << Deceleration;
        //ar << bUseControllerDesiredRotation;
        //ar << bOrientRotationToMovement;
        ar << RotationRate;
        ar << GravityScale;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FMovementComponentInfo::Deserialize(ar);
        ar >> MaxSpeed;
        ar >> Acceleration;
        ar >> Deceleration;
        //ar >> bUseControllerDesiredRotation;
        //ar >> bOrientRotationToMovement;
        ar >> RotationRate;
        ar >> GravityScale;
    }
};

class UPawnMovementComponent : public UMovementComponent // UNavMovementComponent는 생략
{
    DECLARE_CLASS(UPawnMovementComponent, UMovementComponent)

public:
    UPawnMovementComponent();
    UPawnMovementComponent(const UPawnMovementComponent& Other);

    void InitializeComponent() override;

    virtual void TickComponent(float DeltaTime) override;

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

    std::unique_ptr<FActorComponentInfo> GetComponentInfo() override;

    float MaxSpeed;
    float Acceleration;
    float Deceleration;

    //bool bUseControllerDesiredRotation;
    //bool bOrientRotationToMovement;
    FRotator RotationRate;
    float GravityScale;

public:
    virtual void SaveComponentInfo(FActorComponentInfo& OutInfo) override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info);
};

