#include "ActorComponent.h"

#include "GameFramework/Actor.h"
#include "UObject/UObjectArray.h"

UActorComponent::UActorComponent()
    : ComponentID(FGuid::NewGuid())
{
}

UActorComponent::UActorComponent(const UActorComponent& Other)
    : UObject(Other)
    , bCanEverTick(Other.bCanEverTick)
    , bRegistered(Other.bRegistered)
    , bWantsInitializeComponent(Other.bWantsInitializeComponent)
    , bIsBeingDestroyed(Other.bIsBeingDestroyed)
    , bIsActive(Other.bIsActive)
    , bTickEnabled(Other.bTickEnabled)
    , bAutoActive(Other.bAutoActive)
    , ComponentID(FGuid::NewGuid())
    , ComponentOrigin(Other.ComponentOrigin)
{
    // Owner는 복제 시점에 AActor가 직접 지정
}

void UActorComponent::InitializeComponent()
{
    assert(!bHasBeenInitialized);

    bHasBeenInitialized = true;
}

void UActorComponent::UninitializeComponent()
{
    assert(bHasBeenInitialized);

    bHasBeenInitialized = false;
}

void UActorComponent::BeginPlay()
{
    bHasBegunPlay = true;
}

void UActorComponent::TickComponent(float DeltaTime)
{
}

void UActorComponent::OnComponentDestroyed()
{
}

void UActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    assert(bHasBegunPlay);

    bHasBegunPlay = false;
}

void UActorComponent::DestroyComponent()
{
    if (bIsBeingDestroyed)
    {
        return;
    }

    bIsBeingDestroyed = true;

    // Owner에서 Component 제거하기
    if (AActor* MyOwner = GetOwner())
    {
        MyOwner->RemoveOwnedComponent(this);
        if (MyOwner->GetRootComponent() == this)
        {
            MyOwner->SetRootComponent(nullptr);
        }
    }

    if (bHasBegunPlay)
    {
        EndPlay(EEndPlayReason::Destroyed);
    }

    if (bHasBeenInitialized)
    {
        UninitializeComponent();
    }

    OnComponentDestroyed();

    // 나중에 ProcessPendingDestroyObjects에서 실제로 제거됨
    GUObjectArray.MarkRemoveObject(this);
}

void UActorComponent::Activate()
{
    // TODO: Tick 다시 재생
    bIsActive = true;
}

void UActorComponent::Deactivate()
{
    // TODO: Tick 멈추기
    bIsActive = false;
}

void UActorComponent::OnRegister()
{
    // Hook: Called by RegisterComponent()
    if (bAutoActive)
    {
        Activate();
    }

    if (bWantsInitializeComponent && !bHasBeenInitialized)
    {
        InitializeComponent();
    }
}

void UActorComponent::OnUnregister()
{
    // Hook: Called by UnregisterComponent()
    Deactivate();
}

std::unique_ptr<FActorComponentInfo> UActorComponent::GetComponentInfo()
{
    auto Info = std::make_unique<FActorComponentInfo>();
    SaveComponentInfo(*Info);
    
    return Info;
}

void UActorComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
{
    OutInfo.Origin = ComponentOrigin;
    OutInfo.ComponentID = ComponentID;
    OutInfo.ComponentClass = GetClass()->GetName();
    OutInfo.ComponentName = GetName();
    OutInfo.ComponentOwner = GetOwner() ? GetOwner()->GetName() : (TEXT("None"));
    OutInfo.bIsActive = bIsActive;
    OutInfo.bAutoActive = bAutoActive;
    OutInfo.bTickEnabled = bTickEnabled;
    OutInfo.bIsRoot = GetOwner() && (GetOwner()->GetRootComponent() == this);
}

void UActorComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    ComponentOrigin = Info.Origin;
    ComponentID = Info.ComponentID;
    SetFName(Info.ComponentName);
    bIsActive = Info.bIsActive;
    bAutoActive = Info.bAutoActive;
    bTickEnabled = Info.bTickEnabled;
}

void UActorComponent::RegisterComponent()
{
    if (bRegistered)
        return;

    bRegistered = true;
    OnRegister();
}

void UActorComponent::UnregisterComponent()
{
    if (!bRegistered)
        return;

    OnUnregister();
    bRegistered = false;
}
UObject* UActorComponent::Duplicate(UObject* InOuter)
{
    UActorComponent* NewComp = FObjectFactory::ConstructObjectFrom<UActorComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();
    return NewComp;
}
void UActorComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UObject::DuplicateSubObjects(Source, InOuter);
}
void UActorComponent::PostDuplicate()
{
    
}