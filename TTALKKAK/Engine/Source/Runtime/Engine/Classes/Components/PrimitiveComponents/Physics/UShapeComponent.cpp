#include "UShapeComponent.h"
#include "Launch/EditorEngine.h"
#include "Components/SceneComponent.h"
#include "Physics/FCollisionManager.h"

UShapeComponent::UShapeComponent()
    : ShapeColor(FLinearColor::Green)
    , bDrawOnlyIfSelected(true)
    , PrevLocation(FVector::ZeroVector)
    , PrevRotation(FRotator(0, 0, 0))
    , PrevScale(FVector::OneVector)
{
}

UShapeComponent::UShapeComponent(const UShapeComponent& Other)
    : UPrimitiveComponent(Other)
    , ShapeColor(Other.ShapeColor)
    , bDrawOnlyIfSelected(Other.bDrawOnlyIfSelected)
    , PrevLocation(Other.GetPrevLocation())
    , PrevRotation(Other.GetPrevRotation())
    , PrevScale(Other.GetPrevScale())
{
}

UShapeComponent::~UShapeComponent()
{
    
}

void UShapeComponent::InitializeComponent()
{
    Super::InitializeComponent();

    bDrawOnlyIfSelected = true;
    BroadAABB = FBoundingBox(FVector(-1, -1, -1), FVector(1, 1, 1));
}

void UShapeComponent::BeginPlay()
{
    UEditorEngine::CollisionManager.Register(this);
}

void UShapeComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    const FVector CurLocation = GetWorldLocation();
    const FRotator CurRotation = GetWorldRotation();
    const FVector CurScale = GetWorldScale();

    if (PrevLocation != CurLocation || PrevRotation != CurRotation || PrevScale != CurScale)
    {
        UpdateBroadAABB();

        PrevLocation = CurLocation;
        PrevRotation = CurRotation;
        PrevScale = CurScale;
    }
}

void UShapeComponent::DestroyComponent()
{
    Super::DestroyComponent();
    UEditorEngine::CollisionManager.Unregister(this);
}

UObject* UShapeComponent::Duplicate(UObject* InOuter)
{
    UShapeComponent* NewComp = FObjectFactory::ConstructObjectFrom<UShapeComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();

    return NewComp;
}

void UShapeComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
}

void UShapeComponent::PostDuplicate()
{
    Super::PostDuplicate();
}

FBoundingBox UShapeComponent::GetBroadAABB()
{
    UpdateBroadAABB();
    return BroadAABB;
}

bool UShapeComponent::TestOverlaps(UShapeComponent* OtherShape)
{
    return false;
}

bool UShapeComponent::BroadPhaseCollisionCheck(UShapeComponent* OtherShape)
{
    return BroadAABB.IntersectAABB(OtherShape->GetBroadAABB());
}
