#include "ACharacter.h"

#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"

ACharacter::ACharacter()
    : APawn()
{
    // Constructor 시점에 필요한 컴포넌트들을 추가합니다.
    // CharacterMovement    = AddComponent<UCharacterMovementComponent>(EComponentOrigin::Constructor);
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>(EComponentOrigin::Constructor);
    SkeletalMeshComponent -> LoadSkeletalMesh(TEXT("FBX/mixmix2.fbx"));
}

ACharacter::ACharacter(const ACharacter& Other)
    : APawn(Other)
{
    // DuplicateSubObjects 에서 컴포넌트 연결
}

ACharacter::~ACharacter() = default;

void ACharacter::BeginPlay()
{
    APawn::BeginPlay();
    // if (CharacterMovement)
    //     CharacterMovement->BeginPlay();
    if (SkeletalMeshComponent)
        SkeletalMeshComponent->BeginPlay();
}

void ACharacter::Tick(float DeltaTime)
{
    APawn::Tick(DeltaTime);
    // if (CharacterMovement && CharacterMovement->IsComponentTickEnabled())
    //     CharacterMovement->TickComponent(DeltaTime);
    if (SkeletalMeshComponent && SkeletalMeshComponent->IsComponentTickEnabled())
        SkeletalMeshComponent->TickComponent(DeltaTime);
}

void ACharacter::Destroyed()
{
    APawn::Destroyed();
}

void ACharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    APawn::EndPlay(EndPlayReason);
}

bool ACharacter::Destroy()
{
    return APawn::Destroy();
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
    APawn::DuplicateSubObjects(Source, InOuter);
    // 복제된 컴포넌트들 중에서 해당 타입의 컴포넌트를 찾아 다시 연결
    //CharacterMovement     = GetComponentByClass<UCharacterMovementComponent>();
    SkeletalMeshComponent = GetComponentByClass<USkeletalMeshComponent>();
}

void ACharacter::PostDuplicate()
{
    // 필요 시 후처리
}

void ACharacter::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    // 예시: NotifyName 에 따라 이벤트 처리
    if (Notify.NotifyName == "")
    {
        // TriggerFire();  // 실제 로직 호출
    }
}
