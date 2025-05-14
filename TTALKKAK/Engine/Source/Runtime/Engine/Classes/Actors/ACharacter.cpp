#include "ACharacter.h"

#include "FBXLoader.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"

ACharacter::ACharacter()
    : APawn()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>(EComponentOrigin::Constructor);
    RootComponent = SkeletalMeshComponent;
    USkeletalMesh* Mesh = FBXLoader::GetSkeletalMesh(TEXT("FBX/mixmix2_2.fbx"));
    SkeletalMeshComponent->SetSkeletalMesh(Mesh);
}

ACharacter::ACharacter(const ACharacter& Other)
    : APawn(Other)
{
}

ACharacter::~ACharacter() = default;

void ACharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACharacter::Destroyed()
{
    Super::Destroyed();
}

void ACharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool ACharacter::Destroy()
{
    return Super::Destroy();
}

UObject* ACharacter::Duplicate(UObject* InOuter)
{
    ACharacter* NewChar = FObjectFactory::ConstructObjectFrom<ACharacter>(this, InOuter);
    NewChar->DuplicateSubObjects(this, InOuter);
    NewChar->PostDuplicate();
    return NewChar;
}

void ACharacter::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
}

void ACharacter::PostDuplicate()
{
    Super::PostDuplicate();
}

void ACharacter::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    if (Notify.NotifyName == "")
    {
        // TriggerFire();  // 실제 로직 호출
    }
}
